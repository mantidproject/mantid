// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

enum DataType {
  WIDTH,
  EISF,
  ALL,
};

class IFQFitObserver {
public:
  virtual ~IFQFitObserver() = default;
  virtual void updateDataType(DataType) = 0;
  virtual void spectrumChanged(int) = 0;
};
