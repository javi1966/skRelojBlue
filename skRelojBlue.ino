#include <SoftwareSerial.h>
#include <TM1637.h>
#include <TimerOne.h>



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
bool toggle = false;
int16_t i = 0;
String strDato="";
String aux="";


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

  }

  if (BT.available() > 0) {
    // read the incoming byte:
   strDato = BT.readString();
   if(strDato.length() != 5)
      Serial.print("Incorrecto: "+strDato);

    minuto=strDato.substring(3,5).toInt();  
    hora=strDato.substring(0,2).toInt();
    
    
    // say what you got:
    Serial.print("Recibido: ");
    Serial.println(minuto);
    Serial.print(strDato);
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

  // if (ClockPoint)tm1637.point(POINT_ON);
  //  else tm1637.point(POINT_OFF);
  tm1637.set_decpoint(1);
  TimeDisp[0] = hora / 10;
  TimeDisp[1] = hora % 10;
  TimeDisp[2] = minuto / 10;
  TimeDisp[3] = minuto % 10;

}
