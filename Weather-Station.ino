/****************************************************************
Arduino Weather Station (Ethernet)

Arduino Ethernet shield on an Arduino Uno: Weather station that sends data to Weather Underground for storage and viewing.

By Dan Fein

Sensors from Adafruit, see their product pages for those libraries.

Notes:
This project has options to conserve battery power. To save power use lpDelay to rest for awhile, wake, send data and rest again.

You will need to sign up for a free account at wunderground.com, to get your pass
When you register a station you will get an ID (If you send data you get ad free membership free)

Sign up at http://www.wunderground.com/personal-weather-station/signup.asp

Wunderground wants UTC Zulu, not local time, if your RTC is local, offset it in code.
Wunderground Upload guidelines: http://wiki.wunderground.com/index.php/PWS_-_Upload_Protocol

****************************************************************/

int DEBUG = 0;      // DEBUG counter; if set to 1, will write values back via serial

#include <Wire.h>
#include <SPI.h>
#include <Ethernet.h>		//Ethernet
#include <EthernetUdp.h>	//Ethernet
#include "DHT.h"                //Humidity
#include <Adafruit_BMP085.h>    //Barometer
#include "Adafruit_SI1145.h"    //UV sensor
//#include "RTClib.h"           //Real time clock -- If used

//#include <io.h> //for the sleepies -- Needed if using lpdelay (sleeping when using battery)

// Pins
#define DHTPIN          3   // DHT 22  (AM2302)
                            // The UV sensor and Barometer are on i2C pins

// Constants
#define DHTTYPE DHT22       // DHT 22 (AM2302) -- Humidity

// assign a MAC address for the ethernet controller -- Newer boards will have this on a sticker.
// fill in your address here:
byte mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
// assign an IP address for the controller:
//IPAddress ip(192,168,1,20);
//IPAddress gateway(192,168,1,1);	
//IPAddress subnet(255, 255, 255, 0);
EthernetClient client;
unsigned int localPort = 8888;

char SERVER[] = "rtupdate.wunderground.com";           // Realtime update server - RapidFire
//char SERVER [] = "weatherstation.wunderground.com";  // Standard server
char WEBPAGE [] = "GET /weatherstation/updateweatherstation.php?";
char ID [] = "xxxx";
char PASSWORD [] = "xxxx";

// Global Variables
//RTC_DS1307 rtc;                       // Hardware RTC time -- If used
DHT dht(DHTPIN, DHTTYPE);               // DHT 22  (AM2302) -- Humidity
Adafruit_BMP085 bmp;                    // BMP Pressure Sensor
Adafruit_SI1145 uv = Adafruit_SI1145(); // UV Sensor
unsigned int connections = 0;           // number of connections
unsigned int timeout = 30000;           // Milliseconds -- 1000 = 1 Second


// Setup runs once at power on or reset
void setup(void){
  //Turn everything on
  Serial.begin(38400);
  Wire.begin();    // Needed for I2C bus (the UV and Barometer are on I2C)
  //rtc.begin();     //Hardware rtc
  bmp.begin();     //Pressure sensor
  dht.begin();     //Humidity Sensor
  
  // Turn the internet on
  Serial.print(F("\nInitializing..."));
  if (Ethernet.begin(mac) ) {
    Serial.println("Initialization complete");
 	 } 
  else {
    Serial.println("Something went wrong during ethernet startup!");
  	} 

} // End of Setup


// Loop runs continuously

void loop(void){  
  //Lets see what time the RTC is set at! -- If RTC is used
  //DateTime now = rtc.now();

  //Get sensor data
  float tempc = bmp.readTemperature(); //Can read temp from bmp or dht sensors
  float tempf = (tempc * 9.0)/ 5.0 + 32.0; //was dht.readTemperature, need to convert native C to F
  float humidity = dht.readHumidity(); 
  float baromin = bmp.readPressure()* 0.0002953;// Calc for converting Pa to inHg (wunderground)
  float dewptf = (dewPoint(tempf, dht.readHumidity())); //Dew point calc(wunderground) //replaced dht.readtemp with converted temp
  float UVindex = uv.readUV();
  // the index is multiplied by 100 so to get the integer index, divide by 100!
        UVindex /= 100.0; 
                            
  if (DEBUG) {   
  // Debug, or you can sit up all night watching it.
  Serial.println("+++++++++++++++++++++++++");
  /*
  //If you are using Real Time Clock (RTC)
  Serial.println("RTC TIME ");
  Serial.print("&dateutc=");
  Serial.print(now.year());
  Serial.print("-");
  Serial.print(now.month());
  Serial.print("-");
  Serial.print(now.day());
  Serial.println("+");
  Serial.print(now.hour());
  Serial.print(":");
  Serial.print(now.minute());
  Serial.print(":");
  Serial.println(now.second());
  */
  Serial.print("temp= ");
  Serial.print(tempf);
  Serial.println(" *F");
  Serial.print("baro= ");
  Serial.print(baromin);
  Serial.println(" inHg");
  Serial.print("dew point= ");
  Serial.println(dewptf);
  Serial.print("humidity= ");
  Serial.println(humidity);
  Serial.print("UV: ");  
  Serial.println(UVindex);
  }//End debug loop
 
 //Send data to Weather Underground
 if (client.connect(SERVER, 80)) { 
    if (DEBUG) {    
      Serial.println("Sending DATA ");
      }
    // Ship it!
    client.print(WEBPAGE); 
    client.print("ID=");
    client.print(ID);
    client.print("&PASSWORD=");
    client.print(PASSWORD);
    client.print("&dateutc=");
    client.print("now");    //can use instead of RTC if sending in real time
    /*
    //If you are using Real Time Clock (RTC)
    client.print(now.year());
    client.print("-");
    client.print(now.month());
    client.print("-");
    client.print(now.day());
    client.print("+");
    client.print(now.hour()+8);// YOU MUST Add 8 hours -for pacific time- to get back to UTC or Wunderground wont show RAPID FIRE
    client.print("%3A");
    client.print(now.minute());
    client.print("%3A");
    client.print(now.second());
    */
    client.print("&tempf=");
    client.print(tempf);
    client.print("&baromin=");
    client.print(baromin);
    client.print("&dewptf=");
    client.print(dewptf);
    client.print("&humidity=");
    client.print(humidity);
    client.print("&uv=");
    client.print(UVindex);
    //client.print("&action=updateraw");//Standard update
    client.print("&softwaretype=Arduino%20UNO%20version1&action=updateraw&realtime=1&rtfreq=2.5");//Rapid Fire
    client.print(" HTTP/1.0\r\n");
  	client.print("Accept: text/html\r\n");
  	client.print("Host: ");
  	client.print(SERVER);
  	client.print("\r\n\r\n");
    client.println();
    
    if (DEBUG) {   
      Serial.println("Upload complete");
      }
   }     
    else {
      if (DEBUG) { Serial.println(F("Connection failed")); }
      return;
      }
    
    delay(2500); // --If plugged in send every 2.5 seconds  
    //lpDelay(1200); // Low Power Delay. Value of 4=1sec, 40=10sec, 1200=5min --If battery, send every 5 min.

}  // End loop


/****************************************************************

  Function Fun Time

****************************************************************/

double dewPoint(double tempf, double humidity)
{
  double A0= 373.15/(273.15 + tempf);
  double SUM = -7.90298 * (A0-1);
  SUM += 5.02808 * log10(A0);
  SUM += -1.3816e-7 * (pow(10, (11.344*(1-1/A0)))-1) ;
  SUM += 8.1328e-3 * (pow(10,(-3.49149*(A0-1)))-1) ;
  SUM += log10(1013.246);
  double VP = pow(10, SUM-3) * humidity;
  double T = log(VP/0.61078);   
  return (241.88 * T) / (17.558-T);
}

 // Low Power Delay.  Drops the system clock to its lowest setting and sleeps for 256*quarterSeconds milliseconds.
 // ie: value of 4=1sec    20=5sec          
int lpDelay(int quarterSeconds) {
  int oldClkPr = CLKPR;   // save old system clock prescale
  CLKPR = 0x80;           // Tell the AtMega we want to change the system clock
  CLKPR = 0x08;           // 1/256 prescaler = 60KHz for a 16MHz crystal
  delay(quarterSeconds);  // since the clock is slowed way down, delay(n) now acts like delay(n*256)
  CLKPR = 0x80;           // Tell the AtMega we want to change the system clock
  CLKPR = oldClkPr;       // Restore old system clock prescale
}

/****************************************************************

  End of Program

****************************************************************/