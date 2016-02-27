/*
Last Build: 4/3/15
This version includes completed code for the following:
  1)4 Thermistors with averaging and error detection
  2)RTC time stamp- need to deal with setting time properly
  3)SD Card datalogging for the above features
  4)Thermocouple Temperature Readings and write to SD Card
  5)Heart Rate connected but need to confirm 2 i2C devices can communicate on the same bus. Untested. Needs to be connected to datafile
In development:
  2) Body Heat Conversion code
  5) Accelerometer
  6) WiFi
  7) Error LED's
*/

// which analog pin to connect
#define THERMISTORPIN0 A0 //body heat 1
#define THERMISTORPIN1 A1 //body heat 2
#define THERMISTORPIN2 A2 //body heat 3
#define THERMISTORPIN3 A3 //body heat 4
#define THERMISTORPIN4 A4 //Evap 1
#define THERMISTORPIN5 A5 //Condenser
#define THERMISTORPIN6 A6 //Evap 2
#define THERMISTORPIN7 A7 //not used

#define THERMISTORNOMINAL       10000   // resistance at 25 degrees C 
#define TEMPERATURENOMINAL      25      // temp. for nominal resistance (almost always 25 C) 
#define NUMSAMPLES              5       // how many samples to take and average, more takes longer
#define BCOEFFICIENT            3950    // The beta coefficient of the thermistor (usually 3000-4000)
#define SERIESRESISTOR          10000   // the value of the 'other' resistor
#define ONE_WIRE_BUS            2       // Data wire is plugged into port 2 on the Arduino
#define TEMPERATURE_PRECISION   9
#define HRMI_I2C_ADDR           127
#define HRMI_HR_ALG              1      // 1= average sample, 0 = raw sample

#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <RTClib.h>
#include <OneWire.h>
#include <DallasTemperature.h>


const int chipSelect = 53;
int c =0;
//int csPin = 4;
//String evap;
float safetyShutOff,condenser,evaporator;

int samples0[NUMSAMPLES]; //therm0
int samples1[NUMSAMPLES];//therm1
int samples2[NUMSAMPLES];//therm2
int samples3[NUMSAMPLES];//therm3
int samples4[NUMSAMPLES];//therm4
int samples5[NUMSAMPLES];//therm5
int samples6[NUMSAMPLES];//therm6
int samples7[NUMSAMPLES];//therm7
float condTherm, evapTherm;

int Cond_safe = 135; //Safety Net to protect against 
int controlPin = 13;// change this when we get board.
  
float steinhart0F,steinhart1F,steinhart2F,steinhart3F,AVG_SteinF,steinhart4F, steinhart5F,steinhart6F ,steinhart7F;
RTC_Millis rtc;
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
//DallasTemperature sensors(&oneWire);
// arrays to hold device addresses
//DeviceAddress evapThermometer, condenserThermometer,safetyShutOff_thermometer;

void setup(void) {
  pinMode(13,OUTPUT); // change pin to break out pin on MegaHeatv2 header for compressor control
  setupHeartMonitor(HRMI_HR_ALG);
  Serial.begin(9600);
  analogReference(EXTERNAL);
  // This line sets the RTC with an explicit date & time, for example to set
  // January 21, 2014 at 3am you would call:
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  //rtc.adjust(DateTime(2015, 4, 3, 21, 48, 50)); //change
  
   Serial.print("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
   pinMode(10, OUTPUT);

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");
  
  // Start up the library
//  sensors.begin();
//  // locate devices on the bus
//  Serial.print("Locating devices...");
//  Serial.print("Found ");
//  Serial.print(sensors.getDeviceCount(), DEC);
//  Serial.println(" devices.");
//  if (!sensors.getAddress(evapThermometer, 0)) Serial.println("Unable to find address for Device 0"); 
//  if (!sensors.getAddress(condenserThermometer, 1)) Serial.println("Unable to find address for Device 1"); 
//  if (!sensors.getAddress(safetyShutOff_thermometer, 2)) Serial.println("Unable to find address for Device 1");
  
  
  // set the resolution to 9 bit
//  sensors.setResolution(evapThermometer, TEMPERATURE_PRECISION);
//  sensors.setResolution(condenserThermometer, TEMPERATURE_PRECISION);
//  sensors.setResolution(safetyShutOff_thermometer, TEMPERATURE_PRECISION);
}


void loop(){
  uint8_t i;
  float average0,average1,average2,average3,average4,average5,average6,average7;
  // make a string for assembling the data to log:
  String dataString = "";
  
  /*Get Heart Rate*/
  int heartRate = getHeartRate();
  
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open("datalog.txt", FILE_WRITE);
 
  // take N samples in a row, with a slight delay
  for (i=0; i< NUMSAMPLES; i++) {
   //samples t0
    samples0[i] = analogRead(THERMISTORPIN0);
   delay(10);
   
   //samples t1
   samples1[i] = analogRead(THERMISTORPIN1);
   delay(10);
   
   //samples t2
   samples2[i] = analogRead(THERMISTORPIN2);
   delay(10);
   
   //samples t3
   samples3[i] = analogRead(THERMISTORPIN3);
   delay(10);
   
   //samples t4
   samples4[i] = analogRead(THERMISTORPIN4);
   delay(10);
   
   //samples t5
   samples5[i] = analogRead(THERMISTORPIN5);
   delay(10);
   
   //samples t6
   samples6[i] = analogRead(THERMISTORPIN6);
   delay(10);
   
   //samples t7
   samples7[i] = analogRead(THERMISTORPIN7);
   delay(10);
  }
 
  // average all the samples of each thermistor out
  average0 = 0;
  average1 = 0;
  average2 = 0;
  average3 = 0;
  average4 = 0; 
  average5 = 0;
  average6 = 0;
  average7 = 0; 
  
  //takes populated temperature array created from NUMSAMPLES of each pin and then sums for each pin
  for (i=0; i< NUMSAMPLES; i++) 
  {
     average0 += samples0[i];
     average1 += samples1[i];
     average2 += samples2[i];
     average3 += samples3[i];
     average4 += samples4[i];
     average5 += samples5[i];
     average6 += samples6[i];
     average7 += samples7[i];         
  }
  
  //finds average reading of NUMSAMPLES per pin
  average0 /= NUMSAMPLES;
  average1 /= NUMSAMPLES;
  average2 /= NUMSAMPLES;
  average3 /= NUMSAMPLES;
  average4 /= NUMSAMPLES;
  average5 /= NUMSAMPLES;
  average6 /= NUMSAMPLES;
  average7 /= NUMSAMPLES;
 
  // convert the value to resistance
  average0 = 1023 / average0 - 1;
  average1 = 1023 / average1 - 1;
  average2 = 1023 / average2 - 1;
  average3 = 1023 / average3 - 1;
  average4 = 1023 / average4 - 1;
  average5 = 1023 / average5 - 1;
  average6 = 1023 / average6 - 1;
  average7 = 1023 / average7 - 1;  
  
  average0 = SERIESRESISTOR / average0;
  average1 = SERIESRESISTOR / average1;
  average2 = SERIESRESISTOR / average2;
  average3 = SERIESRESISTOR / average3;
  average4 = SERIESRESISTOR / average4;
  average5 = SERIESRESISTOR / average5;
  average6 = SERIESRESISTOR / average6;
  average7 = SERIESRESISTOR / average7;  
  

 //converts the resistance reading to C or F for each thermistor Average
  float steinhart0,steinhart1,steinhart2,steinhart3,steinhart4,steinhart5,steinhart6,steinhart7;
  float AVG_Stein;
  
  //Thermistor 0
  steinhart0 = average0 / THERMISTORNOMINAL;     // (R/Ro)
  steinhart0 = log(steinhart0);                  // ln(R/Ro)
  steinhart0 /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
  steinhart0 += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  steinhart0 = 1.0 / steinhart0;                 // Invert
  steinhart0 -= 273.15;                         // convert to C
  /*Conversion of Thermistor 0 to F*/
  steinhart0F = DallasTemperature::toFahrenheit(steinhart0);
  Serial.println("Thermistor0 in F: ");
  Serial.println(steinhart0F);
  /*End Conversion*/
  
  
  //thermistor 1
  steinhart1 = average1 / THERMISTORNOMINAL;     // (R/Ro)
  steinhart1 = log(steinhart1);                  // ln(R/Ro)
  steinhart1 /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
  steinhart1 += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  steinhart1 = 1.0 / steinhart1;                 // Invert
  steinhart1 -= 273.15;                         // convert to C

  /*Conversion of Thermistor 1 to F*/
  steinhart1F = DallasTemperature::toFahrenheit(steinhart1);
  Serial.println("Thermistor1 in F: ");
  Serial.println(steinhart1F);
  /*End Conversion*/

  //thermistor 2 
  steinhart2 = average2 / THERMISTORNOMINAL;     // (R/Ro)
  steinhart2 = log(steinhart2);                  // ln(R/Ro)
  steinhart2 /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
  steinhart2 += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  steinhart2 = 1.0 / steinhart2;                 // Invert
  steinhart2 -= 273.15;                         // convert to C
  
  /*Conversion of Thermistor 3 to F*/
  steinhart2F = DallasTemperature::toFahrenheit(steinhart2);
  Serial.println("Thermistor2 in F: ");
  Serial.println(steinhart2F);
  /*End Conversion*/
 
  
  //thermistor 3 
  steinhart3= average3 / THERMISTORNOMINAL;     // (R/Ro)
  steinhart3 = log(steinhart3);                  // ln(R/Ro)
  steinhart3 /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
  steinhart3 += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  steinhart3 = 1.0 / steinhart3;                 // Invert
  steinhart3 -= 273.15;                         // convert to C
  
  /*Conversion of Thermistor 3 to F*/
  steinhart3F = DallasTemperature::toFahrenheit(steinhart3);
  Serial.println("Thermistor3 in F: ");
  Serial.println(steinhart3F);
  /*End Conversion*/
  
  //thermistor 4 
  steinhart4= average4 / THERMISTORNOMINAL;     // (R/Ro)
  steinhart4 = log(steinhart4);                  // ln(R/Ro)
  steinhart4 /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
  steinhart4 += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  steinhart4 = 1.0 / steinhart4;                 // Invert
  steinhart4 -= 273.15;                         // convert to C
  
  /*Conversion of Thermistor 4 to F*/
  steinhart4F = DallasTemperature::toFahrenheit(steinhart4);
  Serial.println("Evap in F: ");
  Serial.println(steinhart4F);
  /*End Conversion*/
  
  //thermistor 5 
  steinhart5= average5 / THERMISTORNOMINAL;     // (R/Ro)
  steinhart5 = log(steinhart5);                  // ln(R/Ro)
  steinhart5 /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
  steinhart5 += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  steinhart5 = 1.0 / steinhart5;                 // Invert
  steinhart5 -= 273.15;                         // convert to C
  
  /*Conversion of Thermistor 5 to F*/
  steinhart5F = DallasTemperature::toFahrenheit(steinhart5);
  Serial.println("Condenser in F: ");
  Serial.println(steinhart5F);
  /*End Conversion*/ 
  
  //thermistor 6 
  steinhart6= average6 / THERMISTORNOMINAL;     // (R/Ro)
  steinhart6 = log(steinhart6);                  // ln(R/Ro)
  steinhart6 /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
  steinhart6 += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  steinhart6 = 1.0 / steinhart6;                 // Invert
  steinhart6 -= 273.15;                         // convert to C
  
  /*Conversion of Thermistor 6 to F*/
  steinhart6F = DallasTemperature::toFahrenheit(steinhart6);
  Serial.println("Evap Other Side in F: ");
  Serial.println(steinhart6F);
  /*End Conversion*/ 

//
//  //thermistor 7 
//  steinhart7= average7 / THERMISTORNOMINAL;     // (R/Ro)
//  steinhart7 = log(steinhart7);                  // ln(R/Ro)
//  steinhart7 /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
//  steinhart7 += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
//  steinhart7 = 1.0 / steinhart7;                 // Invert
//  steinhart7 -= 273.15;                         // convert to C
//  
//  /*Conversion of Thermistor 7 to F*/
//  steinhart7F = DallasTemperature::toFahrenheit(steinhart7);
//  Serial.println("Thermistor7 in F: ");
//  Serial.println(steinhart7F);
//  /*End Conversion*/


  evapTherm = (steinhart4F+steinhart6F)/2; //sets the evap thermistor reading to the 4th and 6th thermistor
  Serial.println("Average Evap Temp: ");
  Serial.println(evapTherm);
  
  condTherm = steinhart5F; // sets the condesnser thermistor reading to the 5th thermistor

  AVG_Stein = (steinhart0+steinhart1+steinhart2+steinhart3)/4; //averages body temperature readings 
  AVG_SteinF = DallasTemperature::toFahrenheit(AVG_Stein); // Converts average to F
  Serial.println("Average Thermistor Temp: ");
  Serial.println(AVG_SteinF);
  
  //Error checking functionality for thermistor failure
  if(steinhart0 <= (steinhart1-5) || steinhart0 >= (steinhart1+5)|| steinhart0 <= (steinhart2-5) || steinhart0 >= (steinhart2+5) || steinhart0 <= (steinhart3-5) || steinhart0 >= (steinhart3+5))
  {
   Serial.println(" A Thermistor may have failed... please consult repair manual") ;
  }
  
 
  dataString += steinhart0F;
  dataString += ";";
  dataString += steinhart1F;
  dataString += ";";
  dataString += steinhart2F;
  dataString += ";";
  dataString += steinhart3F;
  dataString += ";";
  dataString += AVG_SteinF;
  dataString += ";";
  
  /*Thermo readings*/
  // call sensors.requestTemperatures() to issue a global temperature 
  // request to all devices on the bus
  Serial.println(" ");
  Serial.println(" ");
  
  
  /*******************Start Heart Rate Reading*******************/
  Serial.println(heartRate);//remove after debugging
  Serial.println(";"); // remove after debuggi
  dataFile.print(heartRate);
  dataFile.print(";");
  delay(1000); //just here to slow down the checking to once a second
  /*********************End reading*****************************/
  
  /*end Thermo readings*/
  

  
  
 
  /*Compressor Control*/
    //run as long as this condenser temp is safe
  if(condTherm < Cond_safe){
    Serial.println("Condeser Temp is safe...running thermo loop...");
   //will check if the temperature is over 68 at first turn on start compressor
     delay(1000);
    if(evapTherm > 73)
    {
      Serial.println("Checking to see if cool turns on...");
      digitalWrite(controlPin, HIGH);   // turn the LED on (HIGH is the voltage level)
      Serial.println("Cooling...for a minimum of 31 seconds");
      delay(31000);
    }
    //if evap temp drops below 60 turn off 
      if(evapTherm <= 60)
      {
        Serial.println("Evap reached below 60F. System shut down until system warms up...");
        digitalWrite(controlPin, LOW);    // turn the LED off by making the voltage LOW
        Serial.println("Turned off...");
        delay(120000); //minimum off time
      
      if(steinhart4F <= 55) //checks to make sure frostbite condition doesnt occur under 55
      {
        Serial.println("Evap 1 temp too low...shutting down to avoid frostbite");
        digitalWrite(controlPin, LOW);
        delay(15000); //delays 15 seconds to allow warm up
      }
    }
  }
  else if(condTherm >= Cond_safe)
  {
    Serial.println("The Condenser temp is over 130F. The unit is shutting down until the condenser cools");
    digitalWrite(controlPin,LOW); // emergency shut off if the condenser temp gets too high.This temperature is directly related to pressure in the system
    
    Serial.println("Condenser shut down for atleast 2 min");
    delay(120000);//2 min off time
  }
  /*End Compressor Control*/
  




  // if the file is available, write to it:
  if (dataFile) {
    DateTime now = rtc.now();
    
    dataFile.print(now.year(), DEC);
    dataFile.print('/');
    dataFile.print(now.month(), DEC);
    dataFile.print('/');
    dataFile.print(now.day(), DEC);
    dataFile.print(' ');
    dataFile.print(now.hour(), DEC);
    dataFile.print(':');
    dataFile.print(now.minute(), DEC);
    dataFile.print(':');
    dataFile.print(now.second(), DEC);
    dataFile.print("; ");
    dataFile.print(dataString);
    dataFile.print(" ");//println
    dataFile.close();
    
    Serial.print("The time is: ");
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(' ');
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.print("; ");
    Serial.print("This is a datastring: " + dataString);
    
    
    if(steinhart0 <= (steinhart1-5) || steinhart0 >= (steinhart1+5)|| steinhart0 <= (steinhart2-5) || steinhart0 >= (steinhart2+5) || steinhart0 <= (steinhart3-5) || steinhart0 >= (steinhart3+5))
      {
         dataFile.println(" A Body Heat Thermistor may have failed... please consult repair manual") ;
      }
  }
  // print to the serial port too:
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening datalog.txt");
  }
  
  delay(1000);//wait for 1 sec
}

void setupHeartMonitor(int type){
  //setup the heartrate monitor
  Wire.begin();
  writeRegister(HRMI_I2C_ADDR, 0x53, type); // Configure the HRMI with the requested algorithm mode
}

int getHeartRate(){
  //get and return heart rate
  //returns 0 if we couldnt get the heart rate
  byte i2cRspArray[3]; // I2C response array
  i2cRspArray[2] = 0;

  writeRegister(HRMI_I2C_ADDR,  0x47, 0x1); // Request a set of heart rate values 

  if (hrmiGetData(127, 3, i2cRspArray)) {
    return i2cRspArray[2];
  }
  else{
    return 0;
  }
}

void writeRegister(int deviceAddress, byte address, byte val) {
  //I2C command to send data to a specific address on the device
  Wire.beginTransmission(deviceAddress); // start transmission to device 
  Wire.write(address);       // send register address
  Wire.write(val);         // send value to write
  Wire.endTransmission();     // end transmission
}

boolean hrmiGetData(byte addr, byte numBytes, byte* dataArray){
  //Get data from heart rate monitor and fill dataArray byte with responce
  //Returns true if it was able to get it, false if not
  Wire.requestFrom(addr, numBytes);
  if (Wire.available()) {

    for (int i=0; i<numBytes; i++){
      dataArray[i] = Wire.read();
    }

    return true;
  }
  else{
    return false;
  }
}
