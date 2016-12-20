

/* El modulo Bluetooth HC_05 esta configurado
    como RELOJ_1,PW 1234 y a 115200 b.
*/


#include <SoftwareSerial.h>
#include <TimerOne.h>
#include <Wire.h> //I2C communication library
#include <OneWire.h>
#include "ds3231.h" //Real Time Clock library
#include "TM1637.h"



#define CLK 2//pins definitions for TM1637 and can be changed to other ports    
#define DIO 3
#define ON 1
#define OFF 0

const int LED = 13;
const int BUZZER = 7;
const int BTN_AL_OFF = 6;

int8_t TimeDisp[] = {0x00, 0x00, 0x00, 0x00 , 0x00 ,0x00};
unsigned char ClockPoint = 1;
bool Update = false;
bool bVisTemperatura = false;

unsigned char second = 0;
unsigned char minuto = 0;
unsigned char hora = 12;
unsigned char sec_alarma = 0;
unsigned char minuto_alarma = 0;
unsigned char hora_alarma = 0;
int beep_count = 0;
bool ALARM_ON_OFF = false;

bool toggle = false;
int16_t i = 0;
String strDato = "";
String aux = "";
bool _DEBUG_ = true;
float Temperatura;
bool bFlag5Seg = false;
bool tarea = false;


OneWire  ds1820(10);
TM1637 tm1637(CLK, DIO);
SoftwareSerial BT(8, 9); //RX TX

void setup() {
  // put your setup code here, to run once:

  BT.begin(115000);
  Serial.begin(9600);
  Wire.begin(); //Initialize I2C communication library
  DS3231_init(DS3231_INTCN); //Initialize Real Time Clock for 1Hz square wave output (no RTC alarms on output pin)
  pinMode(LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(BTN_AL_OFF, INPUT); //Alarma OFF


  digitalWrite(BUZZER, LOW);


  tm1637.set(BRIGHTEST);//BRIGHT_TYPICAL = 2,BRIGHT_DARKEST = 0,BRIGHTEST = 7;
  tm1637.init();

  Temperatura = leeTemperatura();

  Timer1.initialize(500000);//timing for 500ms
  Timer1.attachInterrupt(TimingISR);//declare the interrupt serve routine:TimingISR
}

void loop() {
  // put your main code here, to run repeatedly:

  if (bFlag5Seg)
  {
    tarea = !tarea;
    bFlag5Seg = false;
  }

  if (!digitalRead(BTN_AL_OFF)) { //Reset alarm flag if set button pressed

    delay(25); //25ms debounce time
    if (!digitalRead(BTN_AL_OFF)) {
      digitalWrite(BUZZER, LOW);
      tm1637.set(BRIGHTEST);
      DS3231_clear_a1f();
      DS3231_clear_a2f();

    } //Clear alarm flag if set button pressed - insures alarm reset when turning alarm on
  }

  if (Update == ON  )
  {
    TimeUpdate();

    switch (tarea) {

      case 0:  if (! DS3231_triggered_a1())
                     tm1637.set(BRIGHTEST);

                   tm1637.display(TimeDisp);
                   break;
      case 1:  if (! DS3231_triggered_a1())
                          tm1637.set(0);
                      
                 // tm1637.display(Temperatura);
                   displayTemperatura();
                   break ;

      default: break;
    }


    if (ALARM_ON_OFF && DS3231_triggered_a1()) {


      if (++beep_count < 60) { //aprox minuto y medio
        digitalWrite(BUZZER, HIGH);


        if (!toggle)
          tm1637.set(0);
        else
          tm1637.set(BRIGHTEST);

      }
      else
      {
        digitalWrite(BUZZER, LOW);
        tm1637.set(BRIGHTEST);
        beep_count = 0;
        DS3231_clear_a1f();
        DS3231_clear_a2f();
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
        ALARM_ON_OFF = true;
        digitalWrite(LED, ON);


      }
      else if (strDato.startsWith("B")) {
        tm1637.set(BRIGHTEST);
        ALARM_ON_OFF = false;
        DS3231_clear_a1f();
        DS3231_clear_a2f();
        digitalWrite(BUZZER, OFF);
        digitalWrite(LED, OFF);


        if (_DEBUG_)
          Serial.print("Alarma Borrada\r\n");
      }

      else if (strDato.startsWith("T")) {
         BT.write(Temperatura);
         Serial.println(Temperatura);
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
float leeTemperatura() {

  byte data[12];
  byte addr[8];
  byte i;
  byte present = 0;
  float celsius;


  if ( !ds1820.search(addr)) {

    if (_DEBUG_) {
      Serial.println("No direcciones de  1820");
      Serial.println();
    }
    ds1820.reset_search();
    delay(250);
    return 0.0;
  }

  if (_DEBUG_) {

    Serial.print("ROM =");
    for ( i = 0; i < 8; i++) {
      Serial.write(' ');
      Serial.print(addr[i], HEX);
    }
  }

  ds1820.reset();
  ds1820.select(addr);
  ds1820.write(0x44, 1);
  delay(700);
  present = ds1820.reset();
  ds1820.select(addr);
  ds1820.write(0xBE);

  if (_DEBUG_) {
    Serial.print("  Data = ");
    Serial.print(present, HEX);
    Serial.print(" ");
  }
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds1820.read();

    if (_DEBUG_) {
      Serial.print(data[i], HEX);
      Serial.print(" ");
    }
  }

  int16_t raw = (data[1] << 8) | data[0];
  raw = raw << 3;
  if (data[7] == 0x10) {
    // "count remain" gives full 12 bit resolution
    raw = (raw & 0xFFF0) + 12 - data[6];
  }

  celsius = (float)raw / 16.0;
  //celsius -= 1.0;

  return (celsius);

}


//*******************************************************************************************

void TimingISR()
{
  static byte cntTemp = 0;
  static byte cntTiempo = 0;
  static byte cntVisTemperatura = 0;

  Update = ON;

  if (++cntTemp > 2)
  {

    toggle = !toggle;
    // digitalWrite(LED, toggle);
    cntTemp = 0;
  }

  if (++cntTiempo > 15) {

    bFlag5Seg = true;
    cntTiempo = 0;
  }

  if (++cntVisTemperatura > 30) {

    Temperatura = leeTemperatura();
    cntVisTemperatura = 0;
  }

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
  TimeDisp[4] = t.sec / 10;
  TimeDisp[5] = t.sec % 10;

  if( t.min==0 && t.sec==0 ){
      digitalWrite(BUZZER,HIGH);
      delay(50);
      digitalWrite(BUZZER,LOW);
      delay(50);
      digitalWrite(BUZZER,HIGH);
      delay(50);
      digitalWrite(BUZZER,LOW);
  }

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
void displayTemperatura(void) {
  char buffer[6];
  int8_t data[] = {0x00, 0x00, 0x00, 0x00};

  dtostrf(Temperatura, 2, 1, buffer);

   if (_DEBUG_) {
      Serial.println(buffer);
      
    }
 
  data[0] = buffer[0]-0x30;
  data[1] = buffer[1]-0x30;
  data[2] = buffer[3]-0x30;
  data[3] = 12;
  
  tm1637.set_decpoint(1);
  tm1637.display(data);
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
void borra_Alarma() {
  byte flags[5] = { 1, 1, 1, 1, 1 }; //Set alarm to trigger every 24 hours on time match

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


