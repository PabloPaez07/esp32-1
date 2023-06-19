#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHTesp.h>


const char* nombre_red = "Finetwork_5wmS";
const char* password_red = "MtC3zkcJ";
#define WIFI_TIMEOUT_MS 20000
#define brokerUser "PabloPaez07"
#define brokerPass "anv64ahx"
#define broker "broker.emqx.io"
int port = 1883;
long tiempo1 = 0;
long tiempo2 = 0;


DHTesp dht11;

WiFiClient espClient;
PubSubClient client(espClient);

void setupWifi(){
  Serial.print("\nConnecting to WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(nombre_red,password_red);

  while( WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(100);
  }

  if(WiFi.status() != WL_CONNECTED)
  {
    Serial.println("No se ha podido conectar a la red WiFi");
  }else{
    Serial.println("Conectado correctamente");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  }
}

void reconnect(){
  while(!client.connected()){
    Serial.print("\nConnecting to");
    Serial.println(broker);
    if(client.connect("topic",brokerUser,brokerPass)){
      Serial.print("\nConnected to");
      Serial.println(broker);
    }else{
      Serial.println("\nTrying connect again");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(9600);
  setupWifi();
  client.setServer(broker, port);
  pinMode(2,OUTPUT);
  dht11.setup(16,DHTesp::DHT11);
}

void loop() {
  
  if(!client.connected()){
    reconnect();
  }

  //LECTURA DE TEMPERATURAS Y HUMEDAD (DHT11) ------------------------
  float humedad = dht11.getHumidity();
  float temperatura = dht11.getTemperature();
  //------------- JSON -----------------------------------------------
  StaticJsonBuffer<500> JSONbuffer;
  JsonObject& JSONencoder = JSONbuffer.createObject();

  JSONencoder["Dispositivo"] = "ESP32";
  JSONencoder["Sensor"] = "DHT11";
  JSONencoder["Temperatura"] = temperatura;
  JSONencoder["Humedad"] = humedad;

  char JSONmessageBuffer[200];
  JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  //------------------------------------------------------------------
 tiempo2 = millis();
 if(tiempo2 > (tiempo1+10000)){
  Serial.println(dht11.getMinimumSamplingPeriod());
  Serial.println(temperatura);
  Serial.println(humedad);
  Serial.println("--------");
  tiempo1 = millis();
  digitalWrite(2, HIGH);
  client.publish("habitacion/1", JSONmessageBuffer);
 }
  

  //------------------------------------------------------------------
  client.loop();
  digitalWrite(2,LOW);
}