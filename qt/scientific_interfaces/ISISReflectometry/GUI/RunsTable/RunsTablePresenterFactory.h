// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTID_CUSTOMINTERFACES_BATCHPRESENTERFACTORY_H_
#define MANTID_CUSTOMINTERFACES_BATCHPRESENTERFACTORY_H_
#include "Common/DllConfig.h"
#include "GUI/Plotting/IPlotter.h"
#include "IRunsTablePresenter.h"
#include <memory>
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

class IRunsTableView;

class MANTIDQT_ISISREFLECTOMETRY_DLL RunsTablePresenterFactory {
public:
  RunsTablePresenterFactory(std::vector<std::string> const &instruments,
                            double thetaTolerance,
                            std::unique_ptr<IPlotter> plotter);
  virtual std::unique_ptr<IRunsTablePresenter>
  operator()(IRunsTableView *view) const;

protected:
  std::vector<std::string> m_instruments;
  double m_thetaTolerance;
  std::unique_ptr<IPlotter> m_plotter;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACES_BATCHPRESENTERFACTORY_H_
