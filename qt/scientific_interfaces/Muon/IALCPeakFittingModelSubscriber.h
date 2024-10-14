// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

namespace MantidQt::CustomInterfaces {

class MANTIDQT_MUONINTERFACE_DLL IALCPeakFittingModelSubscriber {

public:
  virtual ~IALCPeakFittingModelSubscriber() = default;

  virtual void dataChanged() const = 0;

  virtual void fittedPeaksChanged() const = 0;

  virtual void errorInModel(const std::string &) const = 0;
};

} // namespace MantidQt::CustomInterfaces
