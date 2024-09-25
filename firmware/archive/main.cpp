#include <wiringPi.h>
#include <softPwm.h>
#include "MotorDriver.h"

using namespace std;

#define SERVO 1

#define STBY 26
#define AIN1 27
#define AIN2 28
#define PWMA 29

int main() {
    wiringPiSetup();

    // DC motor
    MotorDriver motor(AIN1, AIN2, PWMA, STBY);

    motor.drive(255);
    delay(1000);
    motor.brake();
    motor.drive(-255);
    delay(1000);
    motor.brake();

    // Servo motor
    softPwmCreate(SERVO, 0, 200);

    // 15 (1500us pulse) is middle position, 10 (1000us pulse) is 0 degrees, 20 (2000us pulse) is 180 degrees
    softPwmWrite(SERVO, 15); // Move to 90 degrees
    delay(1000);
    softPwmWrite(SERVO, 12); // Move to 0 degrees
    delay(1000);
    softPwmWrite(SERVO, 18); // Move to 180 degrees
    delay(1000);
	softPwmWrite(SERVO, 15);
	delay(1000);

    return 0;
}
