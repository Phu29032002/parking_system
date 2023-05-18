#include <Arduino.h>
#include <iostream>
#include <WiFi.h>

#include <MFRC522.h>
#include <SPI.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <WebServer.h>
#define Miso 19
#define Mosi 23
#define Sck 18
#define Sda 5
#define Rst 22
#define servoPin 2
#define LM393 4
const char* ssid = "ok";
const char* password = "tqnguyen";
String apikey = "esp3212345";
//int messageid;
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
String carplate;

MFRC522 mfrc522(Sda,Rst);
String Id = "";
 String card;


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
  client.publish(MQTT_LDP_TOPIC, "1"); 
  delay(500);
}
void send_discam()
{
  client.publish(MQTT_LDP_TOPIC, "2");
  delay(100);
}
// triger RFID
void triggerRFID()
{
 if(digitalRead(LM393) == HIGH)
 {
  do
  {
   readrfid();
    client.loop();
  if (!client.connected()) {  
    connect_to_broker();
  } 
   send_trigcam();
  } while (Id == "" && carplate == "");
  opengate();
 }
 else
 {
  closegate();
  Serial.println(carplate);
  Serial.println(Id);
 }
}
String check ;
void SendData()
{ 
    Serial.println("Sending data to server ...");
    HTTPClient http;
    http.begin("http://192.168.166.43/parking/esppostdata.php");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String httpRequestData = "api_key=" + apikey + "&id=" + card+ "&car=" + carplate;
    
    int httpResponseCode = http.POST(httpRequestData);
    if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println(httpResponseCode);
        Serial.println(response);
        check = response;
    }
    else {
        Serial.print("Error on sending POST: ");
        Serial.println(httpResponseCode);
        check = httpResponseCode;
    }
    http.end();
}

int delaytime = 2500;
unsigned long previousMillis = 0;
unsigned long currentMillis = millis();
void demo()
{
 if(digitalRead(LM393) == LOW)
 {
  while(readrfid()=="")
  {
     readrfid();
    
    delay(200);
    
  } 
  Serial.println("Read_RF");
  send_trigcam();
  while(carplate =="" && carplate != "1" && carplate == carplate)
  {
   // client.loop();
   // if (!client.connected()) {  
    // connect_to_broker();
   //} 
   client.loop();
    if (!client.connected()) {  
     connect_to_broker();
       send_trigcam();
    } 

  } 
  
  Serial.println(carplate);
  send_discam();
  SendData();
  if(check != "1")
  {
    Serial.println("server down");
    SendData();
  }
  carplate = "";
  opengate();
  Serial.println("test reponse");
Serial.println(check);  
  while(digitalRead(LM393) == LOW)
  {
    
  }
  delay(2000);
  closegate();
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
  
}

void loop() {
  // put your main code here, to run repeatedly:
  //  client.loop();
  // if (!client.connected()) {  
  //   connect_to_broker();
  // } 
 // triggerRFID();
demo();

}