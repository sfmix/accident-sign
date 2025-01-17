

/*
    SFMIX "last day since" accident sign
*/


/* Load libaries */
#include <ETH.h>    // ESP32 ethernet
#include <Preferences.h>
#include <ezTime.h>
#include <HardwareSerial.h>
#include <AlphaFive-Control.h>
#include <Wire.h>
#include <RTClib.h> // i2c addr 0x68

/* Local variables */
#define HOSTNAME "sfmix-clock"

RTC_DS3231 rtc;
#define PIR_DOUT 36
HardwareSerial ESPSerial(1);
AlphaClockFive FirstClock(&ESPSerial);
time_t EventDate = 1573673520;

/* Setup libraries */
Preferences preferences;
static bool eth_connected = false;
char padded_days_since[5];
int ProgressBootCounter = 0;

void setup()
{
  pinMode(PIR_DOUT, INPUT);
  ESPSerial.begin(19200, SERIAL_8N1, 13, 12);
  FirstClock.writeWord("BOOT");
  
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  Serial.print("Compiled date: ");
  Serial.print(__DATE__);
  Serial.print(" ");
  Serial.println (__TIME__);

  Serial.print("Arduino: v");
  Serial.println(ARDUINO,DEC);

  Serial.print("ESP32 SDK: ");
  Serial.println(ESP.getSdkVersion());

  Serial.print("ESP current date: ");
  Serial.println(UTC.dateTime());

  rtc.begin();
  if ( rtc.getTemperature() > 65 )
  {
    Serial.println("RTC doesn't seem connected");
  } else {
    Serial.print("RTC Temperature: ");
    Serial.print(rtc.getTemperature());
    Serial.println(" °C");

    DateTime RTCnow = rtc.now();
    Serial.print("RTC boot value: ");
    time_t RTCnow_t = RTCnow.unixtime();
    Serial.println(RTCnow_t);

    if ( RTCnow_t > compileTime() )
    {
      Serial.println("RTC seems reasonable, > compiled time");
    } else {
      Serial.println("RTC out of sync");
    }    
  }
   
  FirstClock.writeWord("DHCP");  
  WiFi.setHostname(HOSTNAME); // DHCP client hostname
  WiFi.onEvent(WiFiEvent);
  btStop();
  ETH.begin();

  while ( !eth_connected ) {
    delay ( 1000 );
    Serial.print ( "." );
    if (ProgressBootCounter % 2 == 0) {
      FirstClock.writeWord(". . .");
    } else {
      FirstClock.writeWord(" . . ");
    }
    ProgressBootCounter = ProgressBootCounter + 1;
  }

  setDebug(INFO);
  events(); // log ezTime events
  rtc.adjust(UTC.now());
}

void loop()
{
  unsigned long epoch;
  epoch = UTC.now();
  int days_since = (epoch - EventDate) / 86400;
  sprintf(padded_days_since, "%05d", days_since);

  boolean MotionState = readMotionSensor();
  if (MotionState) {
    Serial.print("Days since: ");
    Serial.println(padded_days_since);
    FirstClock.writeWord(padded_days_since);
    delay(1500);
    FirstClock.writeWord(" DAYS");
    delay(500);
  } else {
    FirstClock.clear();
    // Serial.println("No motion");
    delay(500);
  }

  delay(500);

  if ( eth_connected ) 
  {
    Serial.print("Setting RTC time: ");
    Serial.println(UTC.dateTime());
    rtc.adjust(UTC.now());
  }
}

void bringupnetwork()
{
  
  // waitForSync(25);
  
  FirstClock.writeWord(" NTP ");
  Serial.print("NTP date: ");
  Serial.println(UTC.dateTime());

  Serial.print("Setting RTC time: ");
  FirstClock.writeWord("RTCSY");
  Serial.println(UTC.dateTime());
  rtc.adjust(UTC.now());
}


boolean readMotionSensor()
{
  int motionSensor = digitalRead(PIR_DOUT);
  
  if (motionSensor == HIGH) {
    return true;
  } else {
    return false;
  }
}

void WiFiEvent(WiFiEvent_t event)
{
  switch (event) {
    case SYSTEM_EVENT_ETH_START:
      Serial.println("ETH Started");
      ETH.setHostname(HOSTNAME);
      break;
    case SYSTEM_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      break;
    case SYSTEM_EVENT_ETH_GOT_IP:
      Serial.print("ETH MAC: ");
      Serial.print(ETH.macAddress());
      Serial.print(", IPv4: ");
      Serial.print(ETH.localIP());
      if (ETH.fullDuplex()) {
        Serial.print(", FULL_DUPLEX");
      }
      Serial.print(", ");
      Serial.print(ETH.linkSpeed());
      Serial.println("Mbps");
      eth_connected = true;
      break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      eth_connected = false;
      break;
    case SYSTEM_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      eth_connected = false;
      break;
    default:
      break;
  }
}
