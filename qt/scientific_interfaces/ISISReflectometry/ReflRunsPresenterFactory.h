#ifndef MANTID_ISISREFLECTOMETRY_REFLRUNSPRESENTERFACTORY_H
#define MANTID_ISISREFLECTOMETRY_REFLRUNSPRESENTERFACTORY_H
#include "DllConfig.h"
#include "IReflEventView.h"
#include "IReflEventTabPresenter.h"
#include "IReflRunsTabPresenter.h"
#include "IReflRunsTabView.h"
#include "ReflRunsTabPresenter.h"
#include "MantidQtWidgets/Common/ProgressableView.h"
#include "Presenters/BatchPresenterFactory.h"
#include "../General/IReflSearcher.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

class RunsPresenterFactory {
public:
  RunsPresenterFactory(BatchPresenterFactory batchPresenterFactory,
                       WorkspaceNamesFactory workspaceNamesFactory,
                       double thetaTolerance,
                       std::vector<std::string> instruments,
                       int defaultInstrumentIndex,
                       boost::shared_ptr<IReflSearcher> searcher)
      : m_batchPresenterFactory(std::move(batchPresenterFactory)),
        m_workspaceNamesFactory(std::move(workspaceNamesFactory)),
        m_thetaTolerance(std::move(thetaTolerance)),
        m_instruments(std::move(instruments)),
        m_defaultInstrumentIndex(std::move(defaultInstrumentIndex)),
        m_searcher(std::move(searcher)) {}

  std::unique_ptr<IReflRunsTabPresenter> make(IReflRunsTabView *view) {
    return std::make_unique<ReflRunsTabPresenter>(
        view, view, m_batchPresenterFactory, m_workspaceNamesFactory,
        m_thetaTolerance, m_instruments, m_defaultInstrumentIndex, m_searcher);
  }

private:
  BatchPresenterFactory m_batchPresenterFactory;
  WorkspaceNamesFactory m_workspaceNamesFactory;
  double m_thetaTolerance;
  std::vector<std::string> m_instruments;
  int m_defaultInstrumentIndex;
  boost::shared_ptr<IReflSearcher> m_searcher;
};
}
}
#endif // MANTID_ISISREFLECTOMETRY_REFLRUNSPRESENTERFACTORY_H
