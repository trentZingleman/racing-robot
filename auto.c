#include <xc.h>
#include <sys/attribs.h> 

//Configure the clock
#pragma config FNOSC = PRIPLL // Oscillator Selection Bits Primary PLL System
#pragma config POSCMOD = EC // Uses external circuit
#pragma config FPLLIDIV = DIV_2         // PLL Input Divider (2x Divider)
#pragma config FPLLMUL = MUL_20         // PLL Multiplier (20x Multiplier)
#pragma config FPLLODIV = DIV_1         // System PLL Output Clock Divider (PLL Divide by 1)

#pragma config FPBDIV = DIV_8           // Peripheral Clock Divisor (Pb_Clk is Sys_Clk/8)
#pragma config UPLLIDIV = DIV_1         // USB PLL Input Divider (1x Divider) 

unsigned short int activatedSensors = 15;
unsigned short int previousScan = 15;
unsigned short int SearchCount = 0;
unsigned int speed = 250;
unsigned int searchSpeed = 150;
float searchTime = 4;    //seconds
unsigned int turnNumber = 1;      //do not modify; for search algorithm
float motorSpeedModifier = 1.5;

unsigned short int rightMotor = 1;          //d6
unsigned short int leftMotor = 0;           //d7

void setup();
void InitalizeLEDs();
void InitalizeTimers();
void InitalizeOC();
void InitalizeLightSensors();
void InitalizeButtons();
void InitalizeMotors();

void updateSensors();
void updateLEDs();
void LEDsOFF();

void updateDirection();

void searchTurn();
void reverseDirection();
void assignDirection();
void wait(float sec);

int main()
{
	setup();
	while (PORTAbits.RA6 == 0);  //wait for Btn1 to be pressed
	while (PORTAbits.RA7 == 0)   //run until Btn2 is pressed
	{
		updateSensors();
		updateLEDs();
		updateDirection();
	}
	return 1;
}

void setup()
{
	InitalizeLEDs();
	InitalizeTimers();
	InitalizeOC();
	InitalizeLightSensors();
	InitalizeButtons();
	InitalizeMotors();
}

void InitalizeLEDs()
{
	TRISBbits.TRISB10 = 0;
	TRISBbits.TRISB11 = 0;
	TRISBbits.TRISB12 = 0;
	TRISBbits.TRISB13 = 0;
	LEDsOFF();
}

void InitalizeTimers()
{
	//Timer 1
	T1CONbits.TON = 0;
	T1CONbits.TCKPS = 3; // prescale by 256
	T1CONbits.ON = 1; // Turn on Timer
	PR1 = 0xFFFF; // Period of Timer1 to be full
	TMR1 = 0; // Initialize Timer1 to above the searchtime

			  //Timer 2
	T2CONbits.TON = 0;
	T2CONbits.TCKPS = 7;
	PR2 = 1000; // one second period
	TMR2 = 0;
	T2CONbits.ON = 1;

	//Timer 4 (32 bit) 
	T4CONbits.T32 = 1;  //set it to 32 bit timer
	T4CONbits.TON = 0;
	T4CONbits.TCKPS = 7;
	PR4 = 0xFFFFFFFF;
	TMR4 = searchTime * 39000;
	T4CONbits.ON = 1;

}

void InitalizeOC()
{
	OC2CONbits.OCM = 6; // pwm mode with no fault protection
	OC2R = 0;   //start speed
	OC2RS = 0;  //start speed
	OC2CONbits.ON = 1; // will turn on OC2 module

	OC3CONbits.OCM = 6; // pwm mode with no fault protection
	OC3R = 0;   //start speed
	OC3RS = 0;  //start speed
	OC3CONbits.ON = 1;  // will turn on OC2 module
}

void InitalizeLightSensors()
{
	TRISDbits.TRISD13 = 1;
	TRISDbits.TRISD8 = 1;
	TRISDbits.TRISD0 = 1;
	TRISEbits.TRISE8 = 1;
}

void InitalizeButtons()
{
	TRISAbits.TRISA6 = 1;
	TRISAbits.TRISA7 = 1;
}

void InitalizeMotors()
{
	assignDirection();

	LATDbits.LATD1 = 0;
	LATDbits.LATD2 = 0;
}

void LEDsOFF()
{
	LATBbits.LATB10 = 0;
	LATBbits.LATB11 = 0;
	LATBbits.LATB12 = 0;
	LATBbits.LATB13 = 0;
}

void updateLEDs()
{
	LEDsOFF();

	LATBbits.LATB10 = activatedSensors / (0x8);
	LATBbits.LATB11 = activatedSensors / (0x4);
	LATBbits.LATB12 = activatedSensors / (0x2);
	LATBbits.LATB13 = activatedSensors / (0x1);
}

void updateSensors()
{
	activatedSensors = PORTDbits.RD13 * (0x8) | PORTDbits.RD8 * (0x4) | PORTDbits.RD0 * (0x2) | PORTEbits.RE8 * (0x1);
}

void updateDirection()
{
	if (activatedSensors == 2 || activatedSensors == 3 || activatedSensors == 11)
	{
		//slight left
		rightMotor = 1; leftMotor = 0;
		assignDirection();
		OC2RS = (speed * motorSpeedModifier); OC3RS = 0;
	}
	else if (activatedSensors == 7)
	{
		rightMotor = 0; leftMotor = 0;
		assignDirection();
		OC2RS = (speed * motorSpeedModifier); OC3RS = speed;
		wait(.75);
	}
	else if (activatedSensors == 4 || activatedSensors == 12 || activatedSensors == 13)
	{
		//slight right
		rightMotor = 1; leftMotor = 0;
		assignDirection();
		OC2RS = 0; OC3RS = speed;
	}
	else if (activatedSensors == 14)
	{
		rightMotor = 1; leftMotor = 1;
		assignDirection();
		OC2RS = speed; OC3RS = speed;
		wait(.75);
	}
	else if (activatedSensors == 5 || activatedSensors == 6 || activatedSensors == 9 || activatedSensors == 10 || activatedSensors == 15)
	{
		//continue forward
		rightMotor = 1; leftMotor = 0;
		assignDirection();
		OC2RS = speed * 2; OC3RS = speed;
	}
	else if (activatedSensors == 1 || activatedSensors == 0)
	{
		//turn left 90 degrees
		//        TRISDbits.TRISD6 = TRISDbits.TRISD7 = 0;
		//        OC2RS = speed * motorSpeedModifier; OC3RS = speed;
		//        wait(.125);
		//        assignDirection();
		TRISDbits.TRISD6 = TRISDbits.TRISD7 = 1;
		OC2RS = (speed * 2); OC3RS = speed;
		wait(.5);
		assignDirection();
	}
	else if (activatedSensors == 8)
	{
		//turn right 90 degrees
		//        TRISDbits.TRISD6 = TRISDbits.TRISD7 = 1;
		//        OC2RS = speed * motorSpeedModifier; OC3RS = speed;
		//        wait(.125);
		//        assignDirection();
		TRISDbits.TRISD6 = TRISDbits.TRISD7 = 0;
		OC2RS = speed * motorSpeedModifier; OC3RS = speed;
		wait(.5);
		assignDirection();
	}
	else if (activatedSensors == 15)
	{
		//search mode
		if (previousScan == 15) { SearchCount++; }
		else { SearchCount = 0; }

		if (SearchCount >= 350)
		{
			OC2RS = searchSpeed * 1.25; OC3RS = searchSpeed;
			if (TMR4 > (int)(searchTime * 39000))
			{
				if (turnNumber % 2 == 0) { searchTurn(); }
				reverseDirection();
				assignDirection();
				turnNumber++;   //every even turnNumber it will turn
				TMR4 = 0;   //reset the timer
			}
			SearchCount = 0;
		}
		else { rightMotor = 1; leftMotor = 0; }
	}
	previousScan = activatedSensors;
}

void searchTurn()
{
	TRISDbits.TRISD6 = TRISDbits.TRISD7 = 0;
	wait(.125);
}

void reverseDirection()
{
	if (rightMotor == 1) { rightMotor = 0; }
	else { rightMotor = 1; }
	if (leftMotor == 1) { leftMotor = 0; }
	else { leftMotor = 1; }
}

void assignDirection()
{
	TRISDbits.TRISD6 = rightMotor;
	TRISDbits.TRISD7 = leftMotor;
}

void wait(float sec)
{
	TMR1 = 0;
	while (TMR1 < sec * 39000);
}
