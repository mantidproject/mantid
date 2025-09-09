// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plotting/PlotWidget/PlotPresenter.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

using Mantid::API::MatrixWorkspace_sptr;

namespace MantidQt::MantidWidgets {
PlotPresenter::PlotPresenter(IPlotView *view, std::unique_ptr<PlotModel> model)
    : m_view(view), m_model(std::move(model)) {
  if (!m_model) {
    m_model = std::make_unique<PlotModel>();
  }
}

void PlotPresenter::clearModel() { m_model->clear(); }

void PlotPresenter::setSpectrum(const Mantid::API::MatrixWorkspace_sptr &ws, const size_t wsIndex) {
  m_model->setSpectrum(ws, wsIndex);
}

void PlotPresenter::setScaleLinear(const AxisID axisID) { m_view->setScaleLinear(axisID); }

void PlotPresenter::setScaleLog(const AxisID axisID) { m_view->setScaleLog(axisID); }

void PlotPresenter::setScaleSymLog(const AxisID axisID, const double linthresh) {
  m_view->setScaleSymLog(axisID, linthresh);
}

void PlotPresenter::setAxisLimit(const AxisID axisID, const double axMin, const double axMax) {
  m_view->setAxisLimit(axisID, axMin, axMax);
}

void PlotPresenter::setPlotErrorBars(const bool plotErrorBars) { m_model->setPlotErrorBars(plotErrorBars); }

void PlotPresenter::plot() {
  m_view->plot(m_model->getWorkspaces(), m_model->getWorkspaceIndices(), m_model->getPlotErrorBars());
}
} // namespace MantidQt::MantidWidgets
