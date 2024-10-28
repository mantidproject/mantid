// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "Common/DllConfig.h"
#include "GUI/Common/Plotter.h"
#include "IRunsTablePresenter.h"
#include <memory>
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class IRunsTableView;

class MANTIDQT_ISISREFLECTOMETRY_DLL RunsTablePresenterFactory {
public:
  RunsTablePresenterFactory(std::vector<std::string> instruments, double thetaTolerance, Plotter plotter);
  virtual std::unique_ptr<IRunsTablePresenter> operator()(IRunsTableView *view) const;

protected:
  std::vector<std::string> m_instruments;
  double m_thetaTolerance;
  Plotter m_plotter;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
