#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <cctype>
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

const string SERVER_ADDRESS	{ "mqtt://test.mosquitto.org:1883" };
const string CLIENT_ID		{ "rpi_car" };
//const string TOPIC              { "rpi_mqtt" };

const int QOS = 1;

#define SERVO 1

#define LIGHT 4
#define HORN 0

#define STBY 26
#define AIN1 27
#define AIN2 28
#define PWMA 29

/////////////////////////////////////////////////////////////////////////////

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

// Function to make the buzzer beep at a specific frequency and duration
void beep(int frequency, int duration) {
    int halfPeriod = 1000000 / (frequency * 2);  // Calculate half-period in microseconds
    int cycles = frequency * duration / 1000;    // Calculate the number of cycles

    for (int i = 0; i < cycles; i++) {
        digitalWrite(HORN, HIGH);          // Turn buzzer on
        usleep(halfPeriod);                      // Wait for half of the period
        digitalWrite(HORN, LOW);           // Turn buzzer off
        usleep(halfPeriod);                      // Wait for the other half
    }
}

// --------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    cout << "Initializing..." << endl;
	wiringPiSetup();

	MotorDriver motor(AIN1, AIN2, PWMA, STBY);

	softPwmCreate(SERVO, 0, 200);

	pinMode(LIGHT, OUTPUT);
	pinMode(HORN, OUTPUT);

	mqtt::client client(SERVER_ADDRESS, CLIENT_ID);

	user_callback cb;
	client.set_callback(cb);

	auto connOpts = mqtt::connect_options_builder()
		.keep_alive_interval(seconds(30))
		.automatic_reconnect(seconds(2), seconds(30))
		.clean_session(false)
		.finalize();

	cout << "...OK" << endl;

	const vector<string> TOPICS { "command", "command_pi" };
	const vector<int> QOS { 0, 1 };

	try {
		cout << "Connecting to the MQTT server..." << flush;
		mqtt::connect_response rsp = client.connect(connOpts);
		cout << "OK\n" << endl;

		if (!rsp.is_session_present()) {
			cout << "Subscribing to topics..." << flush;
			client.subscribe(TOPICS, QOS);
			cout << "OK" << endl;
		}
		else {
			cout << "Session already present. Skipping subscribe." << endl;
		}

		// Consume messages
		string msgCommand = "";

		while (true) {
			auto msg = client.consume_message();
			msgCommand = "";
			if (msg) {				
				if (msg->get_topic() == "command_pi") {
					if (msg->to_string() == "exit") {
						cout << "Exit command received" << endl;
						break;
					}

					cout << msg->get_topic() << ": " << msg->to_string() << endl;
					msgCommand = msg->to_string();
				}
			}
			else if (!client.is_connected()) {
				cout << "Lost connection" << endl;
				while (!client.is_connected()) {
					this_thread::sleep_for(milliseconds(250));
				}
				cout << "Re-established connection" << endl;
			}

			if (msgCommand == "drive") {
				motor.setSpeed(255);
			}
			if (msgCommand == "brake") {
				motor.setSpeed(0);
			}
			if (msgCommand == "reverse") {
				motor.setSpeed(-255);
			}
			if (msgCommand == "left") {
				softPwmWrite(SERVO, 12);
			}
			if (msgCommand == "right") {
				softPwmWrite(SERVO, 18);
			}
			if (msgCommand == "straight") {
				softPwmWrite(SERVO, 15);
			}
			if (msgCommand == "light") {
				digitalWrite(LIGHT, !digitalRead(LIGHT));
			}
			if (msgCommand == "horn") {
				beep(500, 500);
			}
		}

		// Disconnect
		cout << "\nDisconnecting from the MQTT server..." << flush;
		client.disconnect();
		cout << "OK" << endl;
	}
	catch (const mqtt::exception& exc) {
		cerr << exc.what() << endl;
		return 1;
	}

	digitalWrite(HORN, LOW);

 	return 0;
}

