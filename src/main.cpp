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

WiFiClientSecure cliente_telegram;
UniversalTelegramBot bot(BOTtoken, cliente_telegram);
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
  dht11.setup(GPIO_NUM_33,DHTesp::DHT11);
  pinMode(GPIO_NUM_35, INPUT);  //CO
  pinMode(GPIO_NUM_34, INPUT); //Butano_Propano
  pinMode(GPIO_NUM_32, INPUT);
  pinMode(GPIO_NUM_25, INPUT);
  pinMode(GPIO_NUM_27, INPUT);
  cliente_telegram.setInsecure();
  int segundos_sleep = 60;
  esp_sleep_enable_gpio_wakeup();
  esp_sleep_enable_timer_wakeup(segundos_sleep * 1000000);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_25,0);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_27,0);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_32,1);
}

void loop() {
  WiFi.reconnect();
  if(WiFi.status() == WL_CONNECTED)
  {
    reconnect();
  }
  
//------------- JSON -----------------------------------------------
  StaticJsonDocument<500>  JSONbuffer_habitacion;
  StaticJsonDocument<500>  JSONbuffer_cocina;

  char JSONmessageBuffer_habitacion[200];
  char JSONmessageBuffer_cocina[200];

//------------------------------------------------------------------
  humedad = dht11.getHumidity();
  temperatura = dht11.getTemperature();
  JSONbuffer_habitacion["Temperatura"] = temperatura;
  JSONbuffer_habitacion["Humedad"] = humedad;
  serializeJson(JSONbuffer_habitacion, JSONmessageBuffer_habitacion);
  client.publish("habitacion/1", JSONmessageBuffer_habitacion);

  monoxido_carbono = analogRead(35);
  butano_propano = analogRead(34);
  JSONbuffer_cocina["CO"] = monoxido_carbono/4095;
  JSONbuffer_cocina["Butano"] = butano_propano/4095;
  serializeJson(JSONbuffer_cocina, JSONmessageBuffer_cocina);
  client.publish("cocina", JSONmessageBuffer_cocina);

//------------------------------------------------------------------------
  if(digitalRead(32))
  {
    if(!enviada_alerta && alarma_movimiento_activa)
    {
      bot.sendSimpleMessage(CHAT_ID,"¡He detectado movimiento!","");
    }
  }
  Serial.println("----------");
  Serial.print("32: ");
  Serial.println(digitalRead(32));
  Serial.print("25: ");
  Serial.println(digitalRead(25));
  Serial.print("27: ");
  Serial.println(digitalRead(27));
  if(!enviada_alerta_gases && (!digitalRead(25) || !digitalRead(27)) && alarma_gases_activa)
  {
    Serial.println("GASES");
    Serial.print("25: ");
    Serial.println(digitalRead(25));
    Serial.print("27: ");
    Serial.println(digitalRead(27));
    Serial.println("----------");
    bot.sendSimpleMessage(CHAT_ID,"!PRESENCIA DE GASES PELIGROSOS!","");
  }
  client.loop();

  esp_light_sleep_start();
}