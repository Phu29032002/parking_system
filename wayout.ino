#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <String>
#include <Arduino.h>
#include <MFRC522.h>
#include <SPI.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <LiquidCrystal_I2C.h>
#include <HTTPClient.h>
#define Miso 19
#define Mosi 23
#define Sck 18
#define Sda 5
#define Rst 22
#define servoPin 2
#define LM393 4
const char* ssid = "ok";
const char* password = "tqnguyen"; 

MFRC522 mfrc522(Sda,Rst);
LiquidCrystal_I2C lcd(0x3F, 16,2);
String apikey = "esp3212345";
String Id = "";
String card;
#define NEUTRAL_PULSE_WIDTH 1500   // Neutral pulse width in microseconds
#define PERIOD 20000   // PWM period in microseconds (50 Hz)

#define MQTT_SERVER "broker.mqttdashboard.com"
#define MQTT_PORT 1883
#define MQTT_USER "parking"
#define MQTT_PASSWORD "123456"
#define MQTT_LDP_TOPIC "esp32/trigercam"
#define MQTT_CAR "esp32/car"

WiFiClient wifiClient;
PubSubClient client(wifiClient);
String carplate = "";
//String hour;
int fee;
 void wificonnect()
{
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println(WiFi.localIP());
}

void opengate(){
  digitalWrite(servoPin, HIGH);
 delayMicroseconds(NEUTRAL_PULSE_WIDTH + 500); // 90 degrees * 10 microseconds per degree
  digitalWrite(servoPin, LOW);
  delay(1000); 
  
}
void closegate()
{
 digitalWrite(servoPin, HIGH);
   delayMicroseconds(NEUTRAL_PULSE_WIDTH - 500); // 150 degrees * 10 microseconds per degree
  digitalWrite(servoPin, LOW);
  delay(1000);   // wait for a second
}
//scroll
void scrollText(const String& text, int delayTime) {
  for (int i = 0; i <= text.length() - 16; i++) {
    lcd.setCursor(0, 0);
    lcd.print(text.substring(i, i + 16));
    delay(delayTime);
  }
  lcd.setCursor(0, 0);
  //lcd.print("                "); // Clear the first line of the display
}
// LCD
void display(String res)
{
  String hour = res.substring(5,res.indexOf(" hours") - 4);
  const char* string_number = hour.c_str();
  Serial.println(string_number);  
  int fhour = atoi(string_number);
  Serial.println(fhour);    
  fee = fhour * 2000;
  lcd.print(res);
  lcd.setCursor(0,1);
  lcd.print("Fee: " + String(fee));
  Serial.println(res);
  Serial.println(fee);
  scrollText(res, 500);
}

//get data from server
String getdata()
{
  Serial.println("Sending data to server ...");
    HTTPClient http;
    http.begin("http://192.168.166.43/parking/espwayout.php");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String httpRequestData = "api_key=" + apikey + "&id=" + card+ "&car=" + carplate;
  
  
  int httpResponseCode = http.POST(httpRequestData);
    if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println(httpResponseCode);   
        Serial.println(response);
        if(response.substring(0,4) == "true")
        {
          http.end();
          String res = response.substring(5,response.indexOf("minutes")+7);
          display(res);
         // hour = response.substring(5,a.find(" hours") - 5);          
          return response.substring(0,4);
        }
        else
        {
          http.end();
          return response.substring(0,5);
        }
    }
    else {
        Serial.print("Error on sending POST: ");
        Serial.println(httpResponseCode);
        http.end();
    }
    
}

String readrfid()
{
  if (!mfrc522.PICC_IsNewCardPresent())
  {
    return "";
  }
  if (!mfrc522.PICC_ReadCardSerial())
  {
    return "";
  }
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    Id.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : ""));
    Id.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Id.toUpperCase();
 
  card = Id;
  Id = "";
  Serial.println(card);
  
  return card;
}

//MQTT
void connect_to_broker() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "IoTLab4";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("connected");
      client.subscribe(MQTT_CAR);
      Serial.println("flag"); 
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      delay(2000);
    }
  }
}

void callback(char* topic, byte *payload, unsigned int length) {
  Serial.println("-------new message from broker-----");
  Serial.print("topic: ");
  Serial.println(topic);
 
  Serial.print("Callback - ");
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    carplate += (char)payload[i];
  }

}

void send_trigcam() {
  client.publish(MQTT_LDP_TOPIC, "3"); 
  delay(500);
}
void send_discam()
{
  client.publish(MQTT_LDP_TOPIC, "4");
  delay(100);
}
int delaytime = 2500;
unsigned long previousMillis = 0;
unsigned long currentMillis = millis();
void run()
{
  
  if(digitalRead(LM393) == LOW)
  {
    Serial.println("replay.........");
    while(card == "")
    {
      readrfid();
      delay(200);
    }
     Serial.println("Read_RF");
     send_trigcam();
 while(carplate =="" && carplate != "1" && carplate == carplate)
  {
    client.loop();
//    Serial.println("Listen mqtt");
     if (!client.connected()) {
       Serial.println("Listen mqtt1");  
        connect_to_broker();
        Serial.println("Listen mqtt2");
        send_trigcam();
        Serial.println("Listen mqtt3");      
     }
   } 
    Serial.println(carplate);
    send_discam();
    
   String check = "true";
    while(getdata() != "true")
    {
      if(getdata() == "false")
      {
        Serial.println("Car not found");
        check = "false";
         carplate = "";
          card="";        
        break;
      }
      
      delay(500);
    }
    Serial.println(check);
    carplate = "";
    card = "";
    if(check == "true")
    {
      Serial.println("Car found");
      opengate();
     while(digitalRead(LM393) == LOW)
     {
    
     }
        delay(2000);
        closegate();
    }
    
  }
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  SPI.begin(Sck, Miso, Mosi, Sda);
  pinMode(servoPin, OUTPUT);
  mfrc522.PCD_Init();
  wificonnect();
  client.setServer(MQTT_SERVER, MQTT_PORT );
  client.setCallback(callback);
  connect_to_broker();
  lcd.init();
  lcd.backlight();
  lcd.print( "Thank you" );
}

void loop() {
  // put your main code here, to run repeatedly:
  run();
}