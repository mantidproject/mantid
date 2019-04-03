// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_PLOTTER_H
#define MANTID_ISISREFLECTOMETRY_PLOTTER_H

#include "Common/DllConfig.h"

#include <QtGlobal>
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
class RunsTableView;
#endif

class MANTIDQT_ISISREFLECTOMETRY_DLL Plotter {
public:
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  Plotter(RunsTableView *RunsTable);
#endif
  void reflectometryPlot(const std::vector<std::string> &workspaces);

private:
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  void runPython(const std::string &pythonCode);
  // Object only needed for MantidPlot implementation as it requires Python
  // execution
  RunsTableView *m_runsTableView;
#endif
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_ISISREFLECTOMETRY_PLOTTER_H */