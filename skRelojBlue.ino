/* El modulo Bluetooth HC_05 esta configurado
    como RELOJ_1,PW 1234 y a 115200 b.
*/


#include <SoftwareSerial.h>
#include <TimerOne.h>
#include <Wire.h> //I2C communication library
#include "ds3231.h" //Real Time Clock library
#include "TM1637.h"



#define CLK 2//pins definitions for TM1637 and can be changed to other ports    
#define DIO 3
#define ON 1
#define OFF 0

const int LED = 13;

int8_t TimeDisp[] = {0x00, 0x00, 0x00, 0x00};
unsigned char ClockPoint = 1;
bool Update = false;
unsigned char halfsecond = 0;
unsigned char second = 0;
unsigned char minuto = 0;
unsigned char hora = 12;
unsigned char sec_alarma = 0;
unsigned char minuto_alarma = 0;
unsigned char hora_alarma = 0;
int beep_count = 0;

bool toggle = false;
int16_t i = 0;
String strDato = "";
String aux = "";
bool _DEBUG_ = true;
float Temperatura;

TM1637 tm1637(CLK, DIO);
SoftwareSerial BT(8, 9); //RX TX

void setup() {
  // put your setup code here, to run once:

  BT.begin(115000);
  Serial.begin(9600);
  Wire.begin(); //Initialize I2C communication library
  DS3231_init(DS3231_INTCN); //Initialize Real Time Clock for 1Hz square wave output (no RTC alarms on output pin)
  pinMode(LED, OUTPUT);

  tm1637.set(BRIGHTEST);//BRIGHT_TYPICAL = 2,BRIGHT_DARKEST = 0,BRIGHTEST = 7;
  tm1637.init();

  Timer1.initialize(500000);//timing for 500ms
  Timer1.attachInterrupt(TimingISR);//declare the interrupt serve routine:TimingISR
}

void loop() {
  // put your main code here, to run repeatedly:

  if (Update == ON)
  {
    TimeUpdate();
    tm1637.display(TimeDisp);


    if (DS3231_triggered_a1()) {


      if (++beep_count < 120) { //aprox minuto y medio

        if (!toggle)
          tm1637.set(0);
        else
          tm1637.set(BRIGHTEST);

      }
      else
      {
        tm1637.set(BRIGHTEST);
        beep_count = 0;
        hora_alarma = 0;
        minuto_alarma = 0;
        DS3231_clear_a1f();

      }



      if (_DEBUG_)

        Serial.print("!!!!!!!!!!ALARMA !!!!!!!!!!!!\r\n");

    }//if disparo alarma

    if (BT.available() > 0) {


      // read the incoming byte:
      strDato = BT.readString();

      if ( ! strDato.length() && strDato.length() > 9) {
        if (_DEBUG_)
          Serial.print("Incorrecto: " + strDato + "\r\n");
        return;
      }

      if (strDato.startsWith("A"))
      {


        if (strDato.length() == 5) {

          hora_alarma = strDato.substring(1, 2).toInt();
          minuto_alarma = strDato.substring(3).toInt();
        }
        else if (strDato.length() == 6) {
          hora_alarma = strDato.substring(1, 3).toInt();
          minuto_alarma = strDato.substring(4).toInt();

        }
        if (_DEBUG_) {
          Serial.print("Hora alarma: ");
          Serial.print(hora_alarma);
          Serial.print(":");
          Serial.print(minuto_alarma);
          Serial.print("\r\n");
        }

        setAlarma();
        displayAlarma();

      }
      else if (strDato.startsWith("B")) {
        tm1637.set(BRIGHTEST);
        hora_alarma = 0;
        minuto_alarma = 0;
        sec_alarma = 0;
        DS3231_clear_a1f();

        if (_DEBUG_)
          Serial.print("Alarma Borrada\r\n");
      }
      else {
        struct ts t;

        if (strDato.length() == 7) {
          t.sec = strDato.substring(6, 7).toInt();
          t.min = strDato.substring(3, 5).toInt();
          t.hour = strDato.substring(0, 2).toInt();

        }
        else if (strDato.length() == 8) {
          t.sec = strDato.substring(6, 8).toInt();
          t.min = strDato.substring(3, 5).toInt();
          t.hour = strDato.substring(0, 2).toInt();

        }

        DS3231_set(t);

      }


      // say what you got:
      if (_DEBUG_)
        Serial.print("Recibido: " + strDato + "\r\n");


    }// if (BT.available() > 0)

  }//if (Update == ON)


}

//***************************************************************************************

void TimingISR()
{
  static byte cntTemp = 0;
  char tempF[6];

  halfsecond ++;
  Update = ON;



  if (++cntTemp > 2)
  {
    //Temperatura = DS3231_get_treg();
  
   // dtostrf(Temperatura, 5, 1, tempF);
   // Serial.println(tempF);
    toggle = !toggle;
    digitalWrite(LED, toggle);
    cntTemp = 0;
  }


  // Serial.println(second);
}
//********************************************************************************
void TimeUpdate(void)
{
  struct ts t;
  Update = OFF;
  int aux;

  DS3231_get(&t);

  // if (ClockPoint)tm1637.point(POINT_ON);
  //  else tm1637.point(POINT_OFF);
  tm1637.set_decpoint(1);

  TimeDisp[0] = t.hour / 10;
  TimeDisp[1] = t.hour % 10;
  TimeDisp[2] = t.min / 10;
  TimeDisp[3] = t.min % 10;

}
//*******************************************************************************
void displayAlarma(void)
{
  Timer1.stop();
  tm1637.set(0);
  Update = OFF;
  TimeDisp[0] = hora_alarma / 10;
  TimeDisp[1] = hora_alarma % 10;
  TimeDisp[2] = minuto_alarma / 10;
  TimeDisp[3] = minuto_alarma % 10;
  tm1637.display(TimeDisp);
  delay(1000 * 4);
  tm1637.set(BRIGHTEST);
  Timer1.restart();

}
//********************************************************************************
void setAlarma()
{

  // flags define what calendar component to be checked against the current time in order
  // to trigger the alarm - see datasheet
  // A1M1 (seconds) (0 to enable, 1 to disable)
  // A1M2 (minutes) (0 to enable, 1 to disable)
  // A1M3 (hour)    (0 to enable, 1 to disable)
  // A1M4 (day)     (0 to enable, 1 to disable)
  // DY/DT          (dayofweek == 1/dayofmonth == 0)
  byte flags[5] = { 0, 0, 0, 1, 1 }; //Set alarm to trigger every 24 hours on time match

  // set Alarm1
  DS3231_set_a1(0, minuto_alarma, hora_alarma, 0, flags); //Set alarm 1 RTC registers

}
//************************************************************************************
void get_alarm()
{
  uint8_t n[4];
  uint8_t t[4];               //second,minute,hour,day
  uint8_t f[5];               // flags
  uint8_t i;

  Wire.beginTransmission(DS3231_I2C_ADDR);
  Wire.write(DS3231_ALARM1_ADDR);
  Wire.endTransmission();

  Wire.requestFrom(DS3231_I2C_ADDR, 4);

  for (i = 0; i <= 3; i++) {
    n[i] = Wire.read();
    f[i] = (n[i] & 0x80) >> 7;
    t[i] = bcdtodec(n[i] & 0x7F);
  }

  f[4] = (n[3] & 0x40) >> 6;
  t[3] = bcdtodec(n[3] & 0x3F);

  sec_alarma = t[0];
  minuto_alarma = t[1];
  hora_alarma = t[2];
}


