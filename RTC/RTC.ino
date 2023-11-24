//Libreria para podernos conectar el dispositivo a Internet

//por Wi-Fi

#include <WiFi.h>
#include <MQUnifiedsensor.h>

//Libreria para podernos conectar al broker MQTT

#include <PubSubClient.h>

#include "DHTesp.h"  //DHT11 Library for ESP

#define DHTpin 14    //D5 of NodeMCU is GPIO14

#define placa "ESP-32"
#define Voltage_Resolution 3.3
#define pin 34
#define type "MQ-135"
#define ADC_Bit_Resolution 12
#define RatioMQ135CleanAir 3.6  
double CO2 = (0); 
MQUnifiedsensor MQ135(placa, Voltage_Resolution, ADC_Bit_Resolution, pin, type);
int pinSensor = 5;
int movimiento=0;
int pinLed = 13;

DHTesp dht;

float humidity, temperature;

 

//Constantes para conectarnos a la red Wi-Fi mediante ssid y su contraseña

const char* ssid = "irving";

const char* password = "irving123";

int BUILTIN_LED=0;

//Dirección del broker MQTT

const char* mqtt_server = "test.mosquitto.org";

 

//Aquí se realiza configura la conexión del dispositivo para conectarse

//al Wi-Fi y al broker

 

WiFiClient espClient;

 

PubSubClient client(espClient);

 

unsigned long lastMsg = 0;

 

#define MSG_BUFFER_SIZE (50)

 

char msg[MSG_BUFFER_SIZE];

 

int value = 0;

 

 

 

void setup_wifi() {

  delay(10);

 

  // Empezamos por conectarnos a una red WiFi

 

  Serial.println();

  Serial.print("Conectado a ");

  Serial.println(ssid);

  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, password);

 

  while (WiFi.status() != WL_CONNECTED) {

 

    delay(500);

 

    Serial.print(".");

 

  }

 

  randomSeed(micros());

  Serial.println("");

  Serial.println("WiFi conectado");

  Serial.println("Dirección IP: ");

  Serial.println(WiFi.localIP());

}

 

 

 

void callback(char* topic, byte* payload, unsigned int length) {

  Serial.print("Llegó el mensaje del tema [");

  Serial.print(topic);

  Serial.print("] ");

  String message;

 

  for (int i = 0; i < length; i++) {

    message = message + (char) payload[i];  // convert *byte to string

    Serial.print((char)payload[i]);

  }

 

  Serial.println();

  Serial.print(message);

 

  if(message=="prender")

  {

  digitalWrite(BUILTIN_LED, LOW);  // Ponemos el voltaje del led en bajo por que el dispositivo lo tiene invertido     

  Serial.println(); 

  Serial.println("Se prendio el LED");

  }

 

  if(message=="apagar")

  {

  digitalWrite(BUILTIN_LED, HIGH);   // Ponemos el volta del led en alto por que el dispositivo lo tiene invertido

  Serial.println("Se apago el LED");

  }

}

 

 

 

void reconnect() {

 

  // Bucle hasta que nos volvamos a conectar

 

  while (!client.connected()) {

    Serial.print("Intentando la conexión MQTT...");

    // Crea una identificación de cliente aleatoria

    String clientId = "ESP8266Client-";

    clientId += String(random(0xffff), HEX);

    // Intenta conectarse

    if (client.connect(clientId.c_str())) {

      Serial.println("conectado");

      // Una vez conectado, publique un anuncio...

      client.publish("Clatua/led", "hello world");

      // ... y volver a suscribirte

      client.subscribe("Clatua/acciones");

    } else {

      Serial.print("fallido, rc =");

      Serial.print(client.state());

      Serial.println(" inténtalo de nuevo en 5 segundos");

      // Espere 5 segundos antes de volver a intentarlo

      delay(5000);

    }

 

  }

 

}

 

 

 

void setup() {
  pinMode(pinSensor,INPUT);
  pinMode(pinLed,OUTPUT);
  pinMode(BUILTIN_LED, OUTPUT);     // Inicialice el pin BUILTIN_LED como salida

  Serial.begin(115200);

  dht.setup(DHTpin, DHTesp::DHT11); //for DHT11 Connect DHT sensor to GPIO 14

  setup_wifi();

  client.setServer(mqtt_server, 1883);

  client.setCallback(callback);

  //Set math model to calculate the PPM concentration and the value of constants   
    MQ135.setRegressionMethod(1); //_PPM =  a*ratio^b   
    MQ135.setA(110.47); 
    MQ135.setB(-2.862); 
    // Configurate the ecuation values to get NH4 concentration    
    MQ135.init();    
    Serial.print("Calibrating please wait.");   
    float calcR0 = 0;   
    for(int i = 1; i<=10; i ++)   {     
        MQ135.update(); // Update data, the arduino will be read the voltage on the analog pin     
        calcR0 += MQ135.calibrate(RatioMQ135CleanAir);    
        Serial.print(".");   
    }   
    MQ135.setR0(calcR0/10);   
    Serial.println("  done!.");      
    if(isinf(calcR0)) { Serial.println("Warning: Conection issue founded, R0 is infite (Open circuit detected) please check your wiring and supply"); while(1);}   
    if(calcR0 == 0){Serial.println("Warning: Conection issue founded, R0 is zero (Analog pin with short circuit to ground) please check your wiring and supply"); while(1);}   
    /**********  MQ CAlibration *********/                   
    MQ135.serialDebug(false); 
 

}

 

void loop() {

 

delay(2000);

  if (!client.connected()) {

    reconnect();

  }

  client.loop();

 //Get Humidity temperatue data after request is complete

 //Give enough time to handle client to avoid problems

  delay(dht.getMinimumSamplingPeriod());

 

  humidity = dht.getHumidity();

  temperature = dht.getTemperature();

  // Publicación de los datos en el servidor MQTT

  char humidityStr[6];

  char temperatureStr[6];

  dtostrf(humidity, 4, 2, humidityStr);

  dtostrf(temperature, 4, 2, temperatureStr);

 

  char payload[170];

  sprintf(payload, "{\"humidity\": %s, \"temperature\": %s, \"CO2\": %.2f, \"movimiento\": %i}", humidityStr, temperatureStr, CO2, movimiento);

  client.publish("Clatua/sensor", payload);

 

 Serial.print("Humedad: ");

  Serial.print(humidity);

  Serial.print("% - Temperatura: ");

  Serial.print(temperature);

  Serial.println("°C");

 MQ135.update(); // Update data, the arduino will be read the voltage on the analog pin   
    CO2 = MQ135.readSensor(); // Sensor will read CO2 concentration using the model and a and b values setted before or in the setup   
    Serial.print("CO2: ");   
    Serial.println(CO2);    
    delay(5000);

    if(digitalRead(pinSensor))
  {
    movimiento=1;
    digitalWrite(pinLed,HIGH);
    Serial.println("Vibrando");
    delay(1000);
   }
   else
     digitalWrite(pinLed,LOW);

}