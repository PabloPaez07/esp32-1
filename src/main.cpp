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

const char* nombre_red = "TP-LINK_CC86";
const char* password_red = "@Wifipalu";
#define WIFI_TIMEOUT_MS 20000
#define brokerUser "PabloPaez07"
#define brokerPass "anv64ahx"
#define broker "broker.emqx.io"
int port = 1883;

long tiempo = 0;

long tiempo_actual_habitacion_1 = 0;
const long tiempo_habitacion_1 = 10000;

long tiempo_actual_cocina = 0;
const long tiempo_cocina = 5000;

long tiempo_actual_alarma_movimiento = 0;
const long tiempo_alarma_movimiento = 10000;

long tiempo_actual_alarma_gases = 0;
const long tiempo_alarma_gases = 5000;

float movimiento;
bool enviada_alerta = true;
bool alarma_movimiento_activa = true;
bool alarma_gases_activa = true;
bool enviada_alerta_gases = true;
float humedad;
float temperatura;
float monoxido_carbono;
float butano_propano;

DHTesp dht11;

WiFiClientSecure cliente;
UniversalTelegramBot bot(BOTtoken, cliente);
WiFiClient espClient;
PubSubClient client(espClient);

void setupWifi();
void publicarHumedadTemperaturaHabitacion1();
void publicarGasesCocina();
void envioAlertaMovimiento();
void envioAlertaGases();
void reconnect(){
  while(!client.connected()){
    Serial.print("\nConnecting to");
    Serial.println(broker);
    if(client.connect("#",brokerUser,brokerPass)){
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
  pinMode(32, INPUT); //Sensor de movimiento
  cliente.setInsecure();
  humedad = dht11.getHumidity();
  temperatura = dht11.getTemperature();
}

void loop() {
  if(!client.connected()){reconnect();}
  monoxido_carbono = analogRead(35);
  butano_propano = analogRead(34);

  tiempo = millis();

  if(tiempo > (tiempo_actual_habitacion_1 + tiempo_habitacion_1))
  {
    publicarHumedadTemperaturaHabitacion1();
    tiempo_actual_habitacion_1=millis();
  }

  if(tiempo > (tiempo_actual_cocina + tiempo_cocina))
  {
    publicarGasesCocina();
    tiempo_actual_cocina=millis();
  }

  Serial.print(digitalRead(32));
  if(digitalRead(32) == HIGH)
  {
    envioAlertaMovimiento();
  }

  if(monoxido_carbono/4095 > 0.8 || butano_propano/4095 > 0.8){
    envioAlertaGases();
  }

  client.loop();
}



//Funciones a
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
void publicarHumedadTemperaturaHabitacion1(){
  StaticJsonDocument<200>  JSONbuffer_habitacion;
  char JSONmessageBuffer_habitacion[200];
  humedad = dht11.getHumidity();
  temperatura = dht11.getTemperature();
  JSONbuffer_habitacion["Temperatura"] = temperatura;
  JSONbuffer_habitacion["Humedad"] = humedad;
  serializeJson(JSONbuffer_habitacion, JSONmessageBuffer_habitacion);
  client.publish("habitacion/1", JSONmessageBuffer_habitacion);
}
void publicarGasesCocina(){
  StaticJsonDocument<500>  JSONbuffer_cocina;
  char JSONmessageBuffer_cocina[200];
  JSONbuffer_cocina["CO"] = monoxido_carbono/4095;
  JSONbuffer_cocina["Butano"] = butano_propano/4095;
  serializeJson(JSONbuffer_cocina, JSONmessageBuffer_cocina);
  client.publish("cocina", JSONmessageBuffer_cocina);
}
void envioAlertaMovimiento(){
    if(!enviada_alerta && alarma_movimiento_activa)
    {
      Serial.printf("\nenvio_alarma_movimiento\n");
      bot.sendSimpleMessage(CHAT_ID,"¡He detectado movimiento!","");
      enviada_alerta = true;
    }
    if(tiempo > (tiempo_actual_alarma_movimiento + tiempo_alarma_movimiento))
    {
      enviada_alerta = false;
      tiempo_actual_alarma_movimiento = millis();
    }
}
void envioAlertaGases(){
  if(!enviada_alerta_gases && alarma_gases_activa){
      bot.sendSimpleMessage(CHAT_ID,"!PRESENCIA DE GASES PELIGROSOS!","");
      enviada_alerta_gases = true;
  }
  if(tiempo > (tiempo_actual_alarma_gases + tiempo_alarma_gases)){
    enviada_alerta_gases = false;
    tiempo_actual_alarma_gases = millis();
  }
}
