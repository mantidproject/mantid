// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "Plotter.h"

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include "MantidQtWidgets/Common/PythonRunner.h"
#endif

namespace {}

namespace MantidQt {
namespace CustomInterfaces {

void Plotter::reflectometryPlot(const QOrderedSet<QString> &workspaces) {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  if (!workspaces.isEmpty()) {
    QString pythonSrc;
    pythonSrc += "base_graph = None\n";
    for (auto ws = workspaces.begin(); ws != workspaces.end(); ++ws)
      pythonSrc += "base_graph = plotSpectrum(\"" + ws.key() +
                   "\", 0, True, window = base_graph)\n";

    pythonSrc += "base_graph.activeLayer().logLogAxes()\n";

    this->runPython(pythonSrc);
  }
#else
  throw std::runtime_error(
      "Plotter::reflectometryPlot() not implemented for Qt >= 5");
#endif
}

void Plotter::runPython(const QString &python) {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  PythonRunner pyRunner = PythonRunner();
  pyRunner.runPythonCode(runPythonCode);
#else
  // This should never be implemented for Qt 5 or above because that is
  // workbench.
  throw std::runtime_error("Plotter::runPython() not implemented for Qt >= 5");
#endif
}

} // namespace CustomInterfaces
} // namespace MantidQt