// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_IFQFITOBSERVER_H_
#define MANTIDQTCUSTOMINTERFACES_IFQFITOBSERVER_H_

enum DataType {
  WIDTH,
  EISF,
};

class IFQFitObserver {
public:
  virtual ~IFQFitObserver() = default;
  virtual void updateDataType(DataType) = 0;
  virtual void spectrumChanged(int) = 0;
};

#endif
