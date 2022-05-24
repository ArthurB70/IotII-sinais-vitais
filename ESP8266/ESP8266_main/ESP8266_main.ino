#include <Wire.h>
#include <Arduino.h>
#include "algorithm_by_RF.h"
#include "max30102.h"
#include "algorithm.h"

#define MAX_30102_ADDR 0X57
const byte max30102_int_pin = 14; // GPIO14/D5
uint32_t elapsedTime,timeStart;

uint32_t aun_ir_buffer[BUFFER_SIZE]; //infrared LED sensor data
uint32_t aun_red_buffer[BUFFER_SIZE];  //red LED sensor data
float old_n_spo2;  // Previous SPO2 value
uint8_t uch_dummy,k;

void millis_to_hours(uint32_t ms, char* hr_str)
{
  char istr[6];
  uint32_t secs,mins,hrs;
  secs=ms/1000; // time in seconds
  mins=secs/60; // time in minutes
  secs-=60*mins; // leftover seconds
  hrs=mins/60; // time in hours
  mins-=60*hrs; // leftover minutes
  itoa(hrs,hr_str,10);
  strcat(hr_str,":");
  itoa(mins,istr,10);
  strcat(hr_str,istr);
  strcat(hr_str,":");
  itoa(secs,istr,10);
  strcat(hr_str,istr);
}
int cont =0;
void setup() {

  pinMode(max30102_int_pin, INPUT);  //pin D10 connects to the interrupt output pin of the MAX30102
  Serial.begin(115200);
  delay(100);
  maxim_max30102_init();  //initialize the MAX30102
  old_n_spo2=0.0;
  timeStart=millis();
  Serial.read();
}

//Continuously taking samples from MAX30102.  Heart rate and SpO2 are calculated every ST seconds
void loop() {
  float n_spo2,ratio,correl;  //SPO2 value
  int8_t ch_spo2_valid;  //indicator to show if the SPO2 calculation is valid
  int32_t n_heart_rate; //heart rate value
  int8_t  ch_hr_valid;  //indicator to show if the heart rate calculation is valid
  int32_t i;
  char hr_str[10];
  if(cont <1000){
    for(i=0;i<BUFFER_SIZE;i++, cont++)
      {
      while(digitalRead(max30102_int_pin)==1);  //wait until the interrupt pin asserts
      maxim_max30102_read_fifo((aun_red_buffer+i), (aun_ir_buffer+i));  //read from MAX30102 FIFO
  /*
      Serial.print(i, DEC);
      Serial.print(F("\t"));
      Serial.print(aun_red_buffer[i], DEC);
      Serial.print(F("\t"));
      Serial.print(aun_ir_buffer[i], DEC);    
      Serial.println("");*/
      delay(5);
      }    
  }
//param[in]    *pun_ir_buffer           - IR sensor data buffer
//param[in]    n_ir_buffer_length      - IR sensor data buffer length
//param[in]    *pun_red_buffer          - Red sensor data buffer
//param[out]    *pn_spo2                - Calculated SpO2 value
//param[out]    *pch_spo2_valid         - 1 if the calculated SpO2 value is valid
//param[out]    *pn_heart_rate          - Calculated heart rate value
//param[out]    *pch_hr_valid           - 1 if the calculated heart rate value is valid
  maxim_heart_rate_and_oxygen_saturation(aun_ir_buffer, BUFFER_SIZE, aun_red_buffer, &n_spo2, &ch_spo2_valid, &n_heart_rate, &ch_hr_valid);

   Serial.println("--RF--");
  Serial.print(elapsedTime);
  Serial.print("\t");
  Serial.print(n_spo2);
  Serial.print("\t");
  Serial.print(n_heart_rate, DEC);
  Serial.println("------");
  n_spo2 = 0;
  n_heart_rate= 0;
  /*
  //calculate heart rate and SpO2 after BUFFER_SIZE samples (ST seconds of samples) using Robert's method
  rf_heart_rate_and_oxygen_saturation(aun_ir_buffer, BUFFER_SIZE, aun_red_buffer, &n_spo2, &ch_spo2_valid, &n_heart_rate, &ch_hr_valid, &ratio, &correl); 
  elapsedTime=millis()-timeStart;
  millis_to_hours(elapsedTime,hr_str); // Time in hh:mm:ss format
  elapsedTime/=1000; // Time in seconds
  // Read the _chip_ temperature in degrees Celsius
  int8_t integer_temperature;
  uint8_t fractional_temperature;
  maxim_max30102_read_temperature(&integer_temperature, &fractional_temperature);
  float temperature = integer_temperature + ((float)fractional_temperature)/16.0;

  Serial.println("--RF--");
  Serial.print(elapsedTime);
  Serial.print("\t");
  Serial.print(n_spo2);
  Serial.print("\t");
  Serial.print(n_heart_rate, DEC);
  Serial.print("\t");
  Serial.print(hr_str);
  Serial.print("\t");
  Serial.println(temperature);
  Serial.println("------");


  if(ch_hr_valid && ch_spo2_valid) { 
    old_n_spo2=n_spo2;
  }
  */
}
