#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Wire.h>
//#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define CE 7
#define CSN 8

#define MESSAGE_SEND_PERIOD 250
#define BUTTON_DELAY 400
#define ROTORY_ENCODER_CHANGE_MIN_TIME 50

#define SIDE_SWITCH 4
#define ANALOG_LEFT_PUSHBUTTON 2
#define ANALOG_RIGHT_PUSHBUTTON 3
#define ROTORY_ENCODER_PUSHBUTTON 9

#define ROTORY_ENCODER_CLK 6
#define ROTORY_ENCODER_DT 5

#define OLED_RESET 4

#define CONTROLS_STANDARD 0
#define CONTROLS_ENCHANCED 1
#define CONTROLS_MEASURED 2

#define MENU_SIZE 5

//SERIAL OUTPUT


struct radioDataTrasnsmit {
	byte analog_left_X;
	byte analog_left_Y;
	byte analog_right_X;
	byte analog_right_Y;
	byte led_r;
	byte led_g;
	byte led_b;
	byte steering_wheel;
	byte reserved1;
	byte rotory_encoder;
	byte bit_array;
	byte message_no;
};

struct radioDataReceive {
	byte velocity_measured_left,
		velocity_measured_right,
		distance,
		control_mode,
		reserved4,
		reserved5,
		reserved6,
		reserved7,
		reserved8,
		message_no;
};

struct rotoryEncoder {
	bool clk_actual,
		 clk_prev,
		 dt_actual,
		 dt_prev,
		 switch_state;
	byte value;
};

struct radioStruct {
	byte last_message_no, 
		current_message_no, 
		messages_lost, 
		radio_not_availalble_counter;

	bool radio_not_availalble, empty_receive_data;
};

Adafruit_SSD1306 display(OLED_RESET);

/*
SimpleTimer 
*/
//radio variables
radioDataTrasnsmit message_transmit;
radioDataReceive message_receive;
radioStruct radioData;

RF24 radio(CE, CSN);
const byte txAddr[6] = { '1','N','o','d','e','1' };
const byte rxAddr[6] = { '1','N','o','d','e','2' };

byte outcoming_message[6];
byte message_counter = 0;

unsigned long now,
	last_message_send,
	last_encloder_change,
	SerialRawTimer,
	SerialSwitchesTimer,
	SerialIncomingRadioTimer,
	PrepareMessageTimer,
	SendRadioTimer,
	RotoryEncoderTimer,
	DisplayUpdateTimer;

bool analog_left_switch_state, 
	analog_right_switch_state;

rotoryEncoder rotory_encoder;

/*String menu_item_list[MENU_SIZE][MENU_SIZE] = {
	{"MENU1","MENU2","MENU3","MENU4","SPARE"},
	{"SUB11","SUB12","SUB13","SUB14","BACK"},
	{"SUB21","SUB22","SUB23","SUB24","BACK"},
	{"SUB31","SUB32","SUB33","SUB34","BACK"},
	{"SUB41","SUB42","SUB43","SUB44","BACK"},
};*/
byte menu_no = 0;


void setup()
{
	Serial.begin(9600);
	Serial.println("Starting...");

	pinMode(SIDE_SWITCH, INPUT_PULLUP);
	pinMode(ANALOG_LEFT_PUSHBUTTON, INPUT_PULLUP);
	pinMode(ANALOG_RIGHT_PUSHBUTTON, INPUT_PULLUP);

	pinMode(ROTORY_ENCODER_PUSHBUTTON, INPUT_PULLUP);
	pinMode(ROTORY_ENCODER_CLK, INPUT_PULLUP);
	pinMode(ROTORY_ENCODER_DT, INPUT_PULLUP);

	//pinMode(A4, INPUT_PULLUP);
	//pinMode(A5, INPUT_PULLUP);
	//---------------------- Radio config BEGIN -----------------
	
	radio.begin();
	radio.setDataRate(RF24_1MBPS);
	radio.setRetries(2, 5);
	radio.setChannel(0);// 100
	// -----------   JAK SI� ROZJEBIE TO ZMIEN KANA� ---------
	radio.openWritingPipe(txAddr);
	radio.openReadingPipe(0, rxAddr);
	radio.stopListening();
	
	//---------------------- Radio config END -----------------

	//---------------------- OLED Display -----------------
	display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
	display.clearDisplay();


	//---------------------- OLED Display END -----------------
	/*
	SerialRawTimer.setInterval(500, serialPrintRaw);	
	PrepareMessageTimer.setInterval(200, prepareOutMessage);
	SendRadioTimer.setInterval(250, sendRadio);
	SerialIncomingRadioTimer.setInterval(1000, serialPrintIncomingMessage);
	DisplayUpdateTimer.setInterval(500, display_handle);
	*/
	
}

void loop()
{
	now = millis();
	
	if (now - SerialRawTimer > 500)
	{
		//serialPrintIncomingMessage();
		SerialRawTimer = now;
	}

	if (now - SerialIncomingRadioTimer > 1000)
	{
		serialPrintIncomingMessage();
		SerialIncomingRadioTimer = now;
	}
	
	if (now - PrepareMessageTimer > 200)
	{
		prepareOutMessage();
		PrepareMessageTimer = now;
	}
	
	if (now - SendRadioTimer > 250)
	{
		sendRadio();
		SendRadioTimer = now;
	}
	
	if (now - DisplayUpdateTimer > 750)
	{
		display_refresh();
		DisplayUpdateTimer = now;
	}

	readRadio(0);
	
	display_draw();
	
	//Rotory encoder and tactile switches works better this way
	tactileSwitchesHandler();	
	rotoryEncoderHandler();
	
}

/*
int freeRam() {
	extern int __heap_start, *__brkval;
	int v;
	return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}
*/

