/**
   @file ltc.h
   @brief libltc - en+decode linear timecode

   Linear (or Longitudinal) Timecode (LTC) is an encoding of
   timecode data as a Manchester-Biphase encoded audio signal.
   The audio signal is commonly recorded on a VTR track or other
   storage media.

   libltc facilitates decoding and encoding of LTC from/to
   timecode, including SMPTE date support.

   @author Robin Gareus <robin@gareus.org>
   @copyright

   Copyright (C) 2006-2014 Robin Gareus <robin@gareus.org>

   Copyright (C) 2008-2009 Jan Weiß <jan@geheimwerk.de>

   Inspired by SMPTE Decoder - Maarten de Boer <mdeboer@iua.upf.es>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.
   If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef LTC_H
#define LTC_H 1

struct LTCFrame {
	unsigned int frame_units:4; ///< SMPTE framenumber BCD unit 0..9
	unsigned int user1:4;

	unsigned int frame_tens:2; ///< SMPTE framenumber BCD tens 0..3
	unsigned int dfbit:1; ///< indicated drop-frame timecode
	unsigned int col_frame:1; ///< colour-frame: timecode intentionally synchronized to a colour TV field sequence
	unsigned int user2:4;

	unsigned int secs_units:4; ///< SMPTE seconds BCD unit 0..9
	unsigned int user3:4;

	unsigned int secs_tens:3; ///< SMPTE seconds BCD tens 0..6
	unsigned int biphase_mark_phase_correction:1; ///< see note on Bit 27 in description and \ref ltc_frame_set_parity .
	unsigned int user4:4;

	unsigned int mins_units:4; ///< SMPTE minutes BCD unit 0..9
	unsigned int user5:4;

	unsigned int mins_tens:3; ///< SMPTE minutes BCD tens 0..6
	unsigned int binary_group_flag_bit0:1; ///< indicate user-data char encoding, see table above - bit 43
	unsigned int user6:4;

	unsigned int hours_units:4; ///< SMPTE hours BCD unit 0..9
	unsigned int user7:4;

	unsigned int hours_tens:2; ///< SMPTE hours BCD tens 0..2
	unsigned int binary_group_flag_bit1:1; ///< indicate timecode is local time wall-clock, see table above - bit 58
	unsigned int binary_group_flag_bit2:1; ///< indicate user-data char encoding (or parity with 25fps), see table above - bit 59
	unsigned int user8:4;

	unsigned int sync_word:16;
};

typedef struct LTCFrame LTCFrame;
/** the standard defines the assignment of the binary-group-flag bits
 * basically only 25fps is different, but other standards defined in
 * the SMPTE spec have been included for completeness.
 */
enum LTC_TV_STANDARD {
	LTC_TV_525_60, ///< 30fps
	LTC_TV_625_50, ///< 25fps
	LTC_TV_1125_60,///< 30fps
	LTC_TV_FILM_24 ///< 24fps
};

/** encoder and LTCframe <> timecode operation flags */
enum LTC_BG_FLAGS {
	LTC_USE_DATE  = 1, ///< LTCFrame <> SMPTETimecode converter and LTCFrame increment/decrement use date, also set BGF2 to '1' when encoder is initialized or re-initialized (unless LTC_BGF_DONT_TOUCH is given)
	LTC_TC_CLOCK  = 2,///< the Timecode is wall-clock aka freerun. This also sets BGF1 (unless LTC_BGF_DONT_TOUCH is given)
	LTC_BGF_DONT_TOUCH = 4, ///< encoder init or re-init does not touch the BGF bits (initial values after initialization is zero)
	LTC_NO_PARITY = 8 ///< parity bit is left untouched when setting or in/decrementing the encoder frame-number
};


static const int LTC_FRAME_BIT_COUNT = 80;

void ltc_frame_reset(LTCFrame* frame) ;
int ltc_frame_increment(LTCFrame* frame, int fps, int flags);
void ltc_frame_set_parity(LTCFrame *frame, int fps) ;
void ltc_frame_format( char fmt[12], const LTCFrame *f);

#endif
