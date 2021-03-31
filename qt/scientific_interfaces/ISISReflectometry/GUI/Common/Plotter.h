// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "IPlotter.h"

#include <QtGlobal>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
class IPythonRunner;
#endif

class MANTIDQT_ISISREFLECTOMETRY_DLL Plotter : public IPlotter {
public:
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  Plotter(IPythonRunner *pythonRunner);
  void runPython(const std::string &pythonCode) const;
#endif
  void reflectometryPlot(const std::vector<std::string> &workspaces) const override;

private:
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  // Object only needed for MantidPlot implementation as it requires Python
  // execution
  IPythonRunner *m_pythonRunner;
#endif
};

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
