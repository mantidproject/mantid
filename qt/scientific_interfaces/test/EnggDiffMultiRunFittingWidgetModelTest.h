#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETMODELTEST_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETMODELTEST_H_

#include "../EnggDiffraction/EnggDiffMultiRunFittingWidgetModel.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces;

class EnggDiffMultiRunFittingWidgetModelTest : public CxxTest::TestSuite {
public:
  void test_addAndGetFittedPeaks() {
    EnggDiffMultiRunFittingWidgetModel model;

    Mantid::API::MatrixWorkspace_sptr ws =
        WorkspaceCreationHelper::create2DWorkspaceBinned(4, 4, 0.5);
    TS_ASSERT_THROWS_NOTHING(model.addFittedPeaks(123, 1, ws));

    boost::optional<Mantid::API::MatrixWorkspace_sptr> retrievedWS(boost::none);
    TS_ASSERT_THROWS_NOTHING(retrievedWS = model.getFittedPeaks(123, 1));
    TS_ASSERT(retrievedWS);
    TS_ASSERT_EQUALS(ws, *retrievedWS);

    TS_ASSERT_THROWS_NOTHING(retrievedWS = model.getFittedPeaks(456, 2));
    TS_ASSERT(!retrievedWS);
  }

  void test_addAndGetFocusedRun() {
    EnggDiffMultiRunFittingWidgetModel model;

    Mantid::API::MatrixWorkspace_sptr ws =
        WorkspaceCreationHelper::create2DWorkspaceBinned(4, 4, 0.5);

    TS_ASSERT_THROWS_NOTHING(model.addFocusedRun(123, 1, ws));
    boost::optional<Mantid::API::MatrixWorkspace_sptr> retrievedWS(boost::none);
    TS_ASSERT_THROWS_NOTHING(retrievedWS = model.getFocusedRun(123, 1));
    TS_ASSERT(retrievedWS);
    TS_ASSERT_EQUALS(ws, *retrievedWS);

    TS_ASSERT_THROWS_NOTHING(retrievedWS = model.getFocusedRun(456, 2));
    TS_ASSERT(!retrievedWS);
  }

  void test_getWorkspaceLabels() {
    EnggDiffMultiRunFittingWidgetModel model;

    Mantid::API::MatrixWorkspace_sptr ws =
        WorkspaceCreationHelper::create2DWorkspaceBinned(4, 4, 0.5);

    model.addFocusedRun(123, 1, ws);
    model.addFocusedRun(456, 2, ws);
    model.addFocusedRun(123, 2, ws);

    std::vector<std::pair<int, size_t>> workspaceLabels;

    TS_ASSERT_THROWS_NOTHING(workspaceLabels = model.getAllWorkspaceLabels());

    TS_ASSERT_EQUALS(workspaceLabels.size(), 3);
    TS_ASSERT_EQUALS(workspaceLabels[0].first, 123);
    TS_ASSERT_EQUALS(workspaceLabels[0].second, 1);
    TS_ASSERT_EQUALS(workspaceLabels[1].first, 123);
    TS_ASSERT_EQUALS(workspaceLabels[1].second, 2);
    TS_ASSERT_EQUALS(workspaceLabels[2].first, 456);
    TS_ASSERT_EQUALS(workspaceLabels[2].second, 2);
  }
};

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETMODELTEST_H_
