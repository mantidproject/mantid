// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_RUNSPRESENTERFACTORY_H
#define MANTID_ISISREFLECTOMETRY_RUNSPRESENTERFACTORY_H
#include "Common/DllConfig.h"
#include "GUI/RunsTable/RunsTablePresenterFactory.h"
#include "IRunsPresenter.h"
#include "IRunsView.h"
#include "MantidQtWidgets/Common/ProgressableView.h"
#include "RunsPresenter.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class RunsPresenterFactory {
public:
  RunsPresenterFactory( // cppcheck-suppress passedByValue
      RunsTablePresenterFactory runsTablePresenterFactory,
      double thetaTolerance, std::vector<std::string> instruments,
      IMessageHandler *messageHandler)
      : m_runsTablePresenterFactory(std::move(runsTablePresenterFactory)),
        m_thetaTolerance(std::move(thetaTolerance)),
        m_instruments(std::move(instruments)),
        m_messageHandler(messageHandler) {}

  std::unique_ptr<IRunsPresenter> make(IRunsView *view) {
    return std::make_unique<RunsPresenter>(
        view, view, m_runsTablePresenterFactory, m_thetaTolerance,
        m_instruments, m_messageHandler);
  }

private:
  RunsTablePresenterFactory m_runsTablePresenterFactory;
  double m_thetaTolerance;
  std::vector<std::string> m_instruments;
  IMessageHandler *m_messageHandler;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_ISISREFLECTOMETRY_RUNSPRESENTERFACTORY_H
