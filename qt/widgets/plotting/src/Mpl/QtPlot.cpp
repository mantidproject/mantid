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
QtPlot::QtPlot(QWidget *parent) : QWidget(parent), m_canvas(new FigureCanvasQt(111, "mantid", parent)) {
  createLayout();
}

void QtPlot::clear() {}

void QtPlot::addSpectrum(const std::string &lineLabel, const MatrixWorkspace_sptr &ws, const size_t wsIndex) {
  QHash<QString, QVariant> ax_properties;
  ax_properties[QString("yscale")] = QVariant("log");
  ax_properties[QString("xscale")] = QVariant("log");
  const bool plotErrorBars = true;

  auto const workspaces = std::vector<MatrixWorkspace_sptr>{ws};
  auto const wkspIndices = std::vector<int>{static_cast<int>(wsIndex)};

  Widgets::MplCpp::plot(workspaces, boost::none, wkspIndices, m_canvas->gcf().pyobj(), boost::none, ax_properties,
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