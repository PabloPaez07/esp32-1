#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <DHTesp.h>
#include <MQUnifiedsensor.h>
#include <UniversalTelegramBot.h>


#define BOTtoken "6334757569:AAHR6x_kyxVLyjcfqNCYgvBUDPSU0Py40Io"
#define CHAT_ID  "1527257134"

const char* nombre_red = "Finetwork_5wmS";
const char* password_red = "MtC3zkcJ";
#define WIFI_TIMEOUT_MS 20000
#define brokerUser "PabloPaez07"
#define brokerPass "anv64ahx"
#define broker "broker.emqx.io"
int port = 1883;
long tiempo1 = 0;
long tiempo2 = 0;
long tiempo3 = 0;
float movimiento;
bool enviada_alerta = false;
bool alarma_activa = true;
float humedad;
float temperatura;
float monoxido_carbono;
float butano_propano;

DHTesp dht11;

WiFiClientSecure cliente;
UniversalTelegramBot bot(BOTtoken, cliente);
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
    if(client.connect("alarma",brokerUser,brokerPass)){
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
  dht11.setup(33,DHTesp::DHT11);
  pinMode(35, INPUT);  //CO
  pinMode(34, INPUT); //Butano_Propano
  pinMode(32, INPUT);
  cliente.setInsecure();
}

void loop() {
  
  if(!client.connected()){
    reconnect();
  }

  //LECTURA DE TEMPERATURAS Y HUMEDAD (DHT11) ------------------------
  humedad = dht11.getHumidity();
  temperatura = dht11.getTemperature();
  monoxido_carbono = analogRead(35);
  butano_propano = analogRead(34);
  //------------- JSON -----------------------------------------------
  StaticJsonDocument<500>  JSONbuffer;

  JSONbuffer["Dispositivo"] = "ESP32";
  JSONbuffer["Sensor"] = "DHT11";
  JSONbuffer["Temperatura"] = temperatura;
  JSONbuffer["Humedad"] = humedad;
  JSONbuffer["CO"] = monoxido_carbono/4095;
  JSONbuffer["Butano"] = butano_propano/4095;

  char JSONmessageBuffer[200];
  serializeJson(JSONbuffer, JSONmessageBuffer);
  //------------------------------------------------------------------
 tiempo2 = millis();
 if(tiempo2 > (tiempo1+2000))
 {
  Serial.println(temperatura);
  Serial.println(humedad);
  Serial.println(monoxido_carbono/4095);
  Serial.println(butano_propano/4095);
  Serial.println("--------");
  client.publish("habitacion/1", JSONmessageBuffer);
  digitalWrite(2, HIGH);
  delay(100);
  digitalWrite(2,LOW);
  tiempo1=millis();
 }
//------------------------------------------------------------------------

  if(digitalRead(32))
  {
    if(!enviada_alerta && alarma_activa)
    {
      Serial.println("envio de mensaje");
      bot.sendMessage(CHAT_ID,"¡He detectado movimiento!","");
      enviada_alerta = true;
    }
    if(tiempo2 > (tiempo3 + 2200000)) // 5 minutos = 300000
    {
      enviada_alerta = false;
      tiempo3 = millis();
    }
  }
  client.loop();
}