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

  void test_getAllWorkspaceLabels() {
    EnggDiffMultiRunFittingWidgetModel model;

    Mantid::API::MatrixWorkspace_sptr ws =
        WorkspaceCreationHelper::create2DWorkspaceBinned(4, 4, 0.5);
    model.addFocusedRun(123, 1, ws);
    model.addFocusedRun(456, 2, ws);

    std::vector<std::pair<int, size_t>> expectedLabels(
        {std::make_pair(123, 1), std::make_pair(456, 2)});

    std::vector<std::pair<int, size_t>> retrievedLabels;
    TS_ASSERT_THROWS_NOTHING(retrievedLabels = model.getAllWorkspaceLabels());
    TS_ASSERT_EQUALS(expectedLabels, retrievedLabels);

    model.addFocusedRun(456, 1, ws);
    model.addFittedPeaks(123, 2, ws);
    expectedLabels = {std::make_pair(123, 1), std::make_pair(456, 1),
                      std::make_pair(456, 2)};
    TS_ASSERT_THROWS_NOTHING(retrievedLabels = model.getAllWorkspaceLabels());
    TS_ASSERT_EQUALS(expectedLabels, retrievedLabels);
  }
};

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETMODELTEST_H_
