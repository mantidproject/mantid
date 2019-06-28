// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "Plotter.h"

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include "../Common/IPythonRunner.h"
#else
#include "MantidQtWidgets/MplCpp/Plot.h"

#include <QHash>
#include <QString>
#include <QVariant>
using namespace MantidQt::Widgets::MplCpp;
#endif

namespace MantidQt {
namespace CustomInterfaces {

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
Plotter::Plotter(MantidQt::CustomInterfaces::IPythonRunner *pythonRunner)
    : m_pythonRunner(pythonRunner) {}
#endif

void Plotter::reflectometryPlot(
    const std::vector<std::string> &workspaces) const {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  if (!workspaces.empty()) {
    std::string pythonSrc;
    pythonSrc += "base_graph = None\n";
    for (const auto &workspace : workspaces)
      pythonSrc += "base_graph = plotSpectrum(\"" + workspace +
                   "\", 0, True, window = base_graph)\n";

    pythonSrc += "base_graph.activeLayer().logLogAxes()\n";

    this->runPython(pythonSrc);
  }
#else
  // Workbench Plotting
  QHash<QString, QVariant> ax_properties;
  ax_properties[QString("yscale")] = QVariant("log");
  ax_properties[QString("xscale")] = QVariant("log");

  // plot(workspaces, spectrum_nums, wksp_indices, fig, plot_kwargs,
  // ax_properties, windows_title, errors, overplot)
  std::string window_title = "ISIS Reflectometry Plot";
  if (!workspaces.empty()) {
    window_title = workspaces[0];
  }

  const bool plotErrorBars = true;
  std::vector<int> wksp_indices = {0};
  plot(workspaces, boost::none, wksp_indices, boost::none, boost::none,
       ax_properties, window_title, plotErrorBars, false);
#endif
}

// This should never be implemented for Qt 5 or above because that is
// workbench.
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
void Plotter::runPython(const std::string &pythonCode) const {
  m_pythonRunner->runPythonAlgorithm(pythonCode);
}
#endif

} // namespace CustomInterfaces
} // namespace MantidQt
