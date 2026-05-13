// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class MANTIDQT_ISISREFLECTOMETRY_DLL PlottingViewSubscriber {
public:
  virtual ~PlottingViewSubscriber() = default;
};

class MANTIDQT_ISISREFLECTOMETRY_DLL IPlottingView {
public:
  virtual ~IPlottingView() = default;
  virtual void subscribe(PlottingViewSubscriber *notifyee) = 0;
  virtual void setOutputOptionsEnabled(bool enabled) = 0;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
