// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_RUNSPRESENTERFACTORY_H
#define MANTID_ISISREFLECTOMETRY_RUNSPRESENTERFACTORY_H
#include "../RunsTable/RunsTablePresenterFactory.h"
#include "Common/DllConfig.h"
#include "IRunsPresenter.h"
#include "IRunsView.h"
#include "ISearcher.h"
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
      boost::shared_ptr<IAutoreduction> autoreduction,
      boost::shared_ptr<ISearcher> searcher)
      : m_runsTablePresenterFactory(std::move(runsTablePresenterFactory)),
        m_thetaTolerance(std::move(thetaTolerance)),
        m_instruments(std::move(instruments)),
        m_defaultInstrumentIndex(std::move(defaultInstrumentIndex)),
        m_messageHandler(messageHandler),
        m_autoreduction(std::move(autoreduction)),
        m_searcher(std::move(searcher)) {}

  std::unique_ptr<IRunsPresenter> make(IRunsView *view) {
    return std::make_unique<RunsPresenter>(
        view, view, &m_runsTablePresenterFactory, m_thetaTolerance,
        m_instruments, m_defaultInstrumentIndex, m_messageHandler,
        m_autoreduction, m_searcher);
  }

private:
  RunsTablePresenterFactory m_runsTablePresenterFactory;
  double m_thetaTolerance;
  std::vector<std::string> m_instruments;
  int m_defaultInstrumentIndex;
  IMessageHandler *m_messageHandler;
  boost::shared_ptr<IAutoreduction> m_autoreduction;
  boost::shared_ptr<ISearcher> m_searcher;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_ISISREFLECTOMETRY_RUNSPRESENTERFACTORY_H
