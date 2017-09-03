#include "ltc.h"
#include <FlexiTimer2.h>

const static byte outLTCpin = 4;
const static byte intLTCclock = 7;
const static byte inLTC = 3;
const static byte TCEnableLED = 5;

LTCFrame gFrame;
//LTCFrame gInFrame;
volatile byte inLTCbuf[10];

unsigned char gOutBipNum;// ( 0 uptp 160 )
bool gbOutBipBit;// High or Low
volatile bool gbOutFrameCountUp;
volatile bool gSending;

void OnInterrupt();

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  ltc_frame_reset(&gFrame);
  //  ltc_frame_reset(&gInFrame);

  gOutBipNum = 0;
  gbOutBipBit = false;
  gSending = false;


  pinMode(outLTCpin, OUTPUT);

  pinMode(TCEnableLED, OUTPUT);
  Serial.begin(115200);
  Serial1.begin(9600);

  //FlexiTimer2::set(1, 1 / 4800.0, OnInterrupt );
  //FlexiTimer2::start();

  //pinMode(intLTCclock, INPUT_PULLUP);
  pinMode(inLTC, INPUT);
  attachInterrupt(digitalPinToInterrupt(inLTC), OnInterruptReader, CHANGE );
  //attachInterrupt(digitalPinToInterrupt(intLTCclock), OnInterrupt, RISING  );

  gSending = true;
}

// modify gbOutBipBit by sendig bit of gOutBipNum
void SetCurrentOutputBit()
{
  //base clock(nBit = 0, 2, 4...158) always reverse output;
  if ( (gOutBipNum  & 1) == 0 ) {
    gbOutBipBit = !gbOutBipBit ;
    return ;
  }

  //base clock + 1/2. change on/off by value
  const char bytepos = gOutBipNum >> 4 ;
  const char bitpos = (gOutBipNum >> 1 ) & 7;
  const unsigned char curBitValue =  reinterpret_cast<unsigned char*>(&gFrame)[ bytepos ] >> bitpos;
  // bit  on -> reverse output
  if ( curBitValue & 1) {
    gbOutBipBit = !gbOutBipBit ;
  }
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
          memcpy( inLTCbuf, ltc_frame, 10 );
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
    Serial.print("S");
  }

}
//4.8KHz Inteerupt( 80bit*30fps*2(biphase encoding) )
void OnInterrupt()
{
  if ( !gSending ) {
    //OH.We cant send.

    return ;
  }
  SetCurrentOutputBit();
  digitalWrite( outLTCpin, gbOutBipBit ? HIGH : LOW );
  gOutBipNum++;

  if ( gOutBipNum == 160 ) {
    ltc_frame_increment(&gFrame, 30, 0);
    gbOutFrameCountUp = true;
    gOutBipNum = 0;
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
     digitalWrite(TCEnableLED, LOW);   // turn the LED on (HIGH is the voltage level)
    cc = 0;
  }
  if ( bInSync ) {
      digitalWrite(TCEnableLED, HIGH);   // turn the LED on (HIGH is the voltage level)
  }

  if (Serial1.available() > 0) {
    // read the incoming byte:
    int incomingByte = Serial1.read();
    LTCFrame f;
    memcpy( &f,  inLTCbuf, 10 );
    if ( incomingByte == 2 ) {
      char buf[32];
      int di = Serial1.readBytesUntil(';', buf, 32);
      buf[di] = 0;
      char ltvf[16];
      ltc_frame_format( ltvf, &f);

      Serial.write( ltvf );
//      Serial.print("(");
//      Serial.print(intNum, DEC);
//      Serial.print(")");

      Serial.print( " = " );
      Serial.write(buf, di);
      Serial.println("");
    }

  }

  //halt();
  // put your main code here, to run repeatedly:
  //OnInterrupt();
}
