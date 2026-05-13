// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "GUI/Common/PlotOptionsProvider.h"
#include "GUI/Common/Plotter.h"
#include "IPlottingPresenter.h"
#include "IPlottingView.h"
#include "PlottingPresenter.h"

#include <memory>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class PlottingPresenterFactory {
public:
  std::unique_ptr<IPlottingPresenter> make(IPlottingView *view) {
    return std::make_unique<PlottingPresenter>(view, m_plotter, m_plotOptionsProvider);
  }

private:
  Plotter m_plotter;
  PlotOptionsProvider m_plotOptionsProvider;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
