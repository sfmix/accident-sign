

/*
    SFMIX "last day since" accident sign
*/


/* TODO
  - use an RTC, for not using NTP sync upon boot
*/

#include <ETH.h>    // ESP32 ethernet
#include <Preferences.h>
#include <ezTime.h>
#include <HardwareSerial.h>
#include <AlphaFive-Control.h>

/* Local variables */
static bool USE_WIFI = false;
const char* ssid = "Chateau 64th - Guests";
const char* pass = "notmypass";

#define HOSTNAME "sfmix-clock"
String LOCAL_NTP_SERVER = "100.64.42.1";

#define PIR_DOUT 2
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

  Serial.print("ESP date: ");
  Serial.println(UTC.dateTime());

  pinMode(PIR_DOUT, INPUT);
  ESPSerial.begin(19200, SERIAL_8N1, 13, 12);
  FirstClock.writeWord("BOOT");
  
  WiFi.setHostname(HOSTNAME); // DHCP client hostname
  WiFi.onEvent(WiFiEvent);
  btStop();

  if (USE_WIFI) {
    WiFi.begin(ssid, pass);

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
  } else if (!USE_WIFI) {
    ETH.begin();
  }

  FirstClock.writeWord("NET");
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

  FirstClock.clear();
 
  setServer(LOCAL_NTP_SERVER);
  setDebug(INFO);
  
  // waitForSync(15);
}

void loop()
{
  events(); // ezTime logging

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
  }

  delay(500);
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
