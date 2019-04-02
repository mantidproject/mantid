// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "Plotter.h"

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include "RunsTableView.h"
#endif

namespace MantidQt {
namespace CustomInterfaces {

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
Plotter::Plotter(RunsTableView *runsTableView)
    : m_runsTableView(runsTableView) {}
#endif

void Plotter::reflectometryPlot(const std::vector<std::string> &workspaces) {
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
  throw std::runtime_error(
      "Plotter::reflectometryPlot() not implemented for Qt >= 5");
#endif
}

// This should never be implemented for Qt 5 or above because that is
// workbench.
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
void Plotter::runPython(const std::string &pythonCode) {
  m_runsTableView->executePythonCode(pythonCode);
}
#endif

} // namespace CustomInterfaces
} // namespace MantidQt