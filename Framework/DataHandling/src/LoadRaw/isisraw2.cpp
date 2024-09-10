// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "isisraw2.h"
#include "byte_rel_comp.h"
#include <cstdio>
#include <exception>

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Logger.h"
#include <boost/lexical_cast.hpp>

namespace {
/// static logger
Mantid::Kernel::Logger g_log("ISISRAW2");
} // namespace

/// No arg Constructor
ISISRAW2::ISISRAW2() : ISISRAW(nullptr, false), ndes(0), outbuff(nullptr), m_bufferSize(0) {
  // Determine the size of the output buffer to create from the config service.
  g_log.debug("Determining ioRaw buffer size\n");
  auto bufferSizeConfigVal = Mantid::Kernel::ConfigService::Instance().getValue<int>("loadraw.readbuffer.size");

  if (!bufferSizeConfigVal.has_value()) {
    m_bufferSize = 200000;
    g_log.debug() << "loadraw.readbuffer.size not found, setting to " << m_bufferSize << "\n";
  } else {
    m_bufferSize = bufferSizeConfigVal.value();
    g_log.debug() << "loadraw.readbuffer.size set to " << m_bufferSize << "\n";
  }
}

/** Loads the headers of the file, leaves the file pointer at a specific
 *position
 *   @param file :: The file handle to use
 *   @param from_file :: Whether to read from or write to a file
 *   @param read_data :: Whether to go on to read the data
 *   @return file readin exit code, 0 is OK
 **/
int ISISRAW2::ioRAW(FILE *file, bool from_file, bool read_data) {
  (void)read_data; // Avoid compiler warning

  int i;
  fpos_t add_pos, dhdr_pos;
  if (!from_file) {
    add.ad_run = 32;
    add.ad_inst = add.ad_run + 94;
    add.ad_se = add.ad_inst + 70 + 2 * i_mon + (5 + i_use) * i_det;
    add.ad_dae = add.ad_se + 66 + e_nse * 32;
    add.ad_tcb = add.ad_dae + 65 + 5 * i_det;
    add.ad_user = add.ad_tcb + 288 + (t_ntc1 + 1);
    add.ad_data = add.ad_user + 2 + u_len;
    add.ad_log = 0; // we don't know it yet
    add.ad_end = 0;
  }
  ISISRAW::ioRAW(file, &hdr, 1, from_file);
  ISISRAW::ioRAW(file, &frmt_ver_no, 1, from_file);
  fgetpos(file, &add_pos);
  ISISRAW::ioRAW(file, &add, 1, from_file);
  ISISRAW::ioRAW(file, &data_format, 3, from_file);
  ISISRAW::ioRAW(file, r_title, 80, from_file);
  ISISRAW::ioRAW(file, &user, 1, from_file);
  ISISRAW::ioRAW(file, &rpb, 1, from_file);
  ISISRAW::ioRAW(file, &ver3, 1, from_file);
  ISISRAW::ioRAW(file, i_inst, 8, from_file);
  ISISRAW::ioRAW(file, &ivpb, 1, from_file);
  ISISRAW::ioRAW(file, &i_det, 3, from_file);
  ISISRAW::ioRAW(file, &mdet, i_mon, from_file);
  ISISRAW::ioRAW(file, &monp, i_mon, from_file);
  ISISRAW::ioRAW(file, &spec, i_det, from_file);
  ISISRAW::ioRAW(file, &delt, i_det, from_file);
  ISISRAW::ioRAW(file, &len2, i_det, from_file);
  ISISRAW::ioRAW(file, &code, i_det, from_file);
  ISISRAW::ioRAW(file, &tthe, i_det, from_file);
  ISISRAW::ioRAW(file, &ut, i_use * i_det, from_file);
  ISISRAW::ioRAW(file, &ver4, 1, from_file);
  ISISRAW::ioRAW(file, &spb, 1, from_file);
  ISISRAW::ioRAW(file, &e_nse, 1, from_file);
  ISISRAW::ioRAW(file, &e_seblock, e_nse, from_file);
  ISISRAW::ioRAW(file, &ver5, 1, from_file);
  ISISRAW::ioRAW(file, &daep, 1, from_file);
  ISISRAW::ioRAW(file, &crat, i_det, from_file);
  ISISRAW::ioRAW(file, &modn, i_det, from_file);
  ISISRAW::ioRAW(file, &mpos, i_det, from_file);
  ISISRAW::ioRAW(file, &timr, i_det, from_file);
  ISISRAW::ioRAW(file, &udet, i_det, from_file);
  ISISRAW::ioRAW(file, &ver6, 267, from_file);
  ISISRAW::ioRAW(file, &(t_tcp1[0][0]), 20, from_file);
  ISISRAW::ioRAW(file, &t_pre1, 1, from_file);
  ISISRAW::ioRAW(file, &t_tcb1, t_ntc1 + 1, from_file);
  ISISRAW::ioRAW(file, &ver7, 1, from_file);
  // it appear that the VMS ICP traditionally sets u_len to 1 regardless
  // of its real size; thus we cannot rely on it for reading and must instead
  // use section offsets
  i = 0;
  ISISRAW::ioRAW(file, &i, 1, from_file);
  if (from_file) {
    u_len = add.ad_data - add.ad_user - 2;

    if (u_len < 0 || (add.ad_data < add.ad_user + 2)) {
      // this will/would be used for memory allocation
      g_log.error() << "Error in u_len value read from file, it would be " << u_len
                    << "; where it is calculated as "
                       "u_len = ad_data - ad_user - 2, where ad_data: "
                    << add.ad_data << ", ad_user: " << add.ad_user << "\n";
      throw std::runtime_error("Inconsistent value for the field u_len found");
    }
  }

  ISISRAW::ioRAW(file, &u_dat, u_len, from_file);
  ISISRAW::ioRAW(file, &ver8, 1, from_file);
  fgetpos(file, &dhdr_pos);
  ISISRAW::ioRAW(file, &dhdr, 1, from_file);

  if (!outbuff)
    outbuff = new char[m_bufferSize];
  ndes = t_nper * (t_nsp1 + 1);
  ISISRAW::ioRAW(file, &ddes, ndes, true);
  if (!dat1)
    dat1 = new uint32_t[t_ntc1 + 1]; //  space for just one spectrum
  // so when we round up words we get a zero written
  memset(outbuff, 0, m_bufferSize);

  return 0; // stop reading here
}

/// Skip data
/// @param file :: The file pointer
/// @param i :: The amount of data to skip
void ISISRAW2::skipData(FILE *file, int i) {
  if (i < ndes) {
    int zero = fseek(file, 4 * ddes[i].nwords, SEEK_CUR);
    if (0 != zero) {
      g_log.warning() << "Failed to skip data from file, with value: " << i << "\n";
    }
  }
}

/// Read data
/// @param file :: The file pointer
/// @param i :: The amount of data to read
/// @return true on success
bool ISISRAW2::readData(FILE *file, int i) {
  if (i >= ndes)
    return false;
  int nwords = 4 * ddes[i].nwords;
  if (nwords > m_bufferSize) {
    g_log.debug() << "Overflow error, nwords > buffer size. nwords = " << nwords << ", buffer=" << m_bufferSize << "\n";
    throw std::overflow_error("LoadRaw input file buffer too small for "
                              "selected data. Try increasing the "
                              "\"loadraw.readbuffer.size\" user property.");
  }
  int res = ISISRAW::ioRAW(file, outbuff, nwords, true);
  if (res != 0)
    return false;
  byte_rel_expn(outbuff, nwords, 0, reinterpret_cast<int *>(dat1), t_ntc1 + 1);
  return true;
}

ISISRAW2::~ISISRAW2() {
  if (outbuff)
    delete[] outbuff;
}

/// Clears the output buffer
void ISISRAW2::clear() {
  if (outbuff)
    delete[] outbuff;
}
