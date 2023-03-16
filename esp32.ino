#include <WiFi.h>
#define RXp2 16
#define TXp2 17

WiFiClient client;

String apiKey = "EKL504G6U3M10XJK";                  //API key of the data channel thingspeak
const char* server = "api.thingspeak.com";


void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXp2, TXp2);
  Serial.println("Hello, ESP32!");
  WiFi.begin("quelTom", "password", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println(" Connected!");

}

void loop() {
  if (client.connect(server,80)){ 

    String data = Serial2.readString();
    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(data.length());
    client.print("\n\n");

    client.print(data);
    Serial.print(data);
    Serial.println("Sent data...");
  }
  delay(16000); //lets pause 'cause of the license limit of thingspeak
}  


