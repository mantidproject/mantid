// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/*
C
C Compression of 32bit integer data into byte-relative format
C
C Each integer is stored relative to the previous value in byte form.The first
C is relative to zero. This allows for numbers to be within + or - 127 of
C the previous value. Where a 32bit integer cannot be expressed in this way a
C special byte code is used (-128) and the full 32bit integer stored elsewhere.
C The final space used is (NIN-1)/4 +1 + NEXTRA longwords, where NEXTRA is the
C number of extra longwords used in giving absolute values.
C
*/
#include <iostream>
#include <stdexcept>

#include "byte_rel_comp.h"

#define LARGE_NUMBER 1073741824

#define FAILURE 1
#define SUCCESS 0

// We aren't going to EVER change this code - so let's ignore the warnings.
#if defined(__GNUC__) && !(defined(__INTEL_COMPILER))
#pragma GCC diagnostic ignored "-Wconversion"
#elif defined(_WIN32)
#pragma warning(disable : 4100)
#endif

int byte_rel_comp(const int *data_in, int n_in, char *data_out, int max_out, int &n_out) {
  int i, icurrent, irel;
  union {
    int i;
    char c[4];
  } byte_pack;
  if (n_in <= 0) {
    throw std::runtime_error("byte rel comp error: nin <= 0");
  }
  if (max_out <= n_in) {
    throw std::runtime_error("byte rel comp error: nin <= 0");
  }
  n_out = 0;
  icurrent = 0;
  for (i = 0; i < n_in; i++) {

    // Trap out ridiculously large numbers. They could cause problems in
    // subtraction,
    // so force them to be stored absolutely
    if ((data_in[i] > LARGE_NUMBER) || (data_in[i] < -LARGE_NUMBER) || (icurrent > LARGE_NUMBER) ||
        (icurrent < -LARGE_NUMBER)) {
      irel = 128; // Force absolute mode
    } else {
      irel = data_in[i] - icurrent; // Calc relative offset
    }
    // If small put it in a byte
    if ((irel <= 127) && (irel >= -127)) {
      if (n_out > max_out) {
        throw std::runtime_error("byte rel comp error: nin <= 0");
      }
      data_out[n_out] = irel; // Pack relative byte
      n_out++;
    } else {
      // Otherwise put marker in byte followed by packed 32bit integer
      if (n_out + 4 >= max_out) {
        throw std::runtime_error("byte rel comp error: nin <= 0");
      }
      data_out[n_out] = -128; // pack marker
      byte_pack.i = data_in[i];
      data_out[n_out + 1] = byte_pack.c[0];
      data_out[n_out + 2] = byte_pack.c[1];
      data_out[n_out + 3] = byte_pack.c[2];
      data_out[n_out + 4] = byte_pack.c[3];
      n_out += 5;
    }
    icurrent = data_in[i];
  }
  return SUCCESS;
}

/*
C
C Expansion of byte-relative format into 32bit format
C
C Each integer is stored relative to the previous value in byte form.The first
C is relative to zero. This allows for numbers to be within + or - 127 of
C the previous value. Where a 32bit integer cannot be expressed in this way a
C special byte code is used (-128) and the full 32bit integer stored elsewhere.
C The final space used is (NIN-1)/4 +1 + NEXTRA longwords, where NEXTRA is the
C number of extra longwords used in giving absolute values.
C
C
C Type definitions
C   Passed parameters
        INTEGER NIN
        BYTE    INDATA(NIN)
C                               Array of NIN compressed bytes
        INTEGER NFROM
C                               pass back data from this 32bit word onwards
C
        INTEGER NOUT
C                               Number of 32bit words expected
        INTEGER OUTDATA(NOUT)
C                               outgoing expanded data
        INTEGER ISTATUS
C                               Status return
C                                      =1  no problems!
C                                      =3  NOUT .lt.NIN/5
C                                      =2  NIN .le.0
C                                      =4  NOUT .gt.NIN
C                                      =6  number of channels lt NOUT
*/
// n_from is zero based
int byte_rel_expn(const char *data_in, int n_in, int n_from, int *data_out, int n_out) {
  int i, j;
  union {
    int i;
    char c[4];
  } byte_pack;
  // First check no slip-ups in the input parameters
  if (n_in <= 0) {
    throw std::runtime_error("byte rel comp error: nin <= 0");
  }
  if (n_out + n_from > n_in) {
    throw std::runtime_error("byte rel comp error: nin <= 0");
  }
  // Set initial absolute value to zero and channel counter to zero
  byte_pack.i = 0;
  j = 0;
  // Loop over all expected 32bit integers
  for (i = 0; i < n_from + n_out; i++) {
    if (j >= n_in) {
      throw std::runtime_error("byte rel comp error: nin <= 0");
    }
    // if number is contained in a byte
    if (data_in[j] != -128) {
      // add in offset to base
      byte_pack.i += data_in[j];
      j++;
    } else {
      // Else skip marker and pick up new absolute value
      // check there are enough bytes
      if (j + 4 >= n_in) {
        throw std::runtime_error("byte rel comp error: nin <= 0");
      }
      // unpack 4 bytes
      byte_pack.c[0] = data_in[j + 1];
      byte_pack.c[1] = data_in[j + 2];
      byte_pack.c[2] = data_in[j + 3];
      byte_pack.c[3] = data_in[j + 4];
      //		CALL VAX_TO_LOCAL_INTS(ITEMP, 1)
      j += 5;
    }
    // update current value
    if (i >= n_from) {
      data_out[i - n_from] = byte_pack.i;
    }
  }

  // expansion OK, but excessive number of bytes given to the routine
  if (n_out < n_in / 5) {
    std::cerr << "byte rel expn: excessive bytes\n";
  }
  return SUCCESS;
}
