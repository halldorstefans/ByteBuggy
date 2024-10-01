#include <iostream>
#include <string>
#include <csignal>
#include <thread>
#include <chrono>
#include <map>
#include <vector>
#include <wiringPi.h>
#include <softPwm.h>
#include <unistd.h>  // For usleep
#include "MotorDriver.h"
#include "mqtt/client.h"

using namespace std;
using namespace chrono;

bool running = true;

constexpr int HORN = 0;
constexpr int SERVO = 1;
constexpr int LIGHT = 4;
constexpr int STBY = 26;
constexpr int AIN1 = 27;
constexpr int AIN2 = 28;
constexpr int PWMA = 29;

constexpr int PWM_RANGE = 200;
constexpr int FULL_SPEED = 255;
constexpr int STEER_LEFT = 12;
constexpr int STEER_RIGHT = 18;
constexpr int STEER_STRAIGHT = 15;

const string RPI_CTRL_TOPIC	= "bytebuggy_control";

const string SERVER_ADDRESS	{ "mqtt://test.mosquitto.org:1883" };
const string CLIENT_ID		{ "bytebuggy_311" };
const vector<string> TOPICS { RPI_CTRL_TOPIC };
const vector<int> QOS { 1 };

/////////////////////////////////////////////////////////////////////////////

class ICommandObserver {
public:
    virtual void onCommandReceived(const string& topic, const string& command) = 0;
    virtual ~ICommandObserver() = default;
};

class SignalHandler : public ICommandObserver{
public:
    // Implementation of the observer interface
    void onCommandReceived(const std::string& topic, const std::string& command) override {
        if (topic == RPI_CTRL_TOPIC && command == "exit") {
            handleSignal(SIGTERM);
        }
    }

    static void handleSignal(int signum) {
        digitalWrite(LIGHT, 0);
	    running = false;
        exit(signum);
    }
};

class Car : public ICommandObserver{
private:
    MotorDriver motor;
    
public:
    Car() : motor(AIN1, AIN2, PWMA, STBY) {
        cout << "Initializing car..." << flush;
        
        // Initialize PWM for Servo
        if (softPwmCreate(SERVO, 0, PWM_RANGE) != 0) {
            throw std::runtime_error("Failed to initialize PWM on SERVO pin!");
        }
        
        // Initialize Light and Horn
        pinMode(LIGHT, OUTPUT);
        pinMode(HORN, OUTPUT);

        toggleLight();
        cout << "OK\n" << endl;
    }

    // Implementation of the observer interface
    void onCommandReceived(const std::string& topic, const std::string& command) override {
        if (topic == RPI_CTRL_TOPIC) {
            handleCommand(command);
        }
    }

    // Method to handle commands
    void handleCommand(const string& command) {
        if (command == "drive") {
            forward(FULL_SPEED); // Full speed ahead
        } else if (command == "brake") {
            brake(); // Stop the car
        } else if (command == "reverse") {
            reverse(); // Reverse
        } else if (command == "left") {
            steer(STEER_LEFT); // Turn left
        } else if (command == "right") {
            steer(STEER_RIGHT); // Turn right
        } else if (command == "straight") {
            steer(STEER_STRAIGHT); // Go straight
        } else if (command == "light") {
            toggleLight();  // Toggle the light
        } else if (command == "horn") {
            beep(500, 500);  // Sound the horn
        }
    }

    void forward(int speed) {
        motor.setSpeed(speed);
    }

    void brake() {
        motor.setSpeed(0);
    }

    void reverse() {
        motor.setSpeed(-FULL_SPEED);
    }

    void steer(int direction) {
        softPwmWrite(SERVO, direction);
    }

    // Function to toggle the light
    void toggleLight() {
        digitalWrite(LIGHT, !digitalRead(LIGHT));
    }

    // Function to beep the horn
    void beep(int frequency, int duration) {
        auto halfPeriod = chrono::microseconds(1000000 / (frequency * 2));
        int cycles = frequency * duration / 1000;

        for (int i = 0; i < cycles; ++i) {
            digitalWrite(HORN, HIGH);
            this_thread::sleep_for(halfPeriod);
            digitalWrite(HORN, LOW);
            this_thread::sleep_for(halfPeriod);
        }
    }
};

class MqttClient {
private:
    mqtt::client client;
    vector<ICommandObserver*> observers;  // List of observers

public:
    MqttClient(const string& serverAddress, const string& clientId)
        : client(serverAddress, clientId) {}

    // Add observer to the list
    void addObserver(ICommandObserver* observer) {
        observers.push_back(observer);
    }

    // Remove observer from the list
    void removeObserver(ICommandObserver* observer) {
        observers.erase(remove(observers.begin(), observers.end(), observer), observers.end());
    }

    // Notify all observers when a command is received
    void notifyObservers(const string& topic, const string& command) {
        for (auto& observer : observers) {
            observer->onCommandReceived(topic, command);
        }
    }

    // Connect to MQTT broker and subscribe to topics
    void connectAndSubscribe(const vector<string>& topics, const vector<int>& qos) {
        try {
            user_callback cb;
            client.set_callback(cb);

            auto connOpts = mqtt::connect_options_builder()
                .keep_alive_interval(seconds(30))
                .automatic_reconnect(seconds(2), seconds(30))
                .clean_session(false)
                .finalize();

            cout << "Connecting to the MQTT server..." << flush;
            mqtt::connect_response rsp = client.connect(connOpts);
            cout << "OK\n" << endl;

            if (!rsp.is_session_present()) {
                cout << "Subscribing to topics..." << flush;
                client.subscribe(topics, qos);
                cout << "OK" << endl;
            }
            else {
                cout << "Session already present. Skipping subscribe." << endl;
            }
        } catch (const mqtt::exception& exc) {
            cerr << "Error connecting or subscribing: " << exc.what() << endl;
            throw;
        }
    }

    // Continuously process MQTT messages and notify observers
    void processMessages() {
        while (client.is_connected()) {
            auto msg = client.consume_message();
            if (msg) {
                notifyObservers(msg->get_topic(), msg->to_string());  // Notify observers of the command
            } else if (!client.is_connected()) {
                reconnect();
            }
            if (!running) {
                cout << "Exiting..." << endl;
                break;
            }
        }
    }

    // Graceful disconnection
    void disconnect() {
        try {
            cout << "Disconnecting from MQTT server..." << flush;
            client.disconnect();
            cout << "OK" << endl;
        } catch (const mqtt::exception& exc) {
            cerr << "Error: " << exc.what() << endl;
        }
    }

private:
    // Class to receive callbacks
    class user_callback : public virtual mqtt::callback
    {
        void connection_lost(const string& cause) override {
            cout << "\nConnection lost" << endl;
            if (!cause.empty())
                cout << "\tcause: " << cause << endl;
        }

        void delivery_complete(mqtt::delivery_token_ptr tok) override {
            cout << "\n\t[Delivery complete for token: "
                << (tok ? tok->get_message_id() : -1) << "]" << endl;
        }

    public:
    };

    // Reconnect to MQTT broker
    void reconnect() {
        while (!client.is_connected()) {
            cerr << "Connection lost. Reconnecting..." << endl;
            try {
                client.reconnect();
            } catch (const mqtt::exception& exc) {
                cerr << "Reconnection failed: " << exc.what() << endl;
                this_thread::sleep_for(chrono::milliseconds(1000));
            }
        }
    }
};

// --------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    SignalHandler signalHandler;
    signal(SIGTERM, SignalHandler::handleSignal);

    // Initialize WiringPi, Servo, and other components
    if (wiringPiSetup() == -1) {
        cerr << "Failed to initialize WiringPi!" << endl;
        exit(1);
    }
    
    // Initialize the car
    Car car;

    // Initialize the MQTT client
    MqttClient mqttClient(SERVER_ADDRESS, CLIENT_ID);

    // Register observers
    mqttClient.addObserver(&car);
    mqttClient.addObserver(&signalHandler);

    // Connect to MQTT broker and subscribe to topic(s)
    mqttClient.connectAndSubscribe(TOPICS, QOS);

    // Start processing messages and notifying observers
    mqttClient.processMessages();

    // Disconnect
    mqttClient.disconnect();

	digitalWrite(HORN, LOW);

 	return 0;
}
