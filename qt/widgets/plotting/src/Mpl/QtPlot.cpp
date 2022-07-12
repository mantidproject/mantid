// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plotting/Mpl/QtPlot.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/MplCpp/Plot.h"

#include <QVBoxLayout>
#include <QWidget>

using Mantid::API::MatrixWorkspace_sptr;
using namespace MantidQt::Widgets::Common;
using namespace MantidQt::Widgets::MplCpp;

namespace MantidQt::MantidWidgets {
QtPlot::QtPlot(QWidget *parent) : QWidget(parent), m_canvas(new FigureCanvasQt(111, "", parent)) { createLayout(); }

void QtPlot::setXScaleType(const AxisScale axisScale) {
  switch (axisScale) {
  case AxisScale::LINEAR:
    m_axisProperties[QString("xscale")] = QVariant("linear");
    break;
  case AxisScale::LOG:
    m_axisProperties[QString("xscale")] = QVariant("log");
    break;
  }
  plot();
}

void QtPlot::setYScaleType(const AxisScale axisScale) {
  switch (axisScale) {
  case AxisScale::LINEAR:
    m_axisProperties[QString("yscale")] = QVariant("linear");
    break;
  case AxisScale::LOG:
    m_axisProperties[QString("yscale")] = QVariant("log");
    break;
  }
  plot();
}

void QtPlot::setSpectrum(const MatrixWorkspace_sptr &ws, const size_t wsIndex) {

  m_workspaces = std::vector<MatrixWorkspace_sptr>{ws};
  m_workspaceIndices = std::vector<int>{static_cast<int>(wsIndex)};

  plot();
}

void QtPlot::plot() {
  constexpr bool plotErrorBars = true;

  Widgets::MplCpp::plot(m_workspaces, boost::none, m_workspaceIndices, m_canvas->gcf().pyobj(), boost::none,
                        m_axisProperties, boost::none, plotErrorBars, false);
}

void QtPlot::createLayout() {
  auto plotLayout = new QVBoxLayout(this);
  plotLayout->setContentsMargins(0, 0, 0, 0);
  plotLayout->setSpacing(0);
  plotLayout->addWidget(m_canvas, 0, 0);
  setLayout(plotLayout);
}
} // namespace MantidQt::MantidWidgets