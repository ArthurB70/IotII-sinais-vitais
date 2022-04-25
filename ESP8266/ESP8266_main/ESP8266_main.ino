#include <Wire.h>


#define MAX_30102_ADDR 0X57

void init_I2C_protocol(){
    Wire.begin();
    Wire.setClock(400000L);
}

void max30102_write(uint8_t addr, uint8_t data){
  Wire.beginTransmission(MAX_30102_ADDR); 
  Wire.write(addr);
  Wire.write(data);
  Wire.endTransmission();
}

uint8_t max30102_read(uint8_t addr){
  uint8_t data = -1;
  
  Wire.beginTransmission(MAX_30102_ADDR); 
  Wire.write(addr);
  Wire.endTransmission();
  
  Wire.beginTransmission(MAX_30102_ADDR); 
  Wire.requestFrom(MAX_30102_ADDR, 1);
  data = Wire.read();
  Wire.endTransmission();

  return data;
}
void max30102_read_data(uint32_t *r_data, uint32_t *ir_data){
  uint32_t aux_read = 0;
  *r_data = 0;
  *ir_data = 0;

  Wire.beginTransmission(MAX_30102_ADDR);
  Wire.write(0x07); // reg_fifo_data
  Wire.endTransmission();

  Wire.beginTransmission(MAX_30102_ADDR);
  Wire.requestFrom(MAX_30102_ADDR,6);

  aux_read = Wire.read();
  aux_read <<= 16;
  *r_data += aux_read;

  aux_read = Wire.read();
  aux_read <<= 8;
  *r_data += aux_read;

  aux_read = Wire.read();
  aux_read <<= 8;
  *r_data += aux_read;

  aux_read = Wire.read();
  *r_data += aux_read;

  aux_read = 0;

  aux_read = Wire.read();
  aux_read <<= 16;
  *ir_data += aux_read;

  aux_read = Wire.read();
  aux_read <<= 8;
  *ir_data += aux_read;

  aux_read = Wire.read();
  aux_read <<= 8;
  *ir_data += aux_read;

  aux_read = Wire.read();
  *ir_data += aux_read;

  Wire.endTransmission();

  *ir_data &= 0x03FFFF;
  *r_data &= 0x03FFFF;
}

void max30102_init(){
  Wire.begin();
  max30102_write(0x09, 0x40);
  delay(1000);
  max30102_read(0x00); 
  
  max30102_write(0x02,0xc0);
  max30102_write(0x03,0x00);
  max30102_write(0x04,0x00);
  max30102_write(0x05,0x00);
  max30102_write(0x06,0x00);
  max30102_write(0x08,0x4f);
  max30102_write(0x09,0x03);
  max30102_write(0x0A,0x27);
  max30102_write(0x0C,0x24);
  max30102_write(0x0D,0x24);
  max30102_write(0x10,0x7f);
}

uint32_t ir_buffer[100]; 
uint32_t r_buffer[100];
uint32_t r_avg;
uint32_t ir_avg;

const byte max30102_int_pin = 14; // GPIO14/D5

void setup()
{
  Serial.begin(115200);
  pinMode(max30102_int_pin, INPUT);
  delay(1000);
  init_I2C_protocol();
  max30102_init();
  
}
//
void loop()
{
  r_avg = 0;
  ir_avg = 0;
  //delay(1000);
  int j = 0;
  for(int i=0;i<100;i++)
    {
      while(digitalRead(max30102_int_pin) == 1);
      max30102_read_data((r_buffer+i),(ir_buffer+i));
      if(r_buffer[i]>ir_buffer[i] && ir_buffer[i]/r_buffer[i] <= 1 && (r_buffer[i]> 100000 && ir_buffer[i]> 100000)){
        j++;
      }
    }
    for(int i=0;i<100;i++){
      if(r_buffer[i]>ir_buffer[i] && ir_buffer[i]/r_buffer[i] <= 1 && (r_buffer[i]> 100000 && ir_buffer[i]> 100000)){
        r_avg += (r_buffer[i]/j);
        ir_avg += (ir_buffer[i]/j);
      }
    }
    if(r_avg != 0 && ir_avg != 0){
      Serial.print(r_avg);
      Serial.print("\t");
      Serial.print(ir_avg);   
      Serial.println();
    }

}
