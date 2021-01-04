#include "ltc.h"
#include <stdlib.h>
#include <string.h>

void ltc_frame_format( char fmt[12], const LTCFrame *f)
{
 fmt[0] = f->hours_tens + '0';
 fmt[1] = f->hours_units + '0';
 fmt[2] = ':';
 fmt[3] = f->mins_tens  + '0';
 fmt[4] = f->mins_units + '0';
 fmt[5] = ':';
 fmt[6] = f->secs_tens  + '0';
 fmt[7] = f->secs_units + '0';
 fmt[8] = '.';
 fmt[9] = f->frame_tens  + '0';
 fmt[10] = f->frame_units + '0';
 fmt[11] = 0;
}

void ltc_frame_reset(LTCFrame* frame)
{
  memset(frame, 0, sizeof(LTCFrame));
  // syncword = 0x3FFD
#ifdef LTC_BIG_ENDIAN
  // mirrored BE bit order: FCBF
  frame->sync_word = 0xFCBF;
#else
  // mirrored LE bit order: BFFC
  frame->sync_word = 0xBFFC;
#endif
}


static void skip_drop_frames(LTCFrame* frame) {
  if ((frame->mins_units != 0)
      && (frame->secs_units == 0)
      && (frame->secs_tens == 0)
      && (frame->frame_units == 0)
      && (frame->frame_tens == 0)
     ) {
    frame->frame_units += 2;
  }
}
void ltc_frame_set_parity(LTCFrame *frame, int fps) {
  int i;
  unsigned char p = 0;

  if (fps != 25) { /* 30fps, 24fps */
    frame->biphase_mark_phase_correction = 0;
  } else { /* 25fps */
    frame->binary_group_flag_bit2 = 0;
  }

  for (i = 0; i < LTC_FRAME_BIT_COUNT / 8; ++i) {
    p = p ^ (((unsigned char*)frame)[i]);
  }
#define PRY(BIT) ((p>>BIT)&1)

  if (fps != 25) { /* 30fps, 24fps */
    frame->biphase_mark_phase_correction =
      PRY(0)^PRY(1)^PRY(2)^PRY(3)^PRY(4)^PRY(5)^PRY(6)^PRY(7);
  } else { /* 25fps */
    frame->binary_group_flag_bit2 =
      PRY(0)^PRY(1)^PRY(2)^PRY(3)^PRY(4)^PRY(5)^PRY(6)^PRY(7);
  }
}


int ltc_frame_increment(LTCFrame* frame, int fps, int flags) {
  int rv = 0;

  frame->frame_units++;

  if (frame->frame_units == 10)
  {
    frame->frame_units = 0;
    frame->frame_tens++;
  }
  if (fps == frame->frame_units + frame->frame_tens * 10)
  {
    frame->frame_units = 0;
    frame->frame_tens = 0;
    frame->secs_units++;
    if (frame->secs_units == 10)
    {
      frame->secs_units = 0;
      frame->secs_tens++;
      if (frame->secs_tens == 6)
      {
        frame->secs_tens = 0;
        frame->mins_units++;
        if (frame->mins_units == 10)
        {
          frame->mins_units = 0;
          frame->mins_tens++;
          if (frame->mins_tens == 6)
          {
            frame->mins_tens = 0;
            frame->hours_units++;
            if (frame->hours_units == 10)
            {
              frame->hours_units = 0;
              frame->hours_tens++;
            }
            if (frame->hours_units == 4 && frame->hours_tens == 2)
            {
              /* 24h wrap around */
              rv = 1;
              frame->hours_tens = 0;
              frame->hours_units = 0;
#if 0
              if (flags & 1)
              {
                /* wrap date */
                SMPTETimecode stime;
                stime.years  = frame->user5 + frame->user6 * 10;
                stime.months = frame->user3 + frame->user4 * 10;
                stime.days   = frame->user1 + frame->user2 * 10;

                if (stime.months > 0 && stime.months < 13)
                {
                  unsigned char dpm[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
                  /* proper leap-year calc:
                     ((stime.years%4)==0 && ( (stime.years%100) != 0 || (stime.years%400) == 0) )
                     simplified since year is 0..99
                  */
                  if ((stime.years % 4) == 0 /* && stime.years!=0 */ ) /* year 2000 was a leap-year */
                    dpm[1] = 29;
                  stime.days++;
                  if (stime.days > dpm[stime.months - 1])
                  {
                    stime.days = 1;
                    stime.months++;
                    if (stime.months > 12) {
                      stime.months = 1;
                      stime.years = (stime.years + 1) % 100;
                    }
                  }
                  frame->user6 = stime.years / 10;
                  frame->user5 = stime.years % 10;
                  frame->user4 = stime.months / 10;
                  frame->user3 = stime.months % 10;
                  frame->user2 = stime.days / 10;
                  frame->user1 = stime.days % 10;
                } else {
                  rv = -1;
                }
              }
#endif
            }
          }
        }
      }
    }
  }

  if (frame->dfbit) {
    skip_drop_frames(frame);
  }

  if ((flags & LTC_NO_PARITY) == 0) {
    ltc_frame_set_parity(frame, fps);
  }

  return rv;
}
