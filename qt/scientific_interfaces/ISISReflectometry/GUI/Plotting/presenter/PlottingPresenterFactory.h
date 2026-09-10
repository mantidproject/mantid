// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "GUI/Common/PlotOptionsProvider.h"
#include "GUI/Common/Plotter.h"
#include "GUI/Plotting/model/PlottingModel.h"
#include "GUI/Plotting/presenter/IPlottingPresenter.h"
#include "GUI/Plotting/view/IPlottingView.h"
#include "PlottingPresenter.h"

#include <memory>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

/// Factory that wires the plotting presenter to its default collaborators.
class PlottingPresenterFactory {
public:
  /// Create a plotting presenter for the supplied view.
  std::unique_ptr<IPlottingPresenter> make(IPlottingView *view) {
    return std::make_unique<PlottingPresenter>(view, m_plotter, m_plotOptionsProvider, m_plottingModel);
  }

private:
  Plotter m_plotter;
  PlotOptionsProvider m_plotOptionsProvider;
  PlottingModel m_plottingModel;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
