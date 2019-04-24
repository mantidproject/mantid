// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_PLOTTER_H
#define MANTID_ISISREFLECTOMETRY_PLOTTER_H

#include "Common/DllConfig.h"
#include "IPlotter.h"

#include <QtGlobal>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
class IMainWindowView;
#endif

class MANTIDQT_ISISREFLECTOMETRY_DLL Plotter : public IPlotter {
public:
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  Plotter(MantidQt::CustomInterfaces::IMainWindowView *mainWindowView);
  void runPython(const std::string &pythonCode) const;
#endif
  void
  reflectometryPlot(const std::vector<std::string> &workspaces) const override;

private:
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  // Object only needed for MantidPlot implementation as it requires Python
  // execution
  MantidQt::CustomInterfaces::IMainWindowView *m_mainWindowView;
#endif
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_ISISREFLECTOMETRY_PLOTTER_H */