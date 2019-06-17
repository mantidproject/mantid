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

class RunsPresenterFactory {
public:
  RunsPresenterFactory( // cppcheck-suppress passedByValue
      RunsTablePresenterFactory runsTablePresenterFactory,
      double thetaTolerance, std::vector<std::string> instruments,
      int defaultInstrumentIndex, IMessageHandler *messageHandler,
      IPythonRunner *pythonRunner)
      : m_runsTablePresenterFactory(std::move(runsTablePresenterFactory)),
        m_thetaTolerance(std::move(thetaTolerance)),
        m_instruments(std::move(instruments)),
        m_defaultInstrumentIndex(std::move(defaultInstrumentIndex)),
        m_messageHandler(messageHandler), m_pythonRunner(pythonRunner) {}

  std::unique_ptr<IRunsPresenter> make(IRunsView *view) {
    return std::make_unique<RunsPresenter>(
        view, view, m_runsTablePresenterFactory, m_thetaTolerance,
        m_instruments, m_defaultInstrumentIndex, m_messageHandler,
        m_pythonRunner);
  }

private:
  RunsTablePresenterFactory m_runsTablePresenterFactory;
  double m_thetaTolerance;
  std::vector<std::string> m_instruments;
  int m_defaultInstrumentIndex;
  IMessageHandler *m_messageHandler;
  IPythonRunner *m_pythonRunner;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_ISISREFLECTOMETRY_RUNSPRESENTERFACTORY_H
