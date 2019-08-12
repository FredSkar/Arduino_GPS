#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

#define GPSSerial Serial1

Adafruit_AlphaNum4 alpha4 = Adafruit_AlphaNum4();

uint32_t timer = millis();
double _speed = 0;
char c_speed[5] = {0};

char c;
int lineindex = 0;
char in_buff[100] = {0};
char *RMCBuff;
bool RMC_av = false;
char RMCValue[100] = {0};
char *GGABuff;
char GGAValue[100] = {0};
bool GGA_av = false;
//char *VTGBuff;
//char VTGValue[100] = {0};

bool comma_index = false;
char c_checksum[1] = {0};
char tot_sum;
int csum_index = 0;
char lat[20] = {0};
char lon[20] = {0};
char time_val[20] = {0};
char display_buff[10] = {0};

double kmh_speed = 0;

bool gps_av = false;

File myFile;

void setup()
{
  Serial.println("GPS echo test");
  Serial.begin(9600);
  GPSSerial.begin(9600); // default NMEA GPS baud
  alpha4.begin(0x70);
  alpha4.clear();
  if (!SD.begin(10)){
    Serial.println("SD init failed!");
  }
  myFile = SD.open("log.txt", FILE_WRITE);

}

bool checksum(char string[100], char csum)
{
  char sum = {0};
  int _tot_sum = 0;
  for (int i = 1; i < strlen(string) - 5; i++)
  {
    sum ^= string[i];
    //Serial.print(string[i]);
  }

  //_tot_sum = atoi(sum, HEX);
  if (strcmp(sum, csum) == 0)
  {
    return true;
  }
  else
  {
    return false;
  }
}

unsigned int parse_char(char c)
{
  if ('0' <= c && c <= '9')
    return c - '0';
  if ('a' <= c && c <= 'f')
    return 10 + c - 'a';
  if ('A' <= c && c <= 'F')
    return 10 + c - 'A';

  abort();
}

int quad_disp(char s, bool dot) {
  int display_sum = 0;

  if (s == '0') {
    display_sum = 0b0000000000111111;
  }
  else if (s == '1') {
    display_sum = 0b0000000000000110;
  }
  else if (s == '2') {
    display_sum = 0b0000000011011011;
  }
  else if (s == '3') {
    display_sum = 0b0000000011001111;
  }
  else if (s == '4') {
    display_sum = 0b0000000011100110;
  }
  else if (s == '5') {
    display_sum = 0b0000000011101101;
  }
  else if (s == '6') {
    display_sum = 0b0000000011111101;
  }
  else if (s == '7') {
    display_sum = 0b0000000000000111;
  }
  else if (s == '8') {
    display_sum = 0b0000000011111111;
  }
  else if (s == '9') {
    display_sum = 0b0000000011101111;
  }

  if (dot) {
    display_sum = display_sum | 0b0100000000000000;
  }
  return display_sum;
}

void data_parsing()
{
  bool b_lat = false;
  bool b_lon = false;
  bool b_time = false;
  bool b_speed = false;
  String s_speed = "";

  int s_index = 0;
  int comma_count = 0;

  float speed = 0.0;
  RMC_av = false;

  for (int i = 0; i <= strlen(RMCValue); i++)
  {
    if (RMCValue[i] == ',')
    {
      if (comma_count == 0 && b_time == false)
      {
        b_time = true;
      }
      else if (comma_count == 1)
      {
        //TODO: log value to file
        //Serial.println(time_val);
        b_time = false;
        s_index = 0;
        memset(time_val, 0, sizeof(time_val));
      }
      if (comma_count == 2 && b_lat == false)
      {
        b_lat = true;
      }
      else if (comma_count == 4)
      {
        //TODO: log value to SD-card
        //Serial.println(lat);
        b_lat = false;
        s_index = 0;
        memset(lat, 0, sizeof(lat));
      }
      if (comma_count == 4 && b_lon == false)
      {
        b_lon = true;
      }
      else if (comma_count == 6)
      {
        //TODO: log value to SD-card
        //Serial.println(lon);
        b_lon = false;
        s_index = 0;
        memset(lon, 0, sizeof(lon));
      }
      if (comma_count == 6 && b_speed == false)
      {
        b_speed = true;
      }
      else if (comma_count == 7)
      {
        c_speed[0] = '0';
        s_speed = c_speed;
        speed = s_speed.toFloat();
        kmh_speed = 1.852 * speed;
        //kmh_speed = random(1, 10000)/10; //for test
        dtostrf(kmh_speed, 3, 1, display_buff);
        //_snprintf(display_buff, sizeof(kmh_speed), "Speed: %g", kmh_speed);
        //Serial.println(kmh_speed);
        //Serial.println(c_speed);
        b_speed = false;
        s_index = 0;
        memset(c_speed, 0, sizeof(c_speed));
      }
      comma_count++;
    }
    if (b_time == true)
    {
      time_val[s_index] = RMCValue[i];
      s_index++;
    }
    else if (b_lat == true)
    {
      lat[s_index] = RMCValue[i];
      s_index++;
    }
    else if (b_lon == true)
    {
      lon[s_index] = RMCValue[i];
      Serial.println(lon);
      s_index++;
    }
    else if (b_speed == true)
    {
      c_speed[s_index] = RMCValue[i];
      s_index++;
    }
  }
}

void loop()
{
  while (GPSSerial.available())
  {
    c = GPSSerial.read();
    //Serial.print(c);
    if (c == '$')
    {
      lineindex = 0;
    }
    in_buff[lineindex] = c;
    lineindex++;
    if (c == '\n')
    {
      RMCBuff = strstr(in_buff, "$GPRMC");
      GGABuff = strstr(in_buff, "$GPGGA");
      if (RMCBuff != NULL)
      {
        for (int i = 0; i <= sizeof(in_buff); i++)
        {
          RMCValue[i] = in_buff[i];
          if (in_buff[i] == '\n')
          {
            break;
          }
          else if (in_buff[i] == '*')
          {
            csum_index = i;
          }
        }
        c_checksum[0] = in_buff[csum_index + 1];
        c_checksum[1] = in_buff[csum_index + 2];
        tot_sum = parse_char(c_checksum[0]) * 0x10 + parse_char(c_checksum[1]);
        if (checksum(RMCValue, tot_sum) == false)
        {
          memset(RMCValue, NULL, sizeof(RMCValue));
          RMC_av = false;
          //Serial.println("Wrong checksum... Clear value.");
        }
        else
        {
          RMC_av = true;
        }
        //Serial.print("RMC value found: ");
        //Serial.println(RMCValue);
      }
      if (GGABuff != NULL){
        for (int i = 0; i <= sizeof(in_buff); i++)
        {
          GGAValue[i] = in_buff[i];
          if (in_buff[i] == '\n')
          {
            break;
          }
          else if (in_buff[i] == '*')
          {
            csum_index = i;
          }
        }
        c_checksum[0] = in_buff[csum_index + 1];
        c_checksum[1] = in_buff[csum_index + 2];
        tot_sum = parse_char(c_checksum[0]) * 0x10 + parse_char(c_checksum[1]);
        if (checksum(GGAValue, tot_sum) == false)
        {
          memset(GGAValue, NULL, sizeof(GGAValue));
          GGA_av = false;
          //Serial.println("Wrong checksum... Clear value.");
        }
        else
        {
          GGA_av = true;
        }
      }
    }
  }
  if (GGA_av){
    int comma_count = 0;
    for(int i=0; i<=sizeof(GGAValue); i++){
      
      if(GGAValue[i] == ','){
        comma_count++;
      }
      if(comma_count == 6){
        //Serial.print(GGAValue);
        //Serial.print("GPS lock: ");
        //Serial.println(GGAValue[i+1]);
        //Serial.println(i);
        gps_av = (bool) GGAValue[i+1];
        break;
      }
    }
    memset(GGAValue, NULL, sizeof(GGAValue));
    GGA_av = false;
    
  }
  if (RMC_av)
  {
    data_parsing();
  }
  if(timer > millis()) timer = millis();

  if(gps_av == false){
    kmh_speed = 0.0;
    dtostrf(kmh_speed, 3, 1, display_buff);
  }

  if(millis() - timer > 1500){
    timer = millis();
    //Serial.println("Update display...");
    alpha4.clear();
    if(kmh_speed < 10.0){
      alpha4.writeDigitRaw(0, quad_disp('0', false));
      alpha4.writeDigitRaw(1, quad_disp('0', false));
      alpha4.writeDigitRaw(2, quad_disp(display_buff[0], true));
      alpha4.writeDigitRaw(3, quad_disp(display_buff[2], false));
    }
    else if(kmh_speed >= 10.0 && kmh_speed < 100.0){
      alpha4.writeDigitRaw(0, quad_disp('0', false));
      alpha4.writeDigitRaw(1, quad_disp(display_buff[0], false));
      alpha4.writeDigitRaw(2, quad_disp(display_buff[1], true));
      alpha4.writeDigitRaw(3, quad_disp(display_buff[3], false));
    }
    else if(kmh_speed >= 100.0){
      alpha4.writeDigitRaw(0, quad_disp(display_buff[0], false));
      alpha4.writeDigitRaw(1, quad_disp(display_buff[1], false));
      alpha4.writeDigitRaw(2, quad_disp(display_buff[2], true));
      alpha4.writeDigitRaw(3, quad_disp(display_buff[4], false));
    }
    
    alpha4.writeDisplay();
    //Serial.print("Display buff: ");
    //Serial.println(display_buff);    

  }
  //alpha4.writeDigitAscii(0, 7);

  myFile = SD.open("log.txt", FILE_WRITE);
  myFile.write("GPS locK: ");
  myFile.print((int)gps_av);
  myFile.write("\r\n");
  //Serial.println(gps_av);

  myFile.write("Speed: ");
  myFile.print(kmh_speed);
  myFile.write("\r\n");
  //Serial.println(kmh_speed);

  myFile.write("Display buff: ");
  myFile.print(display_buff);
  myFile.write("\r\n");
  //Serial.println(display_buff);

  myFile.write("Latitude: ");
  myFile.print(lat);
  myFile.write("\r\n");
  //Serial.println(lat);

  myFile.write("Longitude: ");
  myFile.print(lon);
  myFile.write("\r\n");
  //Serial.println(lon);

  myFile.close();

}
