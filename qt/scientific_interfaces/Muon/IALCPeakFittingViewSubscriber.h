// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once
#include "DllConfig.h"

namespace MantidQt::CustomInterfaces {

class MANTIDQT_MUONINTERFACE_DLL IALCPeakFittingViewSubscriber {

public:
  virtual ~IALCPeakFittingViewSubscriber() = default;

  virtual void onFitRequested() = 0;

  virtual void onCurrentFunctionChanged() = 0;

  virtual void onPeakPickerChanged() = 0;

  virtual void onParameterChanged(std::string const &, std::string const &) = 0;

  virtual void onPlotGuessClicked() = 0;
};

} // namespace MantidQt::CustomInterfaces
