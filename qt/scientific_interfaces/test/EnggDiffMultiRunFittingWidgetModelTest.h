// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
    const RunLabel runLabel("123", 1);
    TS_ASSERT_THROWS_NOTHING(model.addFittedPeaks(runLabel, ws));

    boost::optional<Mantid::API::MatrixWorkspace_sptr> retrievedWS(boost::none);
    TS_ASSERT_THROWS_NOTHING(retrievedWS = model.getFittedPeaks(runLabel));
    TS_ASSERT(retrievedWS);
    TS_ASSERT_EQUALS(ws, *retrievedWS);
  }

  void test_getFittedPeaksSucceedsWhenWorkspaceNotInModel() {
    EnggDiffMultiRunFittingWidgetModel model;
    boost::optional<Mantid::API::MatrixWorkspace_sptr> retrievedWS(boost::none);
    TS_ASSERT_THROWS_NOTHING(retrievedWS =
                                 model.getFittedPeaks(RunLabel("123", 1)));
    TS_ASSERT(!retrievedWS);
  }

  void test_addAndGetFocusedRun() {
    EnggDiffMultiRunFittingWidgetModel model;

    Mantid::API::MatrixWorkspace_sptr ws =
        WorkspaceCreationHelper::create2DWorkspaceBinned(4, 4, 0.5);
    const RunLabel runLabel("123", 1);
    TS_ASSERT_THROWS_NOTHING(model.addFocusedRun(runLabel, ws));

    boost::optional<Mantid::API::MatrixWorkspace_sptr> retrievedWS(boost::none);
    TS_ASSERT_THROWS_NOTHING(retrievedWS = model.getFocusedRun(runLabel));
    TS_ASSERT(retrievedWS);
    TS_ASSERT_EQUALS(ws, *retrievedWS);
  }

  void test_getFocusedRunSucceedsWhenWorkspaceNotInModel() {
    EnggDiffMultiRunFittingWidgetModel model;
    boost::optional<Mantid::API::MatrixWorkspace_sptr> retrievedWS(boost::none);
    TS_ASSERT_THROWS_NOTHING(retrievedWS =
                                 model.getFocusedRun(RunLabel("123", 1)));
    TS_ASSERT(!retrievedWS);
  }

  void test_getAllWorkspaceLabels() {
    EnggDiffMultiRunFittingWidgetModel model;

    Mantid::API::MatrixWorkspace_sptr ws =
        WorkspaceCreationHelper::create2DWorkspaceBinned(4, 4, 0.5);

    const RunLabel label1("123", 1);
    model.addFocusedRun(label1, ws);
    const RunLabel label2("456", 2);
    model.addFocusedRun(label2, ws);

    const std::vector<RunLabel> expectedLabels1({label1, label2});

    std::vector<RunLabel> retrievedLabels;
    TS_ASSERT_THROWS_NOTHING(retrievedLabels = model.getAllWorkspaceLabels());
    TS_ASSERT_EQUALS(expectedLabels1, retrievedLabels);

    const RunLabel label3("456", 1);
    model.addFocusedRun(label3, ws);
    model.addFittedPeaks(RunLabel("123", 2), ws);
    const std::vector<RunLabel> expectedLabels2 = {label1, label3, label2};
    TS_ASSERT_THROWS_NOTHING(retrievedLabels = model.getAllWorkspaceLabels());
    TS_ASSERT_EQUALS(expectedLabels2, retrievedLabels);
  }

  void test_removeRun() {
    EnggDiffMultiRunFittingWidgetModel model;

    Mantid::API::MatrixWorkspace_sptr ws =
        WorkspaceCreationHelper::create2DWorkspaceBinned(4, 4, 0.5);

    const RunLabel label1("123", 1);
    model.addFocusedRun(label1, ws);

    TS_ASSERT_THROWS_NOTHING(model.removeRun(label1));
    TS_ASSERT(!model.getFocusedRun(label1));

    TS_ASSERT_THROWS_ANYTHING(model.removeRun(RunLabel("456", 2)));
  }
};

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETMODELTEST_H_
