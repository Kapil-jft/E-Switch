#include "SoftwareSerial.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <Wire.h>
#include "RTClib.h"
#include <time.h>


#define DS1307_ADD 0x68

RTC_DS1307 rtc;
DateTime now;
DateTime Ton(2018, 8, 12, 15, 25, 00);
DateTime Toff(2018, 8, 12, 15, 30, 20);


char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};     //Name of Weekdays


void cleareeprom();               //To clear EEPROM
char config_read(void);           //To Read From Memory
void config_write(void);          //To write Memory
void handle_led();                //Handle led function
void connectSTA(void);             //Connect To Station Mode
void Trigger_Relay();
const char* ssid = "JFT";          //Name of AP
const char* password = "123456789";      //Password to connecet when AP


char * un = 0;
char * pwd = 0;
String uname;
String pswd;
char* ipadd;
char strssid[50];
char strpwd[70];

int i;
int lenn;
String content = "<html><body><form action='/led'  +23method='POST'><br>SSID:<input type='text' name='USERNAME' placeholder='ssid'><br>PASSWORD:<input type='password' name='PASSWORD' placeholder='password'><br><input type='submit' name='CONNECT' value='Submit'></form>................<br>................</a></body></html>";
String form = "<form action='/relay'><input type='text' name='state' value='1' checked>On<input type='text' name='state' value='0'>Off<input type='submit' value='Submit'></form>";



ESP8266WebServer server(80);

const int led = 13;     //GPIO 13 of ESP8266 Module


void setup()
{
  delay(2000);
  pinMode(13, OUTPUT);
  Serial.begin(115200);
  Serial.print("Serial Begin stared....");
  //WiFi.softAP(ssid, password);
  EEPROM.begin(512);
  //cleareeprom();
  //config_write();
  Wire.begin(4, 5);
  config_read();
  rtc.begin();
  connectSTA();
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  Serial.print("\nConnected to ");
  Serial.print("IP address: ");
  IPAddress localip = WiFi.localIP();
  lenn = sizeof(localip);
  Serial.println(lenn);
  Serial.println(localip);
  Serial.println(WiFi.macAddress());
}



void connectSTA()
{
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(un, pwd);
  Serial.println(un);
  Serial.println(pwd);
  Serial.println("Connecting");
  int timeout = millis();
  int timeoutcounter;
  timeoutcounter = timeout;
  while (WiFi.status() != WL_CONNECTED  && !(timeoutcounter - timeout >= 30000))
  {
    delay(500);
    Serial.println(un);
    Serial.println(pwd);
    timeoutcounter = millis();
  }
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.macAddress());

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected...\nAttempting Accesspoint...");
    WiFi.softAP(ssid, password);
    server.on("/", []() {
      server.send(200, "text/html", content);
    });
    //configwrite();
    server.on("/led", handle_led);
    server.begin();
    Serial.println("Access Point Created");
    Serial.println(WiFi.localIP());
    // Start the server
  }
}

void handle_led()
{
  uname = server.arg("USERNAME");    //uname is ssid from server to connect
  pswd =  server.arg("PASSWORD");    //password from server
  memset(strssid, 0, 50);
  lenn = uname.length();
  for (i = 0; i < lenn; i++) {
    strssid[i] = uname[i];
  }
  strssid[lenn] = '\0';
  un = &strssid[0];
  memset(strpwd, 0, 50);
  lenn = pswd.length();
  for (i = 0; i < lenn; i++) {
    strpwd[i] = pswd[i];
  }
  strpwd[lenn] = '\0';
  pwd = &strpwd[0];
  Serial.println(un);
  Serial.println(pwd);
  config_write();
  String str = "<html><body><a>Username and Password have been saved !</a></body></html>";
  server.send(200, "text/plain", String("Username and password has been saved") + uname + pswd);


  connectSTA();

}



void loop()
{
    delay(1000);                      // Wait for a second
   server.handleClient();
  now = rtc.now();
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" (");
  Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
  Serial.print(") ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
  Serial.println("TIME");
  Serial.println("Set Timer....");
  Trigger_Relay();
}

void Trigger_Relay() {
  int timenow = (now.hour() * 60 * 60) + (now.minute() * 60) + now.second();
  int tonsec = (Ton.hour() * 60 * 60) + (Ton.minute() * 60) + Ton.second();
  int toffsec = (Toff.hour() * 60 * 60) + (Toff.minute() * 60) + Toff.second();

  if (timenow > tonsec && timenow < toffsec) {
    Serial.println("Relay ON");
    digitalWrite(13, HIGH);
  } else {
    Serial.println("Relay OFF");
     digitalWrite(13, LOW);
  }
  //if ((RELAY_STATE == 0) &&                             ){// (now.hour() == Ton.hour()) && (now.minute() == Ton.minute()) && (now.second() == Ton.second())) {
  //    RELAY_STATE = 1;
  //    //digitalWrite(13, HIGH);  // Turn the LED off by making the voltage HIGH
  //  }
  //  if ((RELAY_STATE == 1) && (now.hour() == Toff.hour()) && (now.minute() == Toff.minute()) && (now.second() <= Toff.second())) {
  //    RELAY_STATE = 0;
  //    //    digitalWrite(13, LOW);
  //  }
  //  if (RELAY_STATE == 1) {
  //    Serial.println("Relay is ON");
  //    digitalWrite(13, HIGH);
  //  } else {
  //    Serial.println("Relay is OFF");
  //    digitalWrite(13, LOW);
  //  }
}


void config_write(void) {
  Serial.println("Clearing eeprom");
  for (int i = 0; i < 96; ++i) {
    EEPROM.write(i, 0);
  }
  Serial.println("writing eeprom ssid:");
  for (int i = 0; i < strlen(un); ++i)
  {
    EEPROM.write(i, un[i]);
    Serial.print("Wrote: ");
    Serial.println(un[i]);
  }
  Serial.println("writing eeprom pass:");
  for (int i = 0; i < strlen(pwd); ++i)
  {
    EEPROM.write(32 + i, pwd[i]);
    Serial.print("Wrote: ");
    Serial.println(pwd[i]);
  }
  EEPROM.commit();
}

char config_read(void) {

  delay(2000);
  memset(strssid, 0, 50);
  memset(strpwd, 0, 70);
  int j = 0;
  for (i = 0; i < 32; ++i) {
    strssid[j] = EEPROM.read(i);
    if (strssid[j] == '\0')break;
    j++;
  }
  Serial.println("Checking from EEPROM");
  j = 0;
  for (i = 32; i <= 95; ++i) {
    strpwd[j] = EEPROM.read(i);
    if (strpwd[j] == '\0')break;
    j++;
    //Serial.println(i);

  }
  Serial.print("\nUNAME:"); Serial.print(strssid);
  Serial.print("\nPSWD:"); Serial.println(strpwd);
  un = &strssid[0];
  pwd = &strpwd[0];

  return 0;
}


void cleareeprom() {
  Serial.println("Clearing eeprom");
  for (int i = 0; i < 96; ++i) {
    EEPROM.write(i, 0);
  }
}




