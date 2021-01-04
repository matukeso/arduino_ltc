
#include "ltc.h"

#define LEONARD 

#ifdef LEONARD
//SoftwareSerial toRasp(8,9);
#define toRasp Serial1
const static byte inLTC = 3;
const static byte TCEnableLED = 5;
#else
#include <SoftwareSerial.h>

#endif


LTCFrame gFrame;
//LTCFrame gInFrame;
volatile byte inLTCbuf[10];
unsigned char gOutBipNum;// ( 0 uptp 160 )
bool gbOutBipBit;// High or Low
volatile bool gbOutFrameCountUp;


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(TCEnableLED, OUTPUT);

  ltc_frame_reset(&gFrame);
  //  ltc_frame_reset(&gInFrame);

  gOutBipNum = 0;
  gbOutBipBit = false;
  

  Serial.begin(115200);
  toRasp.begin(9600);


  pinMode(inLTC, INPUT);
  attachInterrupt(digitalPinToInterrupt(inLTC), OnInterruptReader, CHANGE );
  
  digitalWrite(TCEnableLED, LOW);   // turn the LED on (HIGH is the voltage level)
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED on (HIGH is the voltage level)
}


volatile bool in_frame_compl = 0;

static const unsigned long short30 = 150;
static const unsigned long  long30 = 300;
static const int  ltc_timeout = 300 * 100;


static const unsigned short  sync_word = 0b0011111111111101;
static bool InCheckSync(byte ch)
{
  static unsigned short mysync = 0;

  mysync <<= 1;
  mysync |= ch;
  

  return sync_word == mysync;
}
char to1hex(byte hv ) {
  if ( hv < 10 )
    return hv + '0';
  return hv - 10 + 'A';
}


volatile static bool bInSync = false;
volatile static unsigned long prev_recv = 0;

void OnInterruptReader()
{
  static byte in_byte;
  static byte in_bit_pos = 1;
  static byte ltc_frame[10];
  static byte ltc_di;

  unsigned long time = micros();
  unsigned long dur =  time - prev_recv;
  if ( (int)dur > ltc_timeout ) {
    prev_recv = time;
    bInSync = false;
    ltc_di = 0;
    return ;
  }
  if ( dur > long30 ) {
    in_bit_pos <<= 1;
    prev_recv = time;
    if ( !bInSync ) {
      bInSync = InCheckSync( in_byte );
      in_bit_pos = 1;
      in_byte = 0;
      ltc_di = 0;
      return;
    }

    if ( in_bit_pos == 0 )
    {
      if(0){
            char hex[3] = {to1hex( in_byte >> 4),  to1hex(in_byte&0xf), 0};
            Serial.print(hex);  
      }
    
      in_bit_pos = 1;
      ltc_frame[ltc_di] = in_byte;
      ltc_di++;
      in_byte = 0;
      
      if ( ltc_di == 10 ) {
        ltc_di = 0;
        if( ltc_frame[9] == 0b10111111 ){
          memcpy( (byte*)&inLTCbuf[0], ltc_frame, 10 );
          in_frame_compl = true;
        }
        else{
          Serial.print("MIS=");
          for( int ii=0; ii<10; ii++){
            Serial.print(ltc_frame[ii], BIN);
            Serial.print(":");
          }
          
          bInSync = false;
          ltc_di = 0;
        }
      }
    }
  }
  else if ( dur > short30 ) {
    in_byte |= in_bit_pos;
  }
  else { 
 //   Serial.print("S");
  }

}

int cc = 0;
void loop() {
  unsigned long now = micros();

  if ( digitalRead( 7 ) == LOW ) {
    //    Serial.println(gOutBipNum, DEC );
    //    Serial.println(gFrame.frame_units, DEC );
  }
  if (bInSync && int( now - prev_recv) >  ltc_timeout ) {
    Serial.print("*DESYNC*");
    Serial.print(int(now - prev_recv));
    Serial.println("");
    prev_recv = now;
    bInSync = false;
    cc = 0;
  }
  if(in_frame_compl){
    byte ltc[10];
    memcpy( ltc, (byte*)&inLTCbuf[0],10);
    in_frame_compl = false;
    toRasp.write(ltc, 10);
  }

  if ( bInSync ) {
     digitalWrite(TCEnableLED, HIGH);   // turn the LED on (HIGH is the voltage level)
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  }else{
    digitalWrite(TCEnableLED, LOW);   // turn the LED on (HIGH is the voltage level)
    digitalWrite(LED_BUILTIN, LOW);   // turn the LED on (HIGH is the voltage level)
  }
}
