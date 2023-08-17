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
const char* password_red = "56721058";
#define WIFI_TIMEOUT_MS 20000
#define brokerUser "PabloPaez07"
#define brokerPass "anv64ahx"
#define broker "broker.emqx.io"
int port = 1883;
long tiempo_habitacion_1 = 0;
long tiempo = 0;
long tiempo_cocina = 0;
long tiempo_alarma_movimiento = 0;
long tiempo_alarma_gases = 0;
long tiempo_lectura_temperatura = 0;
float movimiento;
bool enviada_alerta = false;
bool alarma_movimiento_activa = true;
bool alarma_gases_activa = true;
bool enviada_alerta_gases = false;
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
  pinMode(32, INPUT);
  cliente.setInsecure();
  humedad = dht11.getHumidity();
  temperatura = dht11.getTemperature();
}

void loop() {
  
  if(!client.connected()){
    reconnect();
  }

  
//LECTURA DE TEMPERATURAS Y HUMEDAD (DHT11) ------------------------
// if( tiempo2 > (tiempo_lectura_temperatura + 6000) )
// {

// }
  monoxido_carbono = analogRead(35);
  butano_propano = analogRead(34);

//------------- JSON -----------------------------------------------


  StaticJsonDocument<500>  JSONbuffer_habitacion;
  StaticJsonDocument<500>  JSONbuffer_cocina;



  char JSONmessageBuffer_habitacion[200];
  char JSONmessageBuffer_cocina[200];



//------------------------------------------------------------------


 tiempo = millis();
 if(tiempo > (tiempo_habitacion_1+60000))
 {
  humedad = dht11.getHumidity();
  temperatura = dht11.getTemperature();
  JSONbuffer_habitacion["Temperatura"] = temperatura;
  JSONbuffer_habitacion["Humedad"] = humedad;
  serializeJson(JSONbuffer_habitacion, JSONmessageBuffer_habitacion);
  client.publish("habitacion/1", JSONmessageBuffer_habitacion);
  tiempo_habitacion_1=millis();
 }

 if(tiempo > (tiempo_cocina+10000))
 {
  JSONbuffer_cocina["CO"] = monoxido_carbono/4095;
  JSONbuffer_cocina["Butano"] = butano_propano/4095;
  serializeJson(JSONbuffer_cocina, JSONmessageBuffer_cocina);
  client.publish("cocina", JSONmessageBuffer_cocina);
  tiempo_cocina=millis();
 }

//------------------------------------------------------------------------
  if(digitalRead(32))
  {
    if(!enviada_alerta && alarma_movimiento_activa)
    {
      bot.sendSimpleMessage(CHAT_ID,"¡He detectado movimiento!","");
      enviada_alerta = true;
    }
    if(tiempo > (tiempo_alarma_movimiento + 300000)) // 5 minutos = 300000
    {
      enviada_alerta = false;
      tiempo_alarma_movimiento = millis();
    }
  }


  if(!enviada_alerta_gases && (monoxido_carbono/4095 > 0.8 || butano_propano/4095 > 0.8) && alarma_gases_activa)
  {
    bot.sendSimpleMessage(CHAT_ID,"!PRESENCIA DE GASES PELIGROSOS!","");
    enviada_alerta_gases = true;
  }
  if(tiempo > (tiempo_alarma_gases + 60000))
  {
    enviada_alerta_gases = false;
    tiempo_alarma_gases = millis();
  }

  client.loop();
}