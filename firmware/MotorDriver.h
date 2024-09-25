#ifndef MOTOR_DRIVER_H
#define MOTOR_DRIVER_H

class MotorDriver {
    private:
        int In1, In2, PWM, Offset, Standby;
        void fwd(int speed);
    	void rev(int speed);
        void brake();
        bool DEBUG = false;
    public:
        MotorDriver(int In1pin, int In2pin, int PWMpin, int STBYpin);
        void setSpeed(int speed);
        void standby();
        void setDebug(bool); 
};

#endif /* MOTOR_DRIVER_H */
