// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "Plotter.h"

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include "RunsTableView.h"
#else
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/MplCpp/Plot.h"

#include <QHash>
#include <QString>
#include <QVariant>
using namespace MantidQt::Widgets::MplCpp;
#endif

namespace MantidQt {
namespace CustomInterfaces {

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
Plotter::Plotter(RunsTableView *runsTableView)
    : m_runsTableView(runsTableView) {}
#endif

void Plotter::reflectometryPlot(const std::vector<std::string> &workspaces) {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  // MantidPlot plotting
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
  std::vector<MatrixWorkspace_sptr> workspaceObjects;
  for (const auto &workspaceName : workspaces) {
    workspaceObjects.emplace_back(boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve(workspaceName)));
  }
  QHash<QString, QVariant> ax_properties;
  ax_properties[QString("yscale")] = QVariant("log");
  ax_properties[QString("xscale")] = QVariant("log");

  // plot(workspaces, spectrum_nums, wksp_indices, fig, plot_kwargs,
  // ax_properties, windows_title, errors, overplot)
  plot(workspaceObjects, boost::none, boost::none, boost::none, boost::none,
       ax_properties, boost::none, false, true);
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