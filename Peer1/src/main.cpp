/* Code to be uploaded on the control unit. */


#include <Arduino.h>

#define DELAY 1


const int PINS_ARR[] = {1, 2, 3, 5, 6, 7, 8};
int PIN_STATE[(sizeof(PINS_ARR) / sizeof(PINS_ARR[0]))];

int count = 0;


void setup() { 
  Serial.begin(115200);
  Serial5.begin(115200);

  while(!Serial) {
    Serial.println("No Serial Connection");
  }
  Serial.println("READY");
  while(!Serial5) {
    Serial.println("No Serial5 Connection");
  }
  Serial.println("READY, Serial Checks complete");

  // Initilize all input pins and set state to off
  for(unsigned int i = 0; i < (sizeof(PINS_ARR) / sizeof(PINS_ARR[0])); i++) {
    pinMode(PINS_ARR[i], INPUT_PULLDOWN);
    PIN_STATE[i] = 0;
  }

}


char in;
int i = 0;
int num;

char str[23];

boolean isData;

 
void loop() 
{

  isData = false;

  // [ S,0,0,0,0,0,0,0,0,0,0,E ] - sample all off w/ start and end char

  // Read and send switch states
  String txMsg = "S,";
  for(unsigned int i = 0; i < (sizeof(PINS_ARR) / sizeof(PINS_ARR[0])); i++) {
    PIN_STATE[i] = digitalRead(PINS_ARR[i]);
    txMsg += String(PIN_STATE[i]) + ",";
  }
  txMsg += "E\n";
  //txMsg[txMsg.length() - 1] = '\n';

  //Serial.print(txMsg);
  Serial5.print(txMsg);



  // READ INCOMING SERIAL DATA
  //i = 0;
  char msg[230] = "";
  while(Serial5.available() > 0) {
      //READ SERIAL MONITOR CHAR
      in = Serial5.read(); //.read() is non-blocking function (minimal delay)
      
      //Serial.println(String(int(in)-48) + String(msg));  //FOR TEST
      
      delayMicroseconds(100); //DO NOT DELETE THIS DELAY

      if(in == 'S' || i > 229) {
        i = 0;
      }

      if(int(in) == 10){  //if newline, break loop
        break;
      }
      msg[i] = in;
      i++;

      isData = true;
  }

  if(isData) {
    Serial.println(msg);
  }

  delay(DELAY);
}