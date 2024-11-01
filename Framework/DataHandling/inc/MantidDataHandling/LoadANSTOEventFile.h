// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cstdio>
#include <stdexcept>
#include <stdint.h>

namespace Mantid {
namespace DataHandling {
namespace ANSTO {

// The function reads a binary ANSTO event file,
// It opens the file and returns the data through the callbacks.

constexpr int32_t EVENTFILEHEADER_BASE_MAGIC_NUMBER = 0x0DAE0DAE;
constexpr int32_t EVENTFILEHEADER_BASE_FORMAT_NUMBER = 0x00010002;

// all events contain some or all of these fields
constexpr int32_t NVAL = 5; // x, y, v, w, wa

#pragma pack(push, 1) // otherwise may get 8 byte aligned, no good for us

struct EventFileHeader_Base { // total content should be 16*int (64 bytes)
  int32_t magic_number;       // must equal EVENTFILEHEADER_BASE_MAGIC_NUMBER (DAE data)
  int32_t format_number;      // must equal EVENTFILEHEADER_BASE_FORMAT_NUMBER,
                              // identifies this header format
  // cppcheck-suppress unusedStructMember
  int32_t anstohm_version; // ANSTOHM_VERSION server/filler version number that
                           // generated the file
  int32_t pack_format;     // typically 0 if packed binary, 1 if unpacked binary.
  int32_t oob_enabled;     // if set, OOB events can be present in the data,
                           // otherwise only neutron and t0 events are stored
  int32_t clock_scale;     // the CLOCK_SCALE setting, ns per timestamp unit
  // cppcheck-suppress unusedStructMember
  int32_t spares[16 - 6]; // spares (padding)
};

struct EventFileHeader_Packed { // total content should be 16*int (64 bytes)
  int32_t evt_stg_nbits_x;      // number of bits in x datum
  int32_t evt_stg_nbits_y;      // number of bits in y datum
  int32_t evt_stg_nbits_v;      // number of bits in v datum
  int32_t evt_stg_nbits_w;      // number of bits in w datum
  int32_t evt_stg_nbits_wa;     // number of bits in wa datum // MJL added 5/15 for
                                // format 0x00010002
  int32_t evt_stg_xy_signed;    // 0 if x and y are unsigned, 1 if x and y are
                                // signed ints
  // cppcheck-suppress unusedStructMember
  int32_t spares[16 - 6]; // spares (padding)
};

#pragma pack(pop)

// event decoding state machine
enum event_decode_state {
  // for all events
  DECODE_START, // initial state - then DECODE_VAL_BITFIELDS (for neutron
                // events) or DECODE_OOB_BYTE_1 (for OOB events) for OOB events
                // only
  DECODE_OOB_BYTE_1,
  DECODE_OOB_BYTE_2,
  // for all events
  DECODE_VAL_BITFIELDS,
  DECODE_DT // final state - then output data and return to DECODE_START
};

/*
Types of OOB events, and 'NEUTRON' event.  Not all are used for all
instruments, or supported yet.

NEUTRON = 0 = a neutron detected, FRAME_START = -2 = T0 pulse (e.g. from
chopper, or from Doppler on Emu).  For most instruments, these are the only
types used.

FRAME_AUX_START = -3 (e.g. from reflecting chopper on Emu), VETO = -6 (e.g.
veto signal from ancillary)

BEAM_MONITOR = -7 (e.g. if beam monitors connected direct to Mesytec MCPD8 DAE)

RAW = -8 = pass-through, non-decoded raw event directly from the DAE (e.g.
Mesytec MCPD8).  Used to access special features of DAE.

Other types are not used in general (DATASIZES = -1 TBD in future, FLUSH = -4
deprecated, FRAME_DEASSERT = -5 only on Fastcomtec P7888 DAE).
*/

template <class IReader, class IEventHandler, class IProgress>
void ReadEventFile(IReader &loader, IEventHandler &handler, IProgress &progress, int32_t def_clock_scale,
                   bool use_tx_chopper) {
  // read file headers (base header then packed-format header)
  EventFileHeader_Base hdr_base;
  if (!loader.read(reinterpret_cast<char *>(&hdr_base), sizeof(hdr_base)))
    throw std::runtime_error("unable to load EventFileHeader-Base");

  EventFileHeader_Packed hdr_packed;
  if (!loader.read(reinterpret_cast<char *>(&hdr_packed), sizeof(hdr_packed)))
    throw std::runtime_error("unable to load EventFileHeader-Packed");

  if (hdr_base.magic_number != EVENTFILEHEADER_BASE_MAGIC_NUMBER)
    throw std::runtime_error("bad magic number");

  if (hdr_base.format_number > EVENTFILEHEADER_BASE_FORMAT_NUMBER) {
    char txtBuffer[255] = {};
    snprintf(txtBuffer, sizeof(txtBuffer), "invalid file (only format_number=%08Xh or lower)",
             EVENTFILEHEADER_BASE_FORMAT_NUMBER);
    throw std::runtime_error(txtBuffer);
  }

  if (hdr_base.pack_format != 0)
    throw std::runtime_error("only packed binary format is supported");

  if (hdr_base.clock_scale == 0)
    throw std::runtime_error("clock scale cannot be zero");

  // note: in the old format 0x00010001, the evt_stg_nbits_wa did not exist and
  // it contained evt_stg_xy_signed
  if (hdr_base.format_number <= 0x00010001) {
    hdr_packed.evt_stg_xy_signed = hdr_packed.evt_stg_nbits_wa;
    hdr_packed.evt_stg_nbits_wa = 0;
  }

  // Setup the clock_scale.  In format 0x00010001 this was not part of the
  // headers, hence a function argument is provided to allow it to be
  // specified manually. In the current format 0x00010002, clock_scale is
  // written to the header and need not be specified, unless some alternate
  // scale is needed.
  double scale_microsec = hdr_base.clock_scale / 1000.0;
  if (!hdr_base.clock_scale) {
    // old eventfile format did not have clock_scale...
    scale_microsec = def_clock_scale / 1000.0;
  }

  // the initial time is not set correctly so wait until primary and auxillary
  // time have been reset before sending events
  int64_t primary_time = 0;
  int64_t auxillary_time = 0;
  bool primary_ok = false;
  bool auxillary_ok = false;

  // main loop
  uint32_t x = 0, y = 0, v = 0, w = 0,
           wa = 0;                                 // storage for event data fields
  uint32_t *ptr_val[NVAL] = {&x, &y, &v, &w, &wa}; // used to store data into fields

  // All events are also timestamped.  The differential timestamp dt stored in
  // each event is summed to recover the event timestamp t. All timestamps are
  // frame-relative, i.e. FRAME_START event represents T0 (e.g. from a chopper)
  // and t is reset to 0. In OOB mode and for certain DAE types only (e.g.
  // Mesytec MCPD8), the FRAME_START event is timestamped relative to the last
  // FRAME_START. The timestamp t on the FRAME_START event is therefore the
  // total frame duration, and this can be used to recover the absolute
  // timestamp of all events in the DAQ, if desired (e.g. for accurate timing
  // during long term kinematic experiments).
  int32_t dt = 0; // , t = 0 dt may be negative occasionally for some DAE types,
                  // therefore dt and t are signed ints.

  int32_t nbits_val_oob[NVAL] = {};

  int32_t nbits_val_neutron[NVAL] = {hdr_packed.evt_stg_nbits_x, hdr_packed.evt_stg_nbits_y, hdr_packed.evt_stg_nbits_v,
                                     hdr_packed.evt_stg_nbits_w, hdr_packed.evt_stg_nbits_wa};

  int32_t ind_val = 0;
  int32_t nbits_val = 0;
  int32_t nbits_val_filled = 0;
  int32_t nbits_dt_filled = 0;

  int32_t oob_en = hdr_base.oob_enabled; // will be 1 if we are reading a new OOB
                                         // event file (format 0x00010002 only).
  int32_t oob_event = 0,
          c = 0; // For neutron events, oob_event = 0, and for OOB
                 // events, oob_event = 1 and c indicates the OOB
                 // event type. c<0 for all OOB events currently.

  event_decode_state state = DECODE_START; // event decoding state machine
  bool event_ended = false;

  while (true) {

    // read next byte
    uint8_t ch;
    if (!loader.read(reinterpret_cast<char *>(&ch), 1))
      break;

    int32_t nbits_ch_used = 0; // no bits used initially, 8 to go

    // start of event processing
    if (state == DECODE_START) {

      // if OOB event mode is enabled, the leading Bit 0 of the first byte
      // indicates whether the event is a neutron event or an OOB event
      if (!oob_en)
        state = DECODE_VAL_BITFIELDS;
      else {
        oob_event = (ch & 1);
        nbits_ch_used = 1; // leading bit used as OOB bit

        if (!oob_event)
          state = DECODE_VAL_BITFIELDS;
        else
          state = DECODE_OOB_BYTE_1;
      }

      // setup to decode new event bitfields (for both neutron and OOB events)
      for (ind_val = 0; ind_val < NVAL; ind_val++)
        *ptr_val[ind_val] = 0;

      ind_val = 0;
      nbits_val_filled = 0;

      dt = 0;
      nbits_dt_filled = 0;
    }

    // state machine for event decoding
    switch (state) {
    case DECODE_START: // Should never get here
      throw std::runtime_error("Failure in event decoding");
    case DECODE_OOB_BYTE_1: // first OOB header byte
                            // OOB event Byte 1:  Bit 0 = 1 = OOB event, Bit 1 =
                            // mode (only mode=0 suported currently), Bits 2-5 =
                            // c (OOB event type), Bits 6-7 = bitfieldsize_x
                            // / 8. bitfieldsize_x and following 2-bit
                            // bitfieldsizes are the number of bytes used to
                            // store the OOB parameter. All of x,y,v,w,wa are
                            // short integers (16 bits maximum) and so
                            // bitfieldsizes = 0, 1 or 2 only.
      c = (ch >> 2) & 0xF;  // Bits 2-5 = c

      if (c & 0x8)
        c |= 0xFFFFFFF0;                   // c is a signed parameter so sign extend - OOB events
                                           // are negative values
      nbits_val_oob[0] = (ch & 0xC0) >> 3; // Bits 6-7 * 8 = bitfieldsize_x

      state = DECODE_OOB_BYTE_2; // Proceed to process second OOB event header
                                 // byte next time
      break;

    case DECODE_OOB_BYTE_2:                // second OOB header byte
                                           // bitfieldsizes for y, v, w and wa, as for
                                           // bitfieldsize_x above.
      nbits_val_oob[1] = (ch & 0x03) << 3; // Bits 0-1 * 8 = bitfieldsize_y
      nbits_val_oob[2] = (ch & 0x0C) << 1; // Bits 2-3 * 8 = bitfieldsize_v
      nbits_val_oob[3] = (ch & 0x30) >> 1; // Bits 4-5 * 8 = bitfieldsize_w
      nbits_val_oob[4] = (ch & 0xC0) >> 3; // Bits 6-7 * 8 = bitfieldsize_wa

      state = DECODE_VAL_BITFIELDS; // Proceed to read and store x,y,v,w,wa for
                                    // the OOB event
      break;

    case DECODE_VAL_BITFIELDS:
      // fill bits of the incoming ch to the event's bitfields.
      // stop when we've filled them all, or all bits of ch are used.
      do {
        nbits_val = (oob_event ? nbits_val_oob[ind_val] : nbits_val_neutron[ind_val]);
        if (!nbits_val) {
          nbits_val_filled = 0;
          ind_val++;
        } else {
          int32_t nbits_val_to_fill = (nbits_val - nbits_val_filled);
          if ((8 - nbits_ch_used) >= nbits_val_to_fill) {
            *ptr_val[ind_val] |= ((ch >> nbits_ch_used) & ((1 << nbits_val_to_fill) - 1)) << nbits_val_filled;
            nbits_val_filled = 0;
            nbits_ch_used += nbits_val_to_fill;
            ind_val++;
          } else {
            *ptr_val[ind_val] |= (ch >> nbits_ch_used) << nbits_val_filled;
            nbits_val_filled += (8 - nbits_ch_used);
            nbits_ch_used = 8;
          }
        }
      } while ((ind_val < NVAL) && (nbits_ch_used < 8));

      //
      if (ind_val == NVAL)
        state = DECODE_DT; // and fall through for dt processing

      if (nbits_ch_used == 8) // read next byte
        break;

    case DECODE_DT:
      if ((8 - nbits_ch_used) <= 2) {
        dt |= (ch >> nbits_ch_used) << nbits_dt_filled;
        nbits_dt_filled += (8 - nbits_ch_used);
      } else if ((ch & 0xC0) == 0xC0) {
        dt |= ((ch & 0x3F) >> nbits_ch_used) << nbits_dt_filled;
        nbits_dt_filled += (6 - nbits_ch_used);
      } else {
        dt |= (ch >> nbits_ch_used) << nbits_dt_filled;
        nbits_dt_filled += (8 - nbits_ch_used);
        event_ended = true;
      }

      break;
    }

    if (event_ended) {
      state = DECODE_START; // start on new event next time

      // update times
      primary_time += dt;
      auxillary_time += dt;

      // is this event a frame_start? // FRAME_START is an OOB event when oob
      // mode enabled
      bool frame_start_event = (oob_en ? (oob_event && c == -2) : (x == 0 && y == 0 && dt == -1));

      if (oob_en || !frame_start_event) {
        if (oob_event) {
          if (c == -3) { // FRAME_AUX_START = -3
            // 0 is the reflecting chopper and 1 is the transmission chopper
            if (!use_tx_chopper && x == 0) {
              auxillary_time = 0;
              auxillary_ok = true;
            }
            if (use_tx_chopper && x == 1) {
              auxillary_time = 0;
              auxillary_ok = true;
            }
          }
        } else {
          // if times are ok pass the event trhough the call back, time units in
          // nsec
          if (primary_ok && auxillary_ok)
            handler.addEvent(x, y, static_cast<double>(primary_time) * scale_microsec,
                             static_cast<double>(auxillary_time) * scale_microsec);
        }
      }

      if (frame_start_event) {
        // reset timestamp at start of a new frame
        // the auxillary time is only available in OOB mode
        // otherwise, auxillary time = primary time
        primary_time = 0;
        primary_ok = true;
        if (!oob_en) {
          auxillary_time = 0;
          auxillary_ok = true;
        }
        handler.newFrame();
      }

      progress.update(loader.selected_position());

      event_ended = false;
    }
  }
}
} // namespace ANSTO
} // namespace DataHandling
} // namespace Mantid
