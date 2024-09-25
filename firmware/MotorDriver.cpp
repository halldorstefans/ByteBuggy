#include <stdio.h>
#include <softPwm.h>
#include <wiringPi.h>
#include "MotorDriver.h"

constexpr int MAX_SPEED = 255;

MotorDriver::MotorDriver(int In1pin, int In2pin, int PWMpin, int STBYpin)
{
    In1 = In1pin;
    In2 = In2pin;
    PWM = PWMpin;
    Standby = STBYpin;

    pinMode(In1, OUTPUT);
    pinMode(In2, OUTPUT);
    softPwmCreate(PWM, -MAX_SPEED, MAX_SPEED);
    pinMode(Standby, OUTPUT);
}

void MotorDriver::setSpeed(int speed)
{
    if (DEBUG)
    {
        printf("Setting speed as: %d \n\r", speed);
    }

    digitalWrite(Standby, HIGH);
    if (speed == 0)
        brake();
    else if (speed > 0)
        fwd(speed);
    else
        rev(-speed);
}

void MotorDriver::brake()
{
    if (DEBUG)
    {
        printf("Braking...\n\r");
    }

    digitalWrite(In1, HIGH);
    digitalWrite(In2, HIGH);
    softPwmWrite(PWM, 0);
}

void MotorDriver::fwd(int speed)
{
    if (DEBUG)
    {
        printf("Forward speed is: %d \n\r", speed);
    }

    digitalWrite(In1, HIGH);
    digitalWrite(In2, LOW);
    if (speed > MAX_SPEED)
        softPwmWrite(PWM, MAX_SPEED);
    else
        softPwmWrite(PWM, speed);
}

void MotorDriver::rev(int speed)
{
    if (DEBUG)
    {
        printf("Reverse speed is: %d \n\r", speed);
    }

    digitalWrite(In1, LOW);
    digitalWrite(In2, HIGH);
    if (speed < -MAX_SPEED)
        softPwmWrite(PWM, -MAX_SPEED);
    else
        softPwmWrite(PWM, speed);
}

void MotorDriver::standby()
{
    digitalWrite(Standby, LOW);
}

void MotorDriver::setDebug(bool debug)
{
    DEBUG = debug;
    if (DEBUG)
    {
        printf("Debug messages enabled. Outputting all operations...\n\r");
    }
}

