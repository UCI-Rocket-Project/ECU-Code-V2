/* Code to be uploaded on the unit mounted within the control box. */


#include <Arduino.h>

#define DELAY 50

const int PINS_ARR[] = {1, 2, 3, 5, 6, 7, 8};
int PIN_STATE[(sizeof(PINS_ARR) / sizeof(PINS_ARR[0]))];

int count = 0;

// HEARTBEAT COMMANDS
unsigned int HEARTBEAT_DELAY = 2000;
unsigned long lastHeartbeat;
boolean heartbeat = false;


void setup() { 
  Serial.begin(115200);
  Serial5.begin(115200);
  Serial.println("Initializing...");

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

  lastHeartbeat = millis();

}


char in;
int i;
int num;
int sum1;
int sum2;
int checkSum;
int count2;

char str[30];




 
void loop() 
{
  

  // [ S,0,0,0,0,0,0,0,0,0,0,E ] - sample all off w/ start and end char
  //sum1 = 0;
  //sum2 = 0;
  //checkSum = 0;

  // Read and send switch states
  String txMsg = "S,";
  for(unsigned int i = 0; i < (sizeof(PINS_ARR) / sizeof(PINS_ARR[0])); i++) {
    PIN_STATE[i] = digitalRead(PINS_ARR[i]);
    txMsg += String(PIN_STATE[i]) + ",";
  }

  /*for(int j = 0; j < 16; j++){
      if(txMsg.charAt(j) == '0' || txMsg.charAt(j) == '1'){
        sum1= (sum1 + int(txMsg.charAt(j))) % 255;
        sum2 = (sum2 + sum1) % 255;
      }
  }

  checkSum = (sum2 << 8) | sum1;

  txMsg += String(checkSum);*/


  // SEND ONCE EVERY HEARTBEAT_DELAY ms
  txMsg += (heartbeat) ? "1" : "0";
  heartbeat = false;
  if(millis() - lastHeartbeat > HEARTBEAT_DELAY) {
    heartbeat = true;
    lastHeartbeat = millis();
  }

  txMsg += ",E";


  //txMsg[txMsg.length() - 1] = '\n';

  Serial.print(txMsg);
  Serial5.print(txMsg);



  // READ INCOMING SERIAL DATA
  i = 0;
  sum1 = 0;
  sum2 = 0;
  checkSum =0;
  //char msg[320] = "";
  String msg = "";
  while(Serial5.available() > 0) {
      //READ SERIAL MONITOR CHAR
      in = Serial5.read(); //.read() is non-blocking function (minimal delay)
      
      //Serial.println(String(int(in)-48) + String(msg));  //FOR TEST
      
      delayMicroseconds(100); //DO NOT DELETE THIS DELAY
      if(int(in) == 10){  //if newline, break loop
        break;
      }
      //msg[i] = in;
      msg += in;
      i++;
  }
  
  Serial.println(String(msg));

  //for(int j = 0; j < (i-1); j++){
/*  for(int j = 1; j < 16; j++){
    if(msg.charAt(j) == '0' || msg.charAt(j) == '1'){
      sum1= (sum1 + int(msg.charAt(j))) % 255;
      sum2 = (sum2 + sum1) % 255;
    }
  }

  checkSum = (sum2 << 8) | sum1;
  String CheckSUM = String(checkSum);
  Serial.println(checkSum);
  int Len = msg.length();
  int Posof = 0;
  Posof = msg.indexOf('+');
  String Check; 

  if(Posof != -1){
    for(int j = Posof+1; j < Len; j++){
      Check += msg.charAt(j);
    }
  }  

   Serial.println(Check);




  if(Check.equals(CheckSUM) == false){
    Serial.println("CheckSUM ERROR PEER SIDE!!!");
  }
  else{
    Serial.println("CheckSUMGOOOD");
  }*/



  

  delay(DELAY);
}