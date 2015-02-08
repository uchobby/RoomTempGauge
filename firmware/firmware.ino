//uCHobby code described here:http://www.uchobby.com/index.php/2015/02/07/room-temp-on-custom-gauge/

#include <Servo.h>
#include <OneWire.h>

//Globals used in this program.  Globals are generally a bad idea. I will fix this up later though. :)
Servo servoGauge;  // create servo object to drive the Gauge 
const int SERVO_MAXUS=2000;
const int SERVO_MINUS=1000;

boolean deviceFound=false;  //A flag to show we do have a DS12B20 to read from

byte deviceAddress[8];  //Device address
byte deviceData[12]; //Data we read from the device.

const int DS12B20_ID = 0x28;  //ID code for the DS12B20, we check this...

OneWire  device(2);  // OneWire lib used for comunications on digital 2 (a 4.7K pull up resistor is necessary)

boolean DS18B20Find(byte addr[8]);
void DS18B20StartConversion();
void DS18B20ReadData(); 
float DS18B20GetTemperature();


void setup(void) {
  Serial.begin(115200);  //Serial data baudrate for debug information
  servoGauge.attach(5);  // attaches the servo on pin 5 to the servo object 
  deviceFound=DS18B20Find(deviceData);  //Search for our temp sensor
}

void loop(void) {
  float celsius, fahrenheit;

  if(deviceFound) {  //Do we have a device to work with?
    DS18B20StartConversion();  //Start a temperature conversion
  
    delay(1000);     // maybe 750ms is enough, maybe not
      
    celsius=DS18B20GetTemperature();  //Grab the temperature, it's in SI units.

    fahrenheit = celsius * 1.8 + 32.0;  //converter it to F cause we like that in the US.
    Serial.print("  Temp = ");  //Print out the result on Debug
    Serial.print(fahrenheit);
    Serial.println(" F");
  
    int gaugeMicroseconds=map(fahrenheit*100,6900,7900,SERVO_MAXUS,SERVO_MINUS); //Map function to do the scale conversions
    gaugeMicroseconds=constrain(gaugeMicroseconds,1000,2000); //Limit the microseconds to what a servo can handle.
    servoGauge.writeMicroseconds(gaugeMicroseconds); //Write out the value to move the gauge.

    Serial.print(" ServoGauge:");    //Print the gauge setting for debug.
    Serial.print(gaugeMicroseconds);
    Serial.println(" uS");
  }
}

//Find a device on the one wire bus. Return true if found. Pass the address array so it can be filled in.
boolean DS18B20Find(byte addr[8]){  
  boolean result=false;
  
  //Check the bus for a device
  if (device.search(deviceAddress)) {  //Did we find a device?
    //Check the CRC in case we have data problems. 
    if (OneWire::crc8(deviceAddress, 7) == deviceAddress[7]) {
      //Check the device ID byte
      if (deviceAddress[0]==DS12B20_ID) {
        result=true; //We have what we are looking for.
      }
      else { //device not a DS12B20
        Serial.println("Device is not a DS18B20");
      }
    }
    else { //CRC Fails
      Serial.println("CRC is not valid!");
    }
  }
  
  //Did we find a device?
  if(!result) { //device was not found found or had an error
    device.reset_search();      //reset for another try.
  }
  
  return(result);
}  

//Trigger the temperature conversion on the device.
void DS18B20StartConversion(){
    device.reset();               //Do a bus reset to get devices ready for addressing
    device.select(deviceAddress);          //Address our device
    device.write(0x44, 1);        //Start conversion, with parasite power on at the end
}

//Read the scratchpad memory from the device.
void DS18B20ReadData(){  
    device.reset();               //Do a bus reset to get devices ready for addressing
    device.select(deviceAddress); //Address our device
    device.write(0xBE);           // Read Scratchpad
 
    for (int i = 0; i < 9; i++) {    // we need 9 bytes
      deviceData[i] = device.read(); //Store each read byte into our data array
    }
}

float DS18B20GetTemperature() {  //return temperature in C
  float result;
  
  DS18B20ReadData();

  // Convert the data to actual temperature
  int raw = (deviceData[1] << 8) | deviceData[0];
  byte cfg = (deviceData[4] & 0x60);
  // at lower res, the low bits are undefined, so let's zero them
  if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
  else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
  else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
  //// default is 12 bit resolution, 750 ms conversion time
  result = (float)raw / 16.0;
  
  return(result);
}


