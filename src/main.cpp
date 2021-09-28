#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

void avg();
void pressureSensorRead();
int readPressureMux(int pressureChannel);
void solenoidCurrentRead();
int readCurrentMux(int currentChannel);
void thermocoupleRead();
int readThermoMux(int thermoChannel);
void printData();




// chipselect for teensy 3.6 sdcard
// sdcard
File logfile;
String dataline;
String fileName;


//indicator LED
const int led = 13;
const int sled = 0;

//Current Sensor Mux
const int ss0 = 23;
const int ss1 = 22;
const int ss2 = 20;
const int ss3 = 21;
const int ssSignal = A5;

int solenoidCurrent[11];
int v3RegulatorCurrent = 0;
int v5RegulatorCurrent = 0;
int v12RegulatorCurrent = 0;

//Pressure Transducer / Analog Mux
const int ps0 = 27;
const int ps1 = 26;
const int ps2 = 24;
const int ps3 = 25;
const int pressurePin = A22;

int pressureSensor[11];
int analogInput[5];
int batteryVoltage = 0;

//Thermocouple Mux
const int ts0 = 17;
const int ts1 = 16;
const int ts2 = 14;
const int ts3 = 15;
const int thermoPin = A21;

int thermocouple[11];

//Solenoid Pins
const int s1 = 1;
const int s2 = 2;
const int s3 = 3;
const int s4 = 4;
const int s5 = 5;
const int s6 = 6;
const int s7 = 7;
const int s8 = 8;
const int s9 = 9;
const int s10 = 10;

const int solenoidPins[] = {1, 2, 3, 4, 5, 6, 6, 7, 8, 9, 10};
int solenoidState[11];


//Mux Channel Select
const int muxChannel[16][4] = {{0, 0, 0, 0}, {1, 0, 0, 0}, {0, 1, 0, 0}, {1, 1, 0, 0}, {0, 0, 1, 0}, {1, 0, 1, 0}, {0, 1, 1, 0}, {1, 1, 1, 0}, {0, 0, 0, 1}, {1, 0, 0, 1}, {0, 1, 0, 1}, {1, 1, 0, 1}, {0, 0, 1, 1}, {1, 0, 1, 1}, {0, 1, 1, 1}, {1, 1, 1, 1} };
long counter = 0;




//Running Average for thermocouple
const int numReadings = 10;

int readings[numReadings];      // the readings from the analog input
int readIndex = 0;              // the index of the current reading
int total = 0;                  // the running total
int average = 0;







//TEST
const int relayPins[] = {12, 11, 10, 9, 8, 7};

char in;
int i;
int num;







void setup() {


  // initialize serial
  Serial.begin(115200);

  // initialize solenoid pins
  for (int i = 0; i <= 10; i++) {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }

  // initial solenoid states
  for (int i = 1; i <= 10; i++) {
    solenoidState[i] = 0;
  }

  // initialize solenoid current sensor mux pins
  for (int i = 20; i <= 23; i++) {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }

  // initialize pressure transducer sensor mux pins
  for (int i = 24; i <= 27; i++) {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }

  // initialize thermocouple sensor mux pins
  for (int i = 14; i <= 17; i++) {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }
  Serial.print("hit");
  // initialize status led
  pinMode(led, OUTPUT);





  //set ADC Resolution (13 bit on board ADC)
  analogReadResolution(12);


  // pre check complete blink
  for(int i = 0; i < 20; i++) {
    digitalWrite(led, HIGH);
    delay(100);
    digitalWrite(led, LOW);
    delay(100);
  }

  

  // initialize SD Card
  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println("Card failed, or not present");
  }
  else Serial.println("Card initialized.");


  // create a new file on SD card
  int i = 0;
  while (true) {
    fileName = "LOG" + String(i) + ".CSV";
    char filename[fileName.length() + 1];
    fileName.toCharArray(filename, fileName.length()+1);
    if (! SD.exists(filename)) {
      // only open a new file if it doesn't exist
      logfile = SD.open(filename, FILE_WRITE);
      logfile.close();
      break;
    }
    i++;
    Serial.println(i);
    delay(100);
  }
  if (! logfile) {
    Serial.println("couldnt create file");
  }
  Serial.print("Logging to: ");
  Serial.println(fileName);

  delay(2000);

  //CSV starting line
  dataline = String( "Battery Voltage,Solenoid State 1,Solenoid State 2,Solenoid State 3,Solenoid State 4,Solenoid State 5,Solenoid State 6,Solenoid State 7,Solenoid State 8,Solenoid State 9,Solenoid State 10,Solenoid Current 1,Solenoid Current 2,Solenoid Current 3,Solenoid Current 4,Solenoid Current 5,Solenoid Current 6,Solenoid Current 7,Solenoid Current 8,Solenoid Current 9,Solenoid Current 10,3.3V Reg Current,5V Reg Current,12V Reg Current,Pressure Sensor 1,Pressure Sensor 2,Pressure Sensor 3,Pressure Sensor 4,Pressure Sensor 5,Pressure Sensor 6,Pressure Sensor 7,Pressure Sensor 8,Pressure Sensor 9,Pressure Sensor 10,Analog Sensor 1,Analog Sensor 2,Analog Sensor 3,Analog Sensor 4,Thermocouple 1,Thermocouple 2,Thermocouple 3,Thermocouple 4,Thermocouple 5,Thermocouple 6,Thermocouple 7,Thermocouple 8,Thermocouple 9,Thermocouple 10,Time");
  logfile.println(dataline);
  logfile.flush();

  //For running average on thermocouple
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;
  }

  // setup complete blink
  for(int i = 0; i < 3; i++) {
    digitalWrite(led, HIGH);
    delay(1000);
    digitalWrite(led, LOW);
    delay(1000);
  }

}





void loop() {



  digitalWrite(led, HIGH);
  digitalWrite(sled, HIGH);

  //turn on test solenoid
  digitalWrite(s1, solenoidState[1]);

  //Cycle through Mux channels to read all onboard sensors
  solenoidCurrentRead();
  pressureSensorRead();
  thermocoupleRead();

  digitalWrite(sled, LOW);
  digitalWrite(led, LOW);


  //save data to SD card and print to serial
  printData();


  //reset vars for Serial monitor input (done via aggregation of chars)
  i = 0;
  char msg[32] = "";

  //Serial input
  while(Serial.available() > 0) {
      //READ SERIAL MONITOR CHAR
      in = Serial.read(); //.read() is non-blocking function (minimal delay)
      
      //Serial.println(String(int(in)-48) + String(msg));  //FOR TEST
      
      delayMicroseconds(100); //DO NOT DELETE THIS DELAY
      if(int(in) == 10){  //if newline, break loop
        break;
      }
      msg[i] = in;
      i++;
  }

  /* -- RELAY CONTROL MODE --
   * Relay trigger format
   * [relayPins[#]][relay # state] i.e. 00 -> relayPins[0] OFF
   */
  if(isDigit(msg[0]) && isDigit(msg[1])){
    if(int(msg[1])-48 == 1){
      digitalWrite(solenoidPins[int(msg[0])-48], HIGH);      
    }else{
      digitalWrite(solenoidPins[int(msg[0])-48], LOW);
    }
    /* Serial.println(int(msg[0])-48);  //FOR TEST
    Serial.println(int(msg[1])-48);  //FOR TEST
    delay(2000);*/
  }


}


  //For running average on thermcouple
void avg(){

  total = total - readings[readIndex];
  // read from the sensor:
  readings[readIndex] = thermocouple[1];
  // add the reading to the total:
  total = total + readings[readIndex];
  // advance to the next position in the array:
  readIndex = readIndex + 1;

  // if we're at the end of the array...
  if (readIndex >= numReadings) {
    // ...wrap around to the beginning:
    readIndex = 0;
  }

  // calculate the average:
  average = total / numReadings;
}



//reads pressure sensor data

void pressureSensorRead() {

  //read Mux
  for (int i = 1; i <= 8; i++) {
    pressureSensor[i] = readPressureMux(16 - i);
  }
  pressureSensor[9] = readPressureMux(6);
  pressureSensor[10] = readPressureMux(5);
  for (int i = 1; i <= 4; i++) {
    analogInput[i] = readPressureMux(5 - i);
  }
  batteryVoltage = readPressureMux(0);

}

int readPressureMux(int pressureChannel) {
  int controlPin[] = {ps0, ps1, ps2, ps3};

  //loop through the 4 sig
  for (int i = 0; i < 4; i ++) {
    digitalWrite(controlPin[i], muxChannel[pressureChannel][i]);
  }

  //read the value at the SIG pin
  int pressureVal = analogRead(pressurePin);//ads.readADC_SingleEnded(pressurePin); 
  //return the value
  return pressureVal;
}

// reads solenoid current data

void solenoidCurrentRead() {


  //read Mux
  for (int i = 1; i <= 10; i++) {
    if (i % 2 == 0) {
      solenoidCurrent[i] = readCurrentMux(i + 1);
    } else {
      solenoidCurrent[i] = readCurrentMux(i + 3);
    }
  }
  v3RegulatorCurrent = readCurrentMux(13);
  v5RegulatorCurrent = readCurrentMux(14);
  v12RegulatorCurrent = readCurrentMux(15);



}

int readCurrentMux(int currentChannel) {
  int controlPin[] = {ss0, ss1, ss2, ss3};

  //loop through the 4 sig
  for (int i = 0; i < 4; i ++) {
    digitalWrite(controlPin[i], muxChannel[currentChannel][i]);
  }

  //read the value at the SIG pin
  int currentVal = analogRead(ssSignal);
  //return the value
  return currentVal;
}

//reads thermocouple data

void thermocoupleRead() {


  //read Mux
  for (int i = 1; i <= 10; i++) {
    thermocouple[i] = readThermoMux(16 - i);
  }


}

int readThermoMux(int thermoChannel) {
  int controlPin[] = {ts0, ts1, ts2, ts3};

  //loop through the 4 sig
  for (int i = 0; i < 4; i ++) {
    digitalWrite(controlPin[i], muxChannel[thermoChannel][i]);
  }

  //read the value at the SIG pin

  int thermoVal =analogRead(thermoPin); //ads.readADC_SingleEnded(thermoPin);
  //return the value
  return thermoVal;
}


// prints and loggs all data

void printData() {

  dataline = String(batteryVoltage );
  dataline += ',';

  for (int i = 1; i <= 10; i++) {
    dataline += String(solenoidState[i]);
    dataline += ',';
  }


  for (int i = 1; i <= 10; i++) {
    dataline += String(solenoidCurrent[i]);
    dataline += ',';
  }


  dataline += String(v3RegulatorCurrent);
  dataline += ',';
  dataline += String(v5RegulatorCurrent);
  dataline += ',';
  dataline += String(v12RegulatorCurrent);
  dataline += ',';

  for (int i = 1; i <= 10; i++) {
    dataline += String(pressureSensor[i]);
    dataline += ',';
  }


  for (int i = 1; i <= 4; i++) {
    dataline += String(analogInput[i]);
    dataline += ',';
  }

  //averages thermocouple readings
  avg();

  dataline += String(average);
  dataline += ',';

  for (int i = 2; i <= 10; i++) {
    dataline += String(thermocouple[i]);
    dataline += ',';
  }

  dataline += String(millis());
  dataline += '\n';
  

  //log to SD card
  char filename[fileName.length() + 1];
  fileName.toCharArray(filename, fileName.length()+1);
  logfile = SD.open(filename, FILE_WRITE);
  if(logfile) {
    logfile.print(dataline);
    digitalWrite(led, LOW);
  }else{
    Serial.print("error opening " + fileName + "\t");
    digitalWrite(led, HIGH);
  }
  logfile.close();
  Serial.print(dataline);
  //logfile.print(dataline);
  //logfile.flush();


}