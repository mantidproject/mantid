#ifndef MANTID_CUSTOMINTERFACES_REFLBATCHPRESENTERTEST_H_
#define MANTID_CUSTOMINTERFACES_REFLBATCHPRESENTERTEST_H_

#include "../../../ISISReflectometry/Presenters/BatchPresenter.h"
#include "../../../ISISReflectometry/Reduction/Slicing.h"
#include "MockBatchView.h"
#include "MantidQtWidgets/Common/Batch/MockJobTreeView.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces;
using testing::Return;
using testing::Mock;
using testing::NiceMock;

class BatchPresenterTest {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  void jobsViewIs(MantidQt::MantidWidgets::Batch::IJobTreeView &jobsView,
                  MockBatchView &view) {
    ON_CALL(view, jobs()).WillByDefault(::testing::ReturnRef(jobsView));
  }

  BatchPresenterTest() : m_jobs(), m_view() { jobsViewIs(m_jobs, m_view); }

  bool verifyAndClearExpectations() {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_jobs));
    return true;
  }

  void selectedRowLocationsAre(
      MantidQt::MantidWidgets::Batch::MockJobTreeView &mockJobs,
      std::vector<MantidQt::MantidWidgets::Batch::RowLocation> const &
          locations) {
    ON_CALL(mockJobs, selectedRowLocations()).WillByDefault(Return(locations));
  }

  UnslicedReductionJobs const &
  unslicedJobsFromPresenter(BatchPresenter &presenter) {
    return boost::get<UnslicedReductionJobs>(presenter.reductionJobs());
  }

  template <typename... Args>
  MantidQt::MantidWidgets::Batch::RowLocation location(Args... args) {
    return MantidQt::MantidWidgets::Batch::RowLocation(
        MantidQt::MantidWidgets::Batch::RowPath({args...}));
  }

  BatchPresenter makePresenter(IBatchView &view) {
    static auto slicing = Slicing();
    return BatchPresenter(&view, {}, 0.01, WorkspaceNamesFactory(slicing),
                          UnslicedReductionJobs());
  }

  BatchPresenter makePresenter(IBatchView &view, Jobs jobs) {
    static auto slicing = Slicing();
    return BatchPresenter(&view, {}, 0.01, WorkspaceNamesFactory(slicing),
                          std::move(jobs));
  }

protected:
  NiceMock<MantidQt::MantidWidgets::Batch::MockJobTreeView> m_jobs;
  NiceMock<MockBatchView> m_view;
};

#endif // MANTID_CUSTOMINTERFACES_REFLBATCHPRESENTERTEST_H_
