//Self balancing height control
int IRledPin =  7;	//debugging LED (checks whether values are transmitted correctly)
int board_led = 13;	//debugging LED (checks for off synchronization in packets)
int green_led = 8;	//debugging LED (not used yet)
int yellow_led = 12;
boolean ledState = false;
int blink_counter = 0;
char state = '0';
String incomingString;

int pulseValues[31];
int pulseLength = 0;
unsigned int index = 192;	//records speed
unsigned int old_index = 192;

boolean heliOn = false;

void setup(){                
  // initialize the IR digital pin as an output:
  pinMode(IRledPin, OUTPUT);      
  pinMode(yellow_led, OUTPUT);
  pinMode(board_led, OUTPUT);
  pinMode(green_led, OUTPUT);
  
  Serial.begin(115200);

  for (int i=0; i < 32; i++) 
    pulseValues[i] = 0;
}
void loop()                     
{	
	//digitalWrite(board_led, false);
	//digitalWrite(green_led, false);
	//
	updateInformation();
	//index = 255;
	Balance();
	//Serial.println("One cycle took this number of milli seconds "+ String(time, DEC));
	//delay(120);
}

void my_blink(){
  ledState ? ledState=false : ledState=true;
  digitalWrite(yellow_led,ledState);
};


void pulseIR(long microsecs) {
  cli();  // this turns off any background interrupts

  while (microsecs > 0) {
    // 38 kHz is about 13 microseconds high and 13 microseconds low
    digitalWrite(IRledPin, HIGH);  // this takes about 3 microseconds to happen
    delayMicroseconds(10);         // hang out for 10 microseconds
    digitalWrite(IRledPin, LOW);   // this also takes about 3 microseconds
    delayMicroseconds(10);         // hang out for 10 microseconds

    // so 26 microseconds altogether
    microsecs -= 26;

  }

  sei();  // this turns them back on
}

void Zero(){  
  pulseIR(300);
  delayMicroseconds(300);
  pulseLength += 600;
}

void One(){
  pulseIR(300);
  delayMicroseconds(600); 
  pulseLength += 900;
}

void sendPulseValue(int pulseValue){
  if (pulseValue == 1)
    One();
  else
    Zero(); 
}

void updateCommands(){	//updates state
	if (Serial.available()>0){
		state = Serial.read();
		if(state=='m'){
			heliOn = !heliOn;
			Serial.print("Toggling Engine ");
			Serial.println(heliOn);
			state = '0';
		} else if (state=='p') {
                        heliOn = true;  //turn on
                }else if (state=='o') {
                        heliOn = false;  //turn off
                        
                }else if (state=='k'){
                        index=index+5;
                        if (index > 255){
                           index = 255;
                        }
			Serial.print("Speed is ");
			Serial.println(index);
			state='0';
		} else if (state=='l'){
                        index=index-5;
                        if (index<137) {
                          index=137;
			}
                        Serial.print("Speed is ");
			Serial.println(index);
			state='0';

		} else if(state=='a'){
			state='a';
		} else if(state=='s'){
			state='s';
		} else if(state=='d'){
			state='d';
		} else if(state=='w'){
			state='w';
		} else{
			state='0';
		}
	}
	else {
		state = '0';
	}
}

void updateInformation(){
	if(Serial.available()>0){
		//char first = Serial.read();
		//char second = Serial.read();
		//char third = Serial.read();
		//if (Serial.read()=='a' && Serial.read()=='b' && Serial.read()=='c') my_blink();
		char a = Serial.read();
		char b = Serial.read();
		char c = Serial.read();
		
		// if(((int) b==-1)|| ((int) c==-1) || ((int) a==-1)){
			// digitalWrite(board_led, true);
			// return;	// the error case where serial port has nothing inside yet
		// } else {
			// digitalWrite(board_led, false);
		// }
		
		
		switch (a){
			case '2': index = 200; break;
			case '1': index = 100; break;
			case '0': index = 0; break;
			default:  index = old_index; return;	//the error case where serial port has wrong values
		}

		switch (b){
			case '0': index = index + 0; break;
			case '1': index = index + 10; break;
			case '2': index = index + 20; break;
			case '3': index = index + 30; break;
			case '4': index = index + 40; break;
			case '5': index = index + 50; break;
			case '6': index = index + 60; break;
			case '7': index = index + 70; break;
			case '8': index = index + 80; break;
			case '9': index = index + 90; break;
		}
		
		switch (c){
			case '0': index = index + 0; break;
			case '1': index = index + 1; break;
			case '2': index = index + 2; break;
			case '3': index = index + 3; break;
			case '4': index = index + 4; break;
			case '5': index = index + 5; break;
			case '6': index = index + 6; break;
			case '7': index = index + 7; break;
			case '8': index = index + 8; break;
			case '9': index = index + 9; break;
		}
		
		
		//if (index == 234){
		// if (index <= 255 && index>200){
			// digitalWrite(yellow_led,true);
		// } else {
			// digitalWrite(yellow_led,false);
		// }
		
		// if (index<=200){
			// digitalWrite(green_led,true);
		// } else {
			// digitalWrite(green_led,false);
		// }
		
		old_index = index;
		
		return;
	}
	//Serial.print("index is " );
	//Serial.print(index);
	//Serial.print("\n");
}

void Balance(){
	//updateInformation();	//does nothing if nothing is pressed
	
	
	pulseIR(2000);	//header
	delayMicroseconds(2000);	//wait for main message to come
	pulseLength = 4000;			//time sending so far
	
	sendPulseValue(0);
	sendPulseValue(0);
	sendPulseValue(1);
	sendPulseValue(1);
	sendPulseValue(1);
	sendPulseValue(1);
	sendPulseValue(1);
	sendPulseValue(1);
	//Pitch
	sendPulseValue(0);
	sendPulseValue(0);
	sendPulseValue(1);
	sendPulseValue(1);
	sendPulseValue(1);
	sendPulseValue(1);
	sendPulseValue(1);
	sendPulseValue(1);
	//Throttle
	sendPulseValue((index&128)>>7);
	sendPulseValue((index&64)>>6);
	sendPulseValue((index&32)>>5);
	sendPulseValue((index&16)>>4);
	sendPulseValue((index&8)>>3);
	sendPulseValue((index&4)>>2);
	sendPulseValue((index&2)>>1);
	sendPulseValue(index&1);
	//trim
	sendPulseValue(0);
	sendPulseValue(0);
	sendPulseValue(0);
	sendPulseValue(0);
	sendPulseValue(0);
	sendPulseValue(0);
	sendPulseValue(0);
	sendPulseValue(0);
	
	// if (blink_counter >= 7){
		// my_blink();
		// blink_counter = 0;
	// }
	
	// blink_counter++;
	pulseIR(360); 
	delay( (120000 - pulseLength)/1000 ); 
}
