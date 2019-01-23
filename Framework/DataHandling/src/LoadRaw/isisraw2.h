// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef ISISRAW2_H
#define ISISRAW2_H

#include "isisraw.h"

/// isis raw file.
//  isis raw
class ISISRAW2 : public ISISRAW {
public:
  ISISRAW2();
  ~ISISRAW2() override;

  int ioRAW(FILE *file, bool from_file, bool read_data = true) override;

  void skipData(FILE *file, int i);
  bool readData(FILE *file, int i);
  void clear();

  int ndes; ///< ndes
private:
  char *outbuff; ///< output buffer
  int m_bufferSize;
};

#endif /* ISISRAW2_H */
