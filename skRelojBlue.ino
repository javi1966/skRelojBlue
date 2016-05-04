
/* El modulo Bluetooth HC_05 esta configurado
 *  como RELOJ_1,PW 1234 y a 115200 b.
 */
#include <SoftwareSerial.h>
#include <TimerOne.h>
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
unsigned char minuto_alarma = 0;
unsigned char hora_alarma = 0;
bool toggle = false;
int16_t i = 0;
String strDato = "";
String aux = "";
bool _DEBUG_ = true;


TM1637 tm1637(CLK, DIO);
SoftwareSerial BT(8, 9); //RX TX




void setup() {
  // put your setup code here, to run once:

  BT.begin(115000);
  Serial.begin(9600);
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

    if(hora == hora_alarma && minuto == minuto_alarma)
      if(_DEBUG_)
        Serial.print("!!!!!!!!!!ALARMA !!!!!!!!!!!!");

  }

  if (BT.available() > 0) {
    // read the incoming byte:
    strDato = BT.readString();

    if (strDato.length() < 4 || strDato.length() > 6) {
      if (_DEBUG_)
        Serial.print("Incorrecto: " + strDato + "\n");
      return;
    }

    if (strDato.startsWith("A"))
    {
      if (_DEBUG_)
        Serial.print("Hora alarma: " );

      if (strDato.length() == 5) {

        hora_alarma = strDato.substring(1, 2).toInt();
        minuto_alarma = strDato.substring(3).toInt();
      }
      else if (strDato.length() == 6) {
        hora_alarma = strDato.substring(1, 3).toInt();
        minuto_alarma = strDato.substring(4).toInt();

      }
      if (_DEBUG_) {
        Serial.print(hora_alarma);
        Serial.print(minuto_alarma);
      }
      displayAlarma();
    }
     else if(strDato.startsWith("B")){

        hora_alarma = 0;
        minuto_alarma = 0;
        
        if (_DEBUG_) 
            Serial.print("Alarma Borrada");
        
      
     }

    else {

      if (strDato.length() == 4) {
        minuto = strDato.substring(2, 4).toInt();
        hora = strDato.substring(0, 2).toInt();
      }
      else if (strDato.length() == 5) {

        minuto = strDato.substring(3, 5).toInt();
        hora = strDato.substring(0, 2).toInt();
      }
    }


    // say what you got:
    if (_DEBUG_)
      Serial.print("Recibido: " + strDato + "\n");


  }


}

//***************************************************************************************

void TimingISR()
{
  halfsecond ++;
  Update = ON;
  toggle = !toggle;
  digitalWrite(LED, toggle);


  if (halfsecond == 2) {

    second ++;
    if (second == 60)
    {
      minuto ++;
      if (minuto == 60)
      {
        hora ++;
        if (hora == 24)hora = 0;
        minuto = 0;
      }
      second = 0;
    }
    halfsecond = 0;
  }
  // Serial.println(second);


}
//********************************************************************************
void TimeUpdate(void)
{
  Update = OFF;
  int aux;

  // if (ClockPoint)tm1637.point(POINT_ON);
  //  else tm1637.point(POINT_OFF);
  tm1637.set_decpoint(1);

  TimeDisp[0] = hora / 10;
  TimeDisp[1] = hora % 10;
  TimeDisp[2] = minuto / 10;
  TimeDisp[3] = minuto % 10;

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
  delay(1000 * 5);
  tm1637.set(BRIGHTEST);
  Timer1.restart();

}


