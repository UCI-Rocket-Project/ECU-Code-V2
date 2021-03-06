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
void setSolenoidStates();

// chipselect for teensy 3.6 sdcard
// sdcard
File logfile;
String dataline;
String fileName;

// indicator LED
const int led = 13;
const int sled = 0;

// Current Sensor Mux
const int ss0 = 23;
const int ss1 = 22;
const int ss2 = 20;
const int ss3 = 21;
const int ssSignal = A5;

int v3RegulatorCurrent = 0;
int v5RegulatorCurrent = 0;
int v12RegulatorCurrent = 0;

// Pressure Transducer / Analog Mux
const int ps0 = 27;
const int ps1 = 26;
const int ps2 = 24;
const int ps3 = 25;
const int pressurePin = A22;

int pressureSensor[11];
int analogInput[5];
int batteryVoltage = 0;

// Thermocouple Mux
const int ts0 = 17;
const int ts1 = 16;
const int ts2 = 14;
const int ts3 = 15;
const int thermoPin = A21;

int thermocouple[11];

// Solenoid Pins
const int solenoidPins[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
int solenoidState[10];

// Mux Channel Select
const int muxChannel[16][4] = {{0, 0, 0, 0}, {1, 0, 0, 0}, {0, 1, 0, 0}, {1, 1, 0, 0}, {0, 0, 1, 0}, {1, 0, 1, 0}, {0, 1, 1, 0}, {1, 1, 1, 0}, {0, 0, 0, 1}, {1, 0, 0, 1}, {0, 1, 0, 1}, {1, 1, 0, 1}, {0, 0, 1, 1}, {1, 0, 1, 1}, {0, 1, 1, 1}, {1, 1, 1, 1}};
long counter = 0;

// Solenoid current sensors
const int currentMuxMap[10] = {4, 3, 6, 5, 8, 7, 10, 9, 12, 11};
int solenoidCurrent[16];

// Running Average for thermocouple
const int numReadings = 10;

int readings[numReadings]; // the readings from the analog input
int readIndex = 0;         // the index of the current reading
int total = 0;             // the running total
int average = 0;

// SERIAL READ
char in;
int i;
int num;

// HEARTBEAT
bool heartbeat = false;
unsigned long heartbeatStart;
const int HEARTBEAT_SPAN = 5000;

// TELEMETRY DELAY
unsigned long telemStart;
const int TELEM_SPAN = 50;

void setup()
{

  // initialize serial
  Serial5.begin(115200);
  Serial.begin(115200);
 
  // initialize solenoid pins and states
  for (int i = 0; i < 10; i++)
  {
    pinMode(i, OUTPUT);
    solenoidState[i] = 0;
    digitalWrite(i, LOW);
  }

  // initialize solenoid current sensor mux pins
  for (int i = 20; i <= 23; i++)
  {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }

  // initialize pressure transducer sensor mux pins
  for (int i = 24; i <= 27; i++)
  {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }

  // initialize thermocouple sensor mux pins
  for (int i = 14; i <= 17; i++)
  {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }
  Serial.print("hit");
  // initialize status led
  pinMode(led, OUTPUT);

  // For running average on thermocouple
  for (int thisReading = 0; thisReading < numReadings; thisReading++)
  {
    readings[thisReading] = 0;
  }

  // set ADC Resolution (13 bit on board ADC)
  analogReadResolution(12);

  // pre check complete blink
  for (int i = 0; i < 10; i++)
  {
    digitalWrite(led, HIGH);
    delay(100);
    digitalWrite(led, LOW);
    delay(100);
  }

  // initialize SD Card
  if (!SD.begin(BUILTIN_SDCARD))
  {
    Serial.println("Card failed, or not present");
  }
  else
    Serial.println("Card initialized.");

  // create a new file on SD card
  int i = 0;
  while (true)
  {
    fileName = "LOG" + String(i) + ".CSV";
    char filename[fileName.length() + 1];
    fileName.toCharArray(filename, fileName.length() + 1);
    if (!SD.exists(filename))
    {
      // only open a new file if it doesn't exist
      logfile = SD.open(filename, FILE_WRITE);
      logfile.close();
      break;
    }
    i++;
    Serial.println(i);
    delay(100);
  }
  if (!logfile)
  {
    Serial.println("couldnt create file");
  }
  Serial.print("Logging to: ");
  Serial.println(fileName);

  delay(2000);

  // CSV starting line
  dataline = String("He BV,LNG BV,LOX BV,DLPR 1,DLPR 2,MVAS,Abort,Heartbeat,TC1,TC2,TC3,TC4.MVAS_HallEffect,Time");
  logfile.println(dataline);
  logfile.flush();

  // setup complete blink
  for (int i = 0; i < 3; i++)
  {
    digitalWrite(led, HIGH);
    delay(1000);
    digitalWrite(led, LOW);
    delay(1000);
  }
}

char msg[32] = "";
int arrCount;
int count;

void loop()
{

  digitalWrite(led, HIGH);
  digitalWrite(sled, HIGH);

  // Cycle through Mux channels to read all onboard sensors
  solenoidCurrentRead();
  pressureSensorRead();
  thermocoupleRead();

  digitalWrite(sled, LOW);
  digitalWrite(led, LOW);

  // reset vars for Serial monitor input (done via aggregation of chars)
  i = 0;

  // Serial input
  while (Serial5.available() > 0)
  {
    // READ SERIAL MONITOR CHAR
    in = Serial5.read(); //.read() is non-blocking function (minimal delay)

    // Serial.println(String(int(in)-48) + String(msg));  //FOR TEST

    delayMicroseconds(140); // DO NOT DELETE THIS DELAY
    if (int(in) == 10)
    { // if newline, break loop
      break;
    }
    msg[i] = in;
    i++;
  }



  /* -- RELAY CONTROL MODE --
   * Relay trigger format
   * [relayPins[#]][relay # state] i.e. 00 -> relayPins[0] OFF
   */
  // if(isDigit(msg[0]) && isDigit(msg[1])){
  //   solenoidState[int(msg[0])-48] = int(msg[1])-48;
  /*if(int(msg[1])-48 == 1){
    digitalWrite(solenoidPins[int(msg[0])-48], HIGH);
  }else{
    digitalWrite(solenoidPins[int(msg[0])-48], LOW);
  }*/
  /* Serial.println(int(msg[0])-48);  //FOR TEST
  Serial.println(int(msg[1])-48);  //FOR TEST
  delay(2000);*/
  //}

  // Count num elements in msg
  arrCount = 0;
  for (int i = 0; i < 32; i++)
  {
    if (msg[i] == '0' || msg[i] == 'a')
    {
      arrCount++;
    }
  }
  //Serial.println(arrCount);

  // arrCount == sizeof(solenoidPins)/sizeof(solenoidPins[0])
  /*if(msg[0] == 'S' && msg[15] == 'E') {
    arrCount = 0;
    for(int i = 0; i < 32; i++) {
      if(msg[i] == '0' || msg[i] == '1') {
        solenoidState[arrCount] = msg[i] - '0';
        arrCount++;
      }
    }
  }*/

  // basic check sum
  if(arrCount >= 7){
    
    count = 0;
    while (msg[count] != 'S')
    {
      count++;
    }
    arrCount = 0;
    while (msg[count] != 'E' && count < 17 && arrCount < 10)
    {
      if (msg[count] == '0')
      {
        solenoidState[arrCount] = 0;
        arrCount++;
      }
      if (msg[count] == 'a')
      {
        solenoidState[arrCount] = 1;
        arrCount++;
      }
      count++;
    }

  /*int sum1 = 0;
  int sum2 = 0;
  int EPos = 0;

  for(int j = 0; j < 15; j++){  //Split into 2 halves (1 btye each)
    if(msg[j] == '0' || msg[j] == '1'){
      sum1= (sum1 + int(msg[j])) % 255;
      sum2 = (sum2 + sum1) % 255;
    }
  }
  
  int CheckSum = (sum2 << 8) | sum1;  //shift sum2 and append sum1 to it
  String CheckSUM = String(CheckSum);
  String Check;

  for(int j = 0; j < 32; j++){ 
    if(msg[j] == 'E'){
      EPos = j;
      break;
    }
  }

  for(int j = 16; j < EPos-1; j++){
    Check += msg[j];
  }

  if(Check.equals(CheckSUM) == false){
    Serial.println("Check Sum ERROR!!");
  }
  else{
    Serial.println("Check Sum GOOD");
  }*/

  
  // if(CheckSum != int(msg[EPos - 1])){   //Check the index before E and compare to CheckSum
  //   Serial.println("CheckSUM ERROR 2!!");
  // }

    /* ABORT
    * all bleeds open, mvas closed, dlpr off
    */
   /*
   
    if(solenoidState[6] == 1) {
      solenoidState[0] = 1; // HE BLEED
      solenoidState[1] = 0; // LNG BLEED
      solenoidState[2] = 0; // LOX BLEED
      solenoidState[3] = 0; //DLPR 1
      solenoidState[4] = 0; //DLPR 2
      solenoidState[5] = 0; //MVAS
    }
    */
  }

  Serial.println(msg);





  /* -- HEARTBEAT --
   * looking for msg == 'rp' and will send state back on serial
   */
  if (msg[0] == 'r' && msg[1] == 'p')
  {
    heartbeat = true;
    heartbeatStart = millis();
  }

  setSolenoidStates();

  // save data to SD card and print to serial
  printData();

  delay(5);
}

// For running average on thermcouple
void avg()
{

  total = total - readings[readIndex];
  // read from the sensor:
  readings[readIndex] = thermocouple[1];
  // add the reading to the total:
  total = total + readings[readIndex];
  // advance to the next position in the array:
  readIndex = readIndex + 1;

  // if we're at the end of the array...
  if (readIndex >= numReadings)
  {
    // ...wrap around to the beginning:
    readIndex = 0;
  }

  // calculate the average:
  average = total / numReadings;
}

// reads pressure sensor data

void pressureSensorRead()
{

  // read Mux
  for (int i = 1; i <= 8; i++)
  {
    pressureSensor[i] = readPressureMux(16 - i);
  }
  pressureSensor[9] = readPressureMux(6);
  pressureSensor[10] = readPressureMux(5);
  for (int i = 1; i <= 4; i++)
  {
    analogInput[i] = readPressureMux(5 - i);
  }
  batteryVoltage = readPressureMux(0);
}

int readPressureMux(int pressureChannel)
{
  int controlPin[] = {ps0, ps1, ps2, ps3};

  // loop through the 4 sig
  for (int i = 0; i < 4; i++)
  {
    digitalWrite(controlPin[i], muxChannel[pressureChannel][i]);
  }

  // read the value at the SIG pin
  int pressureVal = analogRead(pressurePin); // ads.readADC_SingleEnded(pressurePin);
  // return the value
  return pressureVal;
}

// reads solenoid current data

void solenoidCurrentRead()
{

  // read Mux
  for (int i = 0; i < 10; i++)
  {
    /*if (i % 2 == 0) {
      solenoidCurrent[i] = readCurrentMux(i + 1);
    } else {
      solenoidCurrent[i] = readCurrentMux(i + 3);
    }*/
    solenoidCurrent[i] = readCurrentMux(currentMuxMap[i]);
  }
  v3RegulatorCurrent = readCurrentMux(13);
  v5RegulatorCurrent = readCurrentMux(14);
  v12RegulatorCurrent = readCurrentMux(15);
}

int readCurrentMux(int currentChannel)
{
  int controlPin[] = {ss0, ss1, ss2, ss3};

  // loop through the 4 sig
  for (int i = 0; i < 4; i++)
  {
    digitalWrite(controlPin[i], muxChannel[currentChannel][i]);
  }

  // read the value at the SIG pin
  // int currentVal = analogRead(ssSignal);
  // return the value
  return analogRead(ssSignal);
}

// reads thermocouple data
void thermocoupleRead()
{

  // only 10 thermocouple pins
  // read Mux
  for (int i = 0; i < 10; i++)
  {
    thermocouple[i] = readThermoMux(16 - i - 1);
  }
}

int readThermoMux(int thermoChannel)
{
  int controlPin[] = {ts0, ts1, ts2, ts3};

  // loop through the 4 sig
  for (int i = 0; i < 4; i++)
  {
    digitalWrite(controlPin[i], muxChannel[thermoChannel][i]);
  }

  // read the value at the SIG pin
  // int thermoVal = analogRead(thermoPin); //ads.readADC_SingleEnded(thermoPin);
  // return the value
  return analogRead(thermoPin);
}

void setSolenoidStates()
{
  for (int i = 0; i < 10; i++)
  {
    digitalWrite(solenoidPins[i], solenoidState[i]);
  }
}

// prints and loggs all data                                                                                                    

void printData()                                                                                                                                                      
{
  dataline = "";


  for (int i = 0; i < 7; i++)
  {
    dataline += String(solenoidState[i]);
    dataline += ',';
  }

  // K-type
  for (int i = 0; i < 2; i++)
  {
    // dataline += String(thermocouple[i]);
    //dataline += String(int((0.45631 * thermocouple[i]) - 161.466));
    dataline += String(int((0.40735 * thermocouple[i]) - 133.41));
    dataline += ',';
  }
  //T-type
  for (int i = 2; i < 10; i++)
  {
    dataline += String(int((0.3989 * thermocouple[i]) - 128.298));
    dataline += ',';
  }

  if(analogRead(A20) < 2000) {
    dataline += "1,";
  }else{
    dataline += "0,";
  }

  int sum1 = 0;
  int sum2 = 0;
  int checkSum = 0;

  //for(int j = 0; j < (i-1); j++){
  /*for(int j = 0; j < 13; j++){
    if(dataline.charAt(j) == '0' || dataline.charAt(j) == '1'){
      sum1= (sum1 + int(dataline.charAt(j))) % 255;
      sum2 = (sum2 + sum1) % 255;
    }
  }

  checkSum = (sum2 << 8) | sum1;
  dataline += "+"+String(checkSum);
  dataline += ',';*/



  /*dataline += String(batteryVoltage);
  dataline += ',';

  for (int i = 0; i < 10; i++)
  {
    dataline += String(solenoidCurrent[i]);
    dataline += ',';
  }

  dataline += String(v3RegulatorCurrent);
  dataline += ',';
  dataline += String(v5RegulatorCurrent);
  dataline += ',';
  dataline += String(v12RegulatorCurrent);
  dataline += ',';    

  for (int i = 1; i <= 10; i++)
  {
    dataline += String(pressureSensor[i]);
    dataline += ',';
  }

  for (int i = 1; i <= 4; i++)
  {
    dataline += String(analogInput[i]);
    dataline += ',';
  }*/


  dataline += String(millis());

  // log to SD card
  char filename[fileName.length() + 1];
  fileName.toCharArray(filename, fileName.length() + 1);
  logfile = SD.open(filename, FILE_WRITE);
  if (logfile)
  {
    logfile.println(dataline);
    dataline += ",1,";
    digitalWrite(led, LOW);
  }
  else
  {
    dataline += ",0,";
    digitalWrite(led, HIGH);
  }

  dataline += String(heartbeat);

  dataline += ('E');


  logfile.close();

  // every 50 ms
  /*if (millis() - telemStart > TELEM_SPAN)
  {
    
  }*/

  Serial.println(dataline);
  Serial5.println(dataline); // added

  // reset heartbeat
  if (millis() - heartbeatStart > HEARTBEAT_SPAN)
  {
    heartbeat = false;
  }

  delay(5);
} 
