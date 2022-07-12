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
#include <string>

using Mantid::API::MatrixWorkspace_sptr;
using namespace MantidQt::Widgets::Common;
using namespace MantidQt::Widgets::MplCpp;

namespace MantidQt::MantidWidgets {
QtPlot::QtPlot(QWidget *parent) : QWidget(parent), m_canvas(new FigureCanvasQt(111, "", parent)) { createLayout(); }

void QtPlot::clear() {}

void QtPlot::setXScaleType(const AxisScale axisScale) {
  switch (axisScale) {
  case AxisScale::LINEAR:
    m_axisProperties[QString("xscale")] = QVariant("linear");
    break;
  case AxisScale::LOG:
    m_axisProperties[QString("xscale")] = QVariant("log");
    break;
  }
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
}

void QtPlot::addSpectrum(const MatrixWorkspace_sptr &ws, const size_t wsIndex) {
  const bool plotErrorBars = true;

  auto const workspaces = std::vector<MatrixWorkspace_sptr>{ws};
  auto const wkspIndices = std::vector<int>{static_cast<int>(wsIndex)};

  Widgets::MplCpp::plot(workspaces, boost::none, wkspIndices, m_canvas->gcf().pyobj(), boost::none, m_axisProperties,
                        boost::none, plotErrorBars, false);
}

void QtPlot::createLayout() {
  auto plotLayout = new QVBoxLayout(this);
  plotLayout->setContentsMargins(0, 0, 0, 0);
  plotLayout->setSpacing(0);
  plotLayout->addWidget(m_canvas, 0, 0);
  setLayout(plotLayout);
}
} // namespace MantidQt::MantidWidgets