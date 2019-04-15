// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_MOCKBATCHPRESENTERFACTORY_H_
#define MANTID_CUSTOMINTERFACES_MOCKBATCHPRESENTERFACTORY_H_
#include "../../../ISISReflectometry/GUI/Plotting/Plotter.h"
#include "../../../ISISReflectometry/GUI/RunsTable/RunsTablePresenterFactory.h"
#include "../ReflMockObjects.h"
#include "DllConfig.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MockRunsTablePresenter.h"
#include <gmock/gmock.h>

GNU_DIAG_OFF_SUGGEST_OVERRIDE

namespace MantidQt {
namespace CustomInterfaces {

class MockRunsTablePresenterFactory : public RunsTablePresenterFactory {
public:
  MockRunsTablePresenterFactory(std::vector<std::string> const &instruments,
                                double thetaTolerance, Plotter *plotter)
      : RunsTablePresenterFactory(instruments, thetaTolerance, plotter) {}
  std::unique_ptr<IRunsTablePresenter>
  operator()(IRunsTableView *view) const override {
    return Mantid::Kernel::make_unique<MockRunsTablePresenter>(
        view, m_instruments, m_thetaTolerance, ReductionJobs());
  }
};
} // namespace CustomInterfaces
} // namespace MantidQt
GNU_DIAG_ON_SUGGEST_OVERRIDE
#endif // MANTID_CUSTOMINTERFACES_MOCKRUNSTABLEPRESENTERFACTORY_H_
