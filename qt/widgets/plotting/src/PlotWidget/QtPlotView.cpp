// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plotting/PlotWidget/QtPlotView.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/MplCpp/Plot.h"

#include <QVBoxLayout>
#include <QWidget>
#include <vector>

using Mantid::API::MatrixWorkspace_sptr;
using namespace MantidQt::Widgets::Common;
using namespace MantidQt::Widgets::MplCpp;

namespace MantidQt::MantidWidgets {
QtPlotView::QtPlotView(QWidget *parent) : QWidget(parent), m_canvas(new FigureCanvasQt(111, "", parent)) {
  createLayout();
}

void QtPlotView::setScaleLinear(const AxisID axisID) {
  switch (axisID) {
  case AxisID::XBottom:
    m_axisProperties[QString("xscale")] = QVariant("linear");
    break;
  case AxisID::YLeft:
    m_axisProperties[QString("yscale")] = QVariant("linear");
    break;
  }
}

void QtPlotView::setScaleLog(const AxisID axisID) {
  switch (axisID) {
  case AxisID::XBottom:
    m_axisProperties[QString("xscale")] = QVariant("log");
    break;
  case AxisID::YLeft:
    m_axisProperties[QString("yscale")] = QVariant("log");
    break;
  }
}

void QtPlotView::plot(const std::vector<Mantid::API::MatrixWorkspace_sptr> &workspaces,
                      const std::vector<int> &workspaceIndices, const bool plotErrorBars) {
  Widgets::MplCpp::plot(workspaces, std::nullopt, workspaceIndices, m_canvas->gcf().pyobj(), std::nullopt,
                        m_axisProperties, std::nullopt, plotErrorBars, false);
}

void QtPlotView::createLayout() {
  auto plotLayout = new QVBoxLayout(this);
  plotLayout->setContentsMargins(0, 0, 0, 0);
  plotLayout->setSpacing(0);
  plotLayout->addWidget(m_canvas, 0);
  setLayout(plotLayout);
}
} // namespace MantidQt::MantidWidgets
