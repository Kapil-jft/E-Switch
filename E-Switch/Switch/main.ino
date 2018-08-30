/*-------------------------------------------------------------------------------------------------
 * This project "Smart Power Socket" using WiFi 
 */
#include "SoftwareSerial.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <EEPROM.h>
#include <Wire.h>
#include "RTClib.h"
#include <time.h>
#include "PubSubClient.h"


#define DS1307_ADD 0x68

RTC_DS1307 rtc;
DateTime now;
DateTime Ton(2018, 8, 12, 15, 25, 00);
DateTime Toff(2018, 8, 12, 15, 30, 20);

const char* mqttServer = "m12.cloudmqtt.com";
const int mqttPort = 13318;
const char* mqttUser = "ekxwalwu";
const char* mqttPassword = "mrdxsg9rjKSt";

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};     //Name of Weekdays

/*----------------Private Decleration---------------*/
void callback(char* topic, byte* payload, unsigned int length);
void cleareeprom();               //To clear EEPROM
char config_read(void);           //To Read From Memory
void config_write(void);          //To write Memory
void station_mode_config();       //Handle led function
void connectSTA(void);            //Connect To Station Mode
void Trigger_Relay();
void mqtt();
void registermyself();

/*------------SSID and Password for access point mode-----------*/
const char* ap_ssid = "JFT";          //Name of AP
const char* password = "123456789";      //Password to connecet when AP

/*-------------Private Global variable---------------*/
char * un = 0;
char * pwd = 0;
char* usrid = 0;
String ssid;
String pswd;
String userid;
char* ipadd;
char strssid[50];
char strpwd[70];
char struserid[50];

int i;
int connectstaflow = 0;
int modeflag = 0;
int lenn;
String content = "<html><body><form action='/station'  +23method='POST'><br>SSID:<input type='text' name='SSID' placeholder='ssid'><br>PASSWORD:<input type='password' name='PASSWORD' placeholder='password'><br>UserID:<input type='userid' name='USERID' placeholder='UserID'><br><input type='submit' name='CONNECT' value='Submit'></form>................<br>................</a></body></html>";
String form = "<form action='/relay'><input type='text' name='state' value='1' checked>On<input type='text' name='state' value='0'>Off<input type='submit' value='Submit'></form>";

//Create Wifi Client
//WiFiClient wificlient;          /*Wifi Client*/
WiFiClient espClient;
PubSubClient client(espClient);
 


ESP8266WebServer server(80);

const int led = 13;     //GPIO 13 of ESP8266 Module

void setup()
{
  delay(2000);
  pinMode(13, OUTPUT);
  Serial.begin(74880);
  Serial.print("Serial Begin stared....");
  //WiFi.softAP(ssid, password);
  EEPROM.begin(512);
  cleareeprom();
  //config_write();
  Wire.begin(4,5);         /*Configure GPIO_4 as SDA,GPIO_5 as SCL of RTC*/
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


void connectSTA(){
  WiFi.disconnect();      
  WiFi.mode(WIFI_STA);
  WiFi.begin(un, pwd);
  Serial.println(un);
  Serial.println(pwd);
  Serial.println("Connecting");
  int timeout = millis();
  int timeoutcounter;

  timeoutcounter = timeout;
  while (WiFi.status() != WL_CONNECTED  && !(timeoutcounter - timeout >= 20000))
  {
    
    delay(500);
    Serial.print(".");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

    //Serial.println(pwd);
    timeoutcounter = millis();
  }
  if(connectstaflow == 0 && WiFi.status() == WL_CONNECTED){
    modeflag = 0;
    mqtt();
    }
  else if(connectstaflow == 1 && WiFi.status() == WL_CONNECTED){
    modeflag = 0;
    registermyself();
    mqtt();
  }

  
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.macAddress());

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi not connected...\nAttempting Accesspoint...");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    WiFi.softAP(ap_ssid, password);
    server.on("/ap", []() { server.send(200, "text/html", content); } ) ;
    //configwrite();
    server.on("/station", station_mode_config);
    server.begin();
    Serial.println("Access Point Created");
    Serial.println(WiFi.localIP());
    modeflag = 1;
    }
}

/*--------------------Handler Function-------------------*/
void station_mode_config(){
  ssid = server.arg("SSID");    //uname is ssid from server to connect
  pswd =  server.arg("PASSWORD");    //password from server
  userid =  server.arg("USERID");    //User ID from server
  Serial.println(userid);
  userid.remove((userid.length())-1); 
  userid.remove(0,1);
  Serial.println("USER ID EXTRACTED IS : "+userid);
  memset(strssid, 0, 50);
  lenn = ssid.length();
  for (i = 0; i < lenn; i++){
    strssid[i] = ssid[i];
  }
  strssid[lenn] = '\0';
  un = &strssid[0];
  
  memset(strpwd, 0, 50);
  lenn = pswd.length();
  for (i = 0; i < lenn; i++){
    strpwd[i] = pswd[i];
  }
  strpwd[lenn] = '\0';  
  pwd = &strpwd[0];
  
  memset(struserid, 0, 50);
  lenn = userid.length();
  for (i = 0; i < lenn; i++){
    struserid[i] = userid[i];
  }
  struserid[lenn] = '\0';  
  usrid = &struserid[0];
  Serial.println(un);
  Serial.println(pwd);
  Serial.println(usrid);
  
  config_write();
  String str = "<html><body><a>Username and Password have been saved !</a></body></html>";
  server.send(200, "text/plain", String("Username and password has been saved") + ssid + pswd + userid);

  connectstaflow = 1;
  connectSTA();

}



void loop()
{
  String postraw;
  String payload;
  
  delay(1000);                      // Wait for a second
  Serial.println(WiFi.macAddress());  
  if(modeflag == 1){
    server.handleClient();
  }else{
    client.loop();
  }
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

  Trigger_Relay();

}



void Trigger_Relay() 
{
  /*Convert the time in a single unnit "Second"*/
  int timenow = (now.hour() * 60 * 60) + (now.minute() * 60) + now.second();
  int tonsec = (Ton.hour() * 60 * 60) + (Ton.minute() * 60) + Ton.second();
  int toffsec = (Toff.hour() * 60 * 60) + (Toff.minute() * 60) + Toff.second();

/*If the timer match then switch ON the relay*/

  if (timenow > tonsec && timenow < toffsec) {
    Serial.println("Relay ON");
    digitalWrite(13, HIGH);
  } else {
    Serial.println("Relay OFF");
     digitalWrite(13, LOW);
  }

}

/*This function write username and password to EEPROM*/
void config_write(void) {
  /*******First We need to clear EEPROM********/
  Serial.println("Clearing eeprom");
  for (int i = 0; i < 96; ++i) {
    EEPROM.write(i, 0);
  }
  Serial.println("writing eeprom ssid:");
  for (int i = 0; i < strlen(un); ++i)
  {
    EEPROM.write(i, un[i]);
    //Serial.print("Wrote: ");
    //Serial.println(un[i]);
  }
  Serial.println("writing eeprom pass:");
  for (int i = 0; i < strlen(pwd); ++i)
  {
    EEPROM.write(32 + i, pwd[i]);
    //Serial.print("Wrote: ");
    //Serial.println(pwd[i]);
  }
  Serial.println("writing userid:");
  for (int i = 0; i < strlen(usrid); ++i)
  {
    EEPROM.write(64 + i, usrid[i]);
    //Serial.print("Wrote: ");
    //Serial.println(usrid[i]);
  }  
  EEPROM.commit();
}


/*Read username and passowrd from EEPROM*/
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
  j = 0;
  for (i = 64; i <= 95; ++i) {
    struserid[j] = EEPROM.read(i);
    if (struserid[j] == '\0')break;
    j++;
    //Serial.println(i);

  }  
  Serial.print("\nUNAME:"); Serial.print(strssid);
  Serial.print("\nPSWD:"); Serial.println(strpwd);
  Serial.print("USERID:"); Serial.println(struserid);
  un = &strssid[0];
  pwd = &strpwd[0];
  usrid = &struserid[0];
  

  return 0;
}

/*Clearing EEPROM before write something*/
void cleareeprom() {
  Serial.println("Clearing eeprom");
  for (int i = 0; i < 96; ++i) {
    EEPROM.write(i, 0);
  }
}

void registermyself(){
  String postraw;
  String payload;
  //Create a HTTP client
  HTTPClient http;
  http.begin("http://switch.jellyfishtechnologies.com/registerDevice");
  
  postraw = "{\"deviceId\":\""
  +WiFi.macAddress()+
  "\",\"userId\":\""
  +userid+
  "\"}";
  Serial.println(postraw);
  int httpCode = http.POST(postraw);
  payload = http.getString();     
  Serial.println(payload);
  Serial.println(httpCode);
}

void callback(char* topic, byte* payload, unsigned int length) {
 
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
 
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
 
  Serial.println();
  Serial.println("-----------------------");
 
}

void mqtt(){
    char mac[20];
    const char* p_mac;
    String s_mac;  
    client.setServer(mqttServer, mqttPort);
    client.setCallback(callback);

    while (!client.connected()) {
      Serial.println("Connecting to MQTT...");
   
      if (client.connect("ESP8266Client", mqttUser, mqttPassword )) {
        Serial.println("connected");  
      } else {
          Serial.print("failed with state ");
          Serial.print(client.state());
          delay(2000);  
      }
    }

    s_mac = WiFi.macAddress();
    memset(mac,0,20);
    int lenn = s_mac.length();
    for (i = 0; i < lenn; i++){
      mac[i] = s_mac[i];
    }
    mac[lenn] = '\0';
    p_mac = &mac[0];
  //client.publish("esp/test", "Hello from ESP8266");
  client.subscribe(p_mac);
}

