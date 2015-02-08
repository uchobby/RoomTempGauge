#include <Servo.h>
#include <OneWire.h>

Servo servoGauge;  // create servo object to control a servo 

boolean deviceFound=false;

byte addr[8];
byte data[12];

const int DS12B20_ID = 0x28;

OneWire  device(2);  // on pin 2 (a 4.7K resistor is necessary)

boolean DS18B20Find(byte addr[8]){
  boolean result=false;
  
  //Check the bus for a device
  if (device.search(addr)) {  //Did we find a device?
    //Check the CRC in case we have data problems. 
    if (OneWire::crc8(addr, 7) == addr[7]) {
      //Check the device ID byte
      if (addr[0]==DS12B20_ID) {
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

void DS18B20StartConversion(){
    device.reset();
    device.select(addr);
    device.write(0x44, 1);        // start conversion, with parasite power on at the end
}

void DS18B20ReadData(){
    deviceFound = device.reset();
    device.select(addr);    
    device.write(0xBE);         // Read Scratchpad
 
    for (int i = 0; i < 9; i++) {           // we need 9 bytes
      data[i] = device.read();
    }
}

float DS18B20GetTemperature() {  //return temperature in C
  float result;
  
  DS18B20ReadData();

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  byte cfg = (data[4] & 0x60);
  // at lower res, the low bits are undefined, so let's zero them
  if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
  else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
  else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
  //// default is 12 bit resolution, 750 ms conversion time
  result = (float)raw / 16.0;
  
  return(result);
}

void setup(void) {
  Serial.begin(115200);
  servoGauge.attach(5);  // attaches the servo on pin 5 to the servo object 
  deviceFound=DS18B20Find(addr);
}

void loop(void) {
  float celsius, fahrenheit;

  if(deviceFound) {
    DS18B20StartConversion();
  
    delay(1000);     // maybe 750ms is enough, maybe not
      
    celsius=DS18B20GetTemperature();

    fahrenheit = celsius * 1.8 + 32.0;
    Serial.print("  Temp = ");
    Serial.print(fahrenheit);
    Serial.println(" F");
  
    int gaugeMicroseconds=map(fahrenheit*100,6900,7900,2000,1000);
    gaugeMicroseconds=constrain(gaugeMicroseconds,1000,2000);
    servoGauge.writeMicroseconds(gaugeMicroseconds);

    Serial.print(" ServoGauge:");
    Serial.print(gaugeMicroseconds);
    Serial.println(" uS");
  }
}
