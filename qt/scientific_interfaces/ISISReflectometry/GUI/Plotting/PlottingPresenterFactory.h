// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IPlottingPresenter.h"
#include "IPlottingView.h"
#include "PlottingPresenter.h"

#include <memory>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class PlottingPresenterFactory {
public:
  std::unique_ptr<IPlottingPresenter> make(IPlottingView *view) { return std::make_unique<PlottingPresenter>(view); }
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
