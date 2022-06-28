#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include <TZ.h>
#include <FS.h>
#include <LittleFS.h>
#include <CertStoreBearSSL.h>
#include <SparkFunMLX90614.h>

const char* ssid = "ASUS_Arthur";
const char* password = "Po#Is5376";
const char* mqtt_server = "ba87952a69c64e73a1569a69541d7fd6.s1.eu.hivemq.cloud";

BearSSL::CertStore certStore;

WiFiClientSecure espClient;
PubSubClient * client;
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (500)
char msg[MSG_BUFFER_SIZE];
int value = 0;


// mqtt user: iot-vital-signs-user
// mqtt password: #Po23aa.14sz
IRTherm therm;
MAX30105 max30102;

int32_t bufferLength = 100; 
int32_t oxigenacao; 
int8_t v_oxigenacao; 
int32_t batimentoCardiaco; 
int8_t v_batimentoCardiaco; 

float batimentoCardiaco_avg = 0; 
float oxigenacao_avg = 0; 
float temp_avg = 0;
int i_hr = 0, i_spo = 0, i_temp = 0;
int32_t milli_segundos = 16000; 

uint32_t irBuffer[100];
uint32_t redBuffer[100];

uint32_t inicio_tempo;


String callback_retorno = "";

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
void setDateTime() {
  // You can use your own timezone, but the exact time is not used at all.
  // Only the date is needed for validating the certificates.
  configTime(TZ_Europe_Berlin, "pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(100);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println();

  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.printf("%s %s", tzname[0], asctime(&timeinfo));
}
void callback(char* topic, byte* payload, unsigned int length) {
  callback_retorno = "";
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    callback_retorno += (char)payload[i];
  }
  Serial.println();

  // Switch on the LED if the first character is present
  if ((char)payload[0] != NULL) {
    digitalWrite(LED_BUILTIN, LOW); // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH); // Turn the LED off by making the voltage HIGH
  } else {
    digitalWrite(LED_BUILTIN, HIGH); // Turn the LED off by making the voltage HIGH
  }
}
void reconnect() {
  // Loop until we’re reconnected
  while (!client->connected()) {
    Serial.print("Attempting MQTT connection…");
    String clientId = "ESP8266Client - MyClient";
    // Attempt to connect
    // Insert your password
    if (client->connect(clientId.c_str(), "iot-vital-signs-user", "#Po23aa.14sz")) {
      Serial.println("connected");
      // Once connected, publish an announcement…
      client->publish("sinais_vitais_topic", "device: connected");
      // … and resubscribe
      client->subscribe("sinais_vitais_topic");
    } else {
      Serial.print("failed, rc = ");
      Serial.print(client->state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200); 
  Serial.println("Conecando MAX30102 . . .");
  while (!max30102.begin(Wire, I2C_SPEED_STANDARD));
  Serial.println("MAX30102 conectado!");  
  
  
  byte ledBrightness = 60;
  byte sampleAverage = 4;
  byte ledMode = 2;
  byte sampleRate = 100; 
  int pulseWidth = 411; 
  int adcRange = 4096;
  Serial.println("Inicializando MAX30102 . . ."); 
  max30102.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);
  Serial.println("MAX30102 inicializado!"); 
  Wire.begin();
  if (therm.begin() == false){ // Initialize thermal IR sensor
    Serial.println("Qwiic IR thermometer did not acknowledge! Freezing!");
    while(1);
  }
  Serial.println("Qwiic IR Thermometer did acknowledge.");
  
  therm.setUnit(TEMP_C); // Set the library's units to Farenheit
  // Alternatively, TEMP_F can be replaced with TEMP_C for Celsius or
  // TEMP_K for Kelvin.
    delay(500);

  LittleFS.begin();
  setup_wifi();
  setDateTime();

  pinMode(LED_BUILTIN, OUTPUT); // Initialize the LED_BUILTIN pin as an output

  // you can use the insecure mode, when you want to avoid the certificates
  //espclient->setInsecure();

  int numCerts = certStore.initCertStore(LittleFS, PSTR("/certs.idx"), PSTR("/certs.ar"));
  Serial.printf("Number of CA certs read: %d\n", numCerts);
  if (numCerts == 0) {
    Serial.printf("No certs found. Did you run certs-from-mozilla.py and upload the LittleFS directory before running?\n");
    return; // Can't connect to anything w/o certs!
  }

  BearSSL::WiFiClientSecure *bear = new BearSSL::WiFiClientSecure();
  // Integrate the cert store with this connection
  bear->setCertStore(&certStore);

  client = new PubSubClient(*bear);

  client->setServer(mqtt_server, 8883);
  client->setCallback(callback);
  inicio_tempo = millis();
}

void loop()
{

  
  if (!client->connected()) {
    reconnect();
  }
  else {
    batimentoCardiaco_avg = 0; 
    oxigenacao_avg = 0; 
    temp_avg = 0;
    i_hr = 0, i_spo = 0, i_temp = 0;
    if(callback_retorno == "usuario: m"){
      callback_retorno = "";
      batimentoCardiaco_avg = oxigenacao_avg = temp_avg = 0; 
      i_hr = i_spo = i_temp = 0;
      for (byte i = 0 ; i < bufferLength ; i++)
      {
        while (max30102.available() == false) 
          max30102.check();
        redBuffer[i] = max30102.getRed();
        irBuffer[i] = max30102.getIR();
        max30102.nextSample(); 
      }
      maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &oxigenacao, &v_oxigenacao, &batimentoCardiaco, &v_batimentoCardiaco);
      //Continuously taking samples from MAX30102.  Heart rate and oxigenacao are calculated every 1 second
      while ((millis() - inicio_tempo + 4000 < milli_segundos) || (i_hr <= (((milli_segundos/1000)/4) - 1) && i_spo <= (((milli_segundos/1000)/4) - 1)))
      {
      
        for (byte i = 0; i < 100; i++)
        {
          while (max30102.available() == false)
            max30102.check(); 
          redBuffer[i] = max30102.getRed();
          irBuffer[i] = max30102.getIR();
          max30102.nextSample(); 
        }
        maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &oxigenacao, &v_oxigenacao, &batimentoCardiaco, &v_batimentoCardiaco);
        
        if (therm.read()) // On success, read() will return 1, on fail 0.
        {
          float temp = therm.object();
          temp_avg += temp;
          Serial.print(F("->temp="));
          Serial.println(temp);
          i_temp++;
        }
        if(v_oxigenacao = 1 && oxigenacao >= 70 && oxigenacao <= 100){
          oxigenacao_avg += oxigenacao;
          Serial.print(F("->oxigenacao="));
          Serial.println(oxigenacao);
          i_spo++;
        }
        if(v_batimentoCardiaco = 1 && batimentoCardiaco >= 40 && batimentoCardiaco <= 180){
          batimentoCardiaco_avg += batimentoCardiaco;
          Serial.print(F("->hr="));
          Serial.println(batimentoCardiaco);
          i_hr++;
        } 
        if(i_spo > 0 || i_hr > 0 || i_temp > 0){
          String aux = "";
          aux = "";
          aux += "device(p): ";
          aux += String(oxigenacao_avg/i_spo);
          aux += ",";
          aux += String((batimentoCardiaco_avg/i_hr)*0.7);
          aux += ",";
          aux += String(temp_avg/i_temp);
          aux.toCharArray(msg, MSG_BUFFER_SIZE);
          //snprintf (msg, MSG_BUFFER_SIZE,retorno, );
          Serial.print("Publish message: ");
          Serial.println(msg);
          while(!client->connected()){
          reconnect();
          }
          client->publish("sinais_vitais_topic", msg);
        }
      }
        String aux = "";
    
        aux += "device(f): ";
        aux += String(oxigenacao_avg/i_spo);
        aux += ",";
        aux += String((batimentoCardiaco_avg/i_hr)*0.7);
        aux += ",";
        aux += String(temp_avg/i_temp);
        //snprintf (msg, MSG_BUFFER_SIZE, "%s", retorno);
        aux.toCharArray(msg, MSG_BUFFER_SIZE);
        Serial.print("Publish message: ");
        Serial.println(msg);
        while(!client->connected()){
          reconnect();
          }
          client->publish("sinais_vitais_topic", msg);
        
        Serial.print(F("M->oxigenacao="));
        Serial.println(oxigenacao_avg/i_spo);
      
        Serial.print(F("M->hr="));
        Serial.println((batimentoCardiaco_avg/i_hr)*0.7);

        Serial.print(F("M->temperatura="));
        Serial.println(temp_avg/i_temp);
    
    }
    else {
      client->loop();
    }
  }
}
