// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOM_INTERFACES_ENGGDIFFGSASFITTINGPRESENTERTEST_H_
#define MANTID_CUSTOM_INTERFACES_ENGGDIFFGSASFITTINGPRESENTERTEST_H_

#include "../EnggDiffraction/EnggDiffGSASFittingPresenter.h"
#include "EnggDiffGSASFittingModelMock.h"
#include "EnggDiffGSASFittingViewMock.h"
#include "EnggDiffMultiRunFittingWidgetPresenterMock.h"
#include "EnggDiffractionParamMock.h"

#include "MantidAPI/WorkspaceFactory.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces;
using testing::Return;
using testing::Throw;

class EnggDiffGSASFittingPresenterTest : public CxxTest::TestSuite {

public:
  void test_loadValidFile() {
    auto presenter = setUpPresenter();
    const auto filename = "Valid filename";

    EXPECT_CALL(*m_mockViewPtr, getFocusedFileNames())
        .Times(1)
        .WillOnce(Return(std::vector<std::string>({filename})));

    const Mantid::API::MatrixWorkspace_sptr ws(
        WorkspaceCreationHelper::create2DWorkspaceBinned(1, 100));

    EXPECT_CALL(*m_mockModelPtr, loadFocusedRun(filename))
        .Times(1)
        .WillOnce(Return(ws));
    EXPECT_CALL(*m_mockMultiRunWidgetPtr, addFocusedRun(ws));

    EXPECT_CALL(*m_mockViewPtr, userWarning(testing::_, testing::_)).Times(0);

    presenter->notify(IEnggDiffGSASFittingPresenter::LoadRun);
    assertMocksUsedCorrectly();
  }

  void test_loadInvalidFile() {
    auto presenter = setUpPresenter();
    const auto filename = "Invalid filename";

    EXPECT_CALL(*m_mockViewPtr, getFocusedFileNames())
        .Times(1)
        .WillOnce(Return(std::vector<std::string>({filename})));

    EXPECT_CALL(*m_mockModelPtr, loadFocusedRun(filename))
        .Times(1)
        .WillOnce(Throw(std::runtime_error("Failure reason")));

    EXPECT_CALL(*m_mockViewPtr,
                userWarning("Could not load file", "Failure reason"))
        .Times(1);

    presenter->notify(IEnggDiffGSASFittingPresenter::LoadRun);
    assertMocksUsedCorrectly();
  }

  void test_doRietveldRefinement() {
    auto presenter = setUpPresenter();

    const GSASIIRefineFitPeaksParameters params(
        WorkspaceCreationHelper::create2DWorkspaceBinned(1, 100),
        RunLabel("123", 1), GSASRefinementMethod::RIETVELD, "Instrument file",
        {"Phase1", "Phase2"}, "GSASHOME", "GPX.gpx", boost::none, boost::none,
        10000, 40000, true, false);
    setRefinementParamsExpectations(params);

    EXPECT_CALL(*m_mockViewPtr, setEnabled(false)).Times(1);
    EXPECT_CALL(
        *m_mockModelPtr,
        doRefinements(std::vector<GSASIIRefineFitPeaksParameters>({params})))
        .Times(1);

    presenter->notify(IEnggDiffGSASFittingPresenter::DoRefinement);
    assertMocksUsedCorrectly();
  }

  void test_doPawleyRefinement() {
    auto presenter = setUpPresenter();

    const GSASIIRefineFitPeaksParameters params(
        WorkspaceCreationHelper::create2DWorkspaceBinned(1, 100),
        RunLabel("123", 1), GSASRefinementMethod::PAWLEY, "Instrument file",
        {"Phase1", "Phase2"}, "GSASHOME", "GPX.gpx", 1, 2, 10000, 40000, true,
        false);
    setRefinementParamsExpectations(params);

    EXPECT_CALL(*m_mockViewPtr, setEnabled(false)).Times(1);
    EXPECT_CALL(
        *m_mockModelPtr,
        doRefinements(std::vector<GSASIIRefineFitPeaksParameters>({params})))
        .Times(1);

    presenter->notify(IEnggDiffGSASFittingPresenter::DoRefinement);
    assertMocksUsedCorrectly();
  }

  void test_selectValidRunFitResultsAvailable() {
    auto presenter = setUpPresenter();
    const RunLabel runLabel("123", 1);

    EXPECT_CALL(*m_mockMultiRunWidgetPtr, getSelectedRunLabel())
        .Times(1)
        .WillOnce(Return(runLabel));
    EXPECT_CALL(*m_mockModelPtr, hasFitResultsForRun(runLabel))
        .Times(1)
        .WillOnce(Return(true));

    const double rwp = 50;
    EXPECT_CALL(*m_mockModelPtr, getRwp(runLabel))
        .Times(1)
        .WillOnce(Return(rwp));

    const double sigma = 30;
    EXPECT_CALL(*m_mockModelPtr, getSigma(runLabel))
        .Times(1)
        .WillOnce(Return(sigma));

    const double gamma = 40;
    EXPECT_CALL(*m_mockModelPtr, getGamma(runLabel))
        .Times(1)
        .WillOnce(Return(gamma));

    const auto latticeParams =
        Mantid::API::WorkspaceFactory::Instance().createTable();
    EXPECT_CALL(*m_mockModelPtr, getLatticeParams(runLabel))
        .Times(1)
        .WillOnce(Return(latticeParams));

    EXPECT_CALL(*m_mockViewPtr, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(*m_mockViewPtr, displayRwp(rwp)).Times(1);
    EXPECT_CALL(*m_mockViewPtr, displaySigma(sigma)).Times(1);
    EXPECT_CALL(*m_mockViewPtr, displayGamma(gamma)).Times(1);
    EXPECT_CALL(*m_mockViewPtr, displayLatticeParams(latticeParams)).Times(1);

    presenter->notify(IEnggDiffGSASFittingPresenter::SelectRun);
    assertMocksUsedCorrectly();
  }

  void test_selectRunNoFitResults() {
    auto presenter = setUpPresenter();
    const RunLabel runLabel("123", 1);

    EXPECT_CALL(*m_mockMultiRunWidgetPtr, getSelectedRunLabel())
        .Times(1)
        .WillOnce(Return(runLabel));
    EXPECT_CALL(*m_mockModelPtr, hasFitResultsForRun(runLabel))
        .Times(1)
        .WillOnce(Return(false));

    EXPECT_CALL(*m_mockModelPtr, getRwp(testing::_)).Times(0);
    EXPECT_CALL(*m_mockModelPtr, getLatticeParams(testing::_)).Times(0);
    EXPECT_CALL(*m_mockModelPtr, getSigma(testing::_)).Times(0);
    EXPECT_CALL(*m_mockModelPtr, getGamma(testing::_)).Times(0);

    presenter->notify(IEnggDiffGSASFittingPresenter::SelectRun);
    assertMocksUsedCorrectly();
  }

  void test_selectRunNoLabelSelected() {
    auto presenter = setUpPresenter();
    EXPECT_CALL(*m_mockMultiRunWidgetPtr, getSelectedRunLabel())
        .Times(1)
        .WillOnce(Return(boost::none));
    presenter->notify(IEnggDiffGSASFittingPresenter::SelectRun);
    assertMocksUsedCorrectly();
  }

  void test_notifyRefinementFailed() {
    auto presenter = setUpPresenter();
    EXPECT_CALL(*m_mockViewPtr,
                userWarning("Refinement failed", "Failure Reason"));
    EXPECT_CALL(*m_mockViewPtr, setEnabled(true));
    EXPECT_CALL(*m_mockViewPtr, showStatus("Refinement failed"));

    presenter->notifyRefinementFailed("Failure Reason");
    assertMocksUsedCorrectly();
  }

  void test_notifyRefinementsComplete() {
    auto presenter = setUpPresenter();
    EXPECT_CALL(*m_mockViewPtr, setEnabled(true));
    EXPECT_CALL(*m_mockViewPtr, showStatus("Ready"));

    const Mantid::API::MatrixWorkspace_sptr fittedPeaks(
        WorkspaceCreationHelper::create2DWorkspaceBinned(1, 100));
    const auto latticeParams =
        Mantid::API::WorkspaceFactory::Instance().createTable();
    const RunLabel runLabel1("123", 1);
    const GSASIIRefineFitPeaksOutputProperties refinementResults1(
        1, 2, 3, fittedPeaks, latticeParams, runLabel1);
    const RunLabel runLabel2("125", 1);
    const std::vector<RunLabel> runLabels({runLabel1, runLabel2});
    const GSASIIRefineFitPeaksOutputProperties refinementResults2(
        1, 2, 3, fittedPeaks, latticeParams, runLabel2);
    const std::vector<GSASIIRefineFitPeaksOutputProperties> refinementResultSet(
        {refinementResults1, refinementResults2});
    const Mantid::API::IAlgorithm_sptr alg(nullptr);

    const std::string outputFilename("/some/dir/Runs/123_125.hdf5");
    EXPECT_CALL(*m_mockParamPtr, userHDFMultiRunFilename(runLabels))
        .Times(1)
        .WillOnce(Return(outputFilename));

    EXPECT_CALL(*m_mockModelPtr, saveRefinementResultsToHDF5(
                                     alg, refinementResultSet, outputFilename));
    presenter->notifyRefinementsComplete(alg, refinementResultSet);

    assertMocksUsedCorrectly();
  }

  void test_notifyRefinementSuccessful() {
    auto presenter = setUpPresenter();
    const Mantid::API::MatrixWorkspace_sptr fittedPeaks(
        WorkspaceCreationHelper::create2DWorkspaceBinned(1, 100));
    const auto latticeParams =
        Mantid::API::WorkspaceFactory::Instance().createTable();
    const RunLabel runLabel("123", 1);
    const GSASIIRefineFitPeaksOutputProperties refinementResults(
        1, 2, 3, fittedPeaks, latticeParams, runLabel);
    const Mantid::API::IAlgorithm_sptr alg(nullptr);

    const std::string hdfFilename = "directory/path/run.hdf5";
    ON_CALL(*m_mockParamPtr, userHDFRunFilename(testing::_))
        .WillByDefault(Return(hdfFilename));

    EXPECT_CALL(*m_mockMultiRunWidgetPtr,
                addFittedPeaks(runLabel, fittedPeaks));
    EXPECT_CALL(*m_mockViewPtr, showStatus("Saving refinement results"));

    EXPECT_CALL(*m_mockModelPtr,
                saveRefinementResultsToHDF5(
                    alg,
                    std::vector<GSASIIRefineFitPeaksOutputProperties>(
                        {refinementResults}),
                    hdfFilename));
    EXPECT_CALL(*m_mockViewPtr, setEnabled(true));
    EXPECT_CALL(*m_mockViewPtr, showStatus("Ready"));

    // make sure displayFitResults(runLabel) is getting called
    EXPECT_CALL(*m_mockModelPtr, getLatticeParams(runLabel))
        .Times(1)
        .WillOnce(Return(boost::none));
    ON_CALL(*m_mockModelPtr, getRwp(runLabel)).WillByDefault(Return(1));
    ON_CALL(*m_mockModelPtr, getSigma(runLabel)).WillByDefault(Return(1));
    ON_CALL(*m_mockModelPtr, getGamma(runLabel)).WillByDefault(Return(1));

    presenter->notifyRefinementSuccessful(alg, refinementResults);
    assertMocksUsedCorrectly();
  }

  void test_notifyRefinementCancelled() {
    auto presenter = setUpPresenter();
    EXPECT_CALL(*m_mockViewPtr, setEnabled(true)).Times(1);
    EXPECT_CALL(*m_mockViewPtr, showStatus("Ready")).Times(1);

    presenter->notifyRefinementCancelled();

    assertMocksUsedCorrectly();
  }

  void test_refineAllPassesParamsCorrectlyFromViewToModel() {
    auto presenter = setUpPresenter();

    const GSASIIRefineFitPeaksParameters params1(
        WorkspaceCreationHelper::create2DWorkspaceBinned(1, 100),
        RunLabel("123", 1), GSASRefinementMethod::RIETVELD, "Instrument file",
        {"Phase1", "Phase2"}, "GSASHOME", "GPX_123_1.gpx", boost::none,
        boost::none, 10000, 40000, true, false);
    const GSASIIRefineFitPeaksParameters params2(
        WorkspaceCreationHelper::create2DWorkspaceBinned(2, 200),
        RunLabel("456", 2), GSASRefinementMethod::RIETVELD, "Instrument file",
        {"Phase1", "Phase2"}, "GSASHOME", "GPX_456_2.gpx", boost::none,
        boost::none, 10000, 40000, true, false);

    const std::vector<RunLabel> runLabels({params1.runLabel, params2.runLabel});
    EXPECT_CALL(*m_mockMultiRunWidgetPtr, getAllRunLabels())
        .WillOnce(Return(runLabels));

    EXPECT_CALL(*m_mockMultiRunWidgetPtr, getFocusedRun(testing::_))
        .Times(2)
        .WillOnce(Return(params1.inputWorkspace))
        .WillOnce(Return(params2.inputWorkspace));
    EXPECT_CALL(*m_mockViewPtr, getRefinementMethod())
        .Times(1)
        .WillOnce(Return(params1.refinementMethod));
    EXPECT_CALL(*m_mockViewPtr, getInstrumentFileName())
        .Times(1)
        .WillOnce(Return(params1.instParamsFile));
    EXPECT_CALL(*m_mockViewPtr, getPhaseFileNames())
        .Times(1)
        .WillOnce(Return(params1.phaseFiles));
    EXPECT_CALL(*m_mockViewPtr, getPathToGSASII())
        .Times(1)
        .WillOnce(Return(params1.gsasHome));
    EXPECT_CALL(*m_mockViewPtr, getGSASIIProjectPath())
        .Times(1)
        .WillOnce(Return("GPX.gpx"));
    EXPECT_CALL(*m_mockViewPtr, getPawleyDMin())
        .Times(1)
        .WillOnce(Return(params1.dMin));
    EXPECT_CALL(*m_mockViewPtr, getPawleyNegativeWeight())
        .Times(1)
        .WillOnce(Return(params1.negativeWeight));
    EXPECT_CALL(*m_mockViewPtr, getXMin())
        .Times(1)
        .WillOnce(Return(params1.xMin));
    EXPECT_CALL(*m_mockViewPtr, getXMax())
        .Times(1)
        .WillOnce(Return(params1.xMax));
    EXPECT_CALL(*m_mockViewPtr, getRefineSigma())
        .Times(1)
        .WillOnce(Return(params1.refineSigma));
    EXPECT_CALL(*m_mockViewPtr, getRefineGamma())
        .Times(1)
        .WillOnce(Return(params1.refineGamma));

    EXPECT_CALL(*m_mockViewPtr, showStatus("Refining run"));
    EXPECT_CALL(*m_mockViewPtr, setEnabled(false));
    EXPECT_CALL(*m_mockModelPtr,
                doRefinements(std::vector<GSASIIRefineFitPeaksParameters>(
                    {params1, params2})));

    presenter->notify(IEnggDiffGSASFittingPresenter::RefineAll);
    assertMocksUsedCorrectly();
  }

  void test_refineAllWarnsIfNoRunsLoaded() {
    auto presenter = setUpPresenter();
    EXPECT_CALL(*m_mockMultiRunWidgetPtr, getAllRunLabels())
        .WillOnce(Return(std::vector<RunLabel>()));
    EXPECT_CALL(*m_mockViewPtr,
                userWarning("No runs loaded",
                            "Please load at least one run before refining"));
    presenter->notify(IEnggDiffGSASFittingPresenter::RefineAll);
    assertMocksUsedCorrectly();
  }

private:
  MockEnggDiffGSASFittingModel *m_mockModelPtr;
  MockEnggDiffGSASFittingView *m_mockViewPtr;
  MockEnggDiffMultiRunFittingWidgetPresenter *m_mockMultiRunWidgetPtr;
  MockEnggDiffractionParam *m_mockParamPtr;

  std::unique_ptr<EnggDiffGSASFittingPresenter> setUpPresenter() {
    auto mockModel =
        std::make_unique<testing::NiceMock<MockEnggDiffGSASFittingModel>>();
    m_mockModelPtr = mockModel.get();

    m_mockViewPtr = new testing::NiceMock<MockEnggDiffGSASFittingView>();

    auto mockMultiRunWidgetPresenter_sptr = boost::make_shared<
        testing::NiceMock<MockEnggDiffMultiRunFittingWidgetPresenter>>();
    m_mockMultiRunWidgetPtr = mockMultiRunWidgetPresenter_sptr.get();

    auto mockParam_sptr =
        boost::make_shared<testing::NiceMock<MockEnggDiffractionParam>>();
    m_mockParamPtr = mockParam_sptr.get();

    auto pres_uptr = std::make_unique<EnggDiffGSASFittingPresenter>(
        std::move(mockModel), m_mockViewPtr, mockMultiRunWidgetPresenter_sptr,
        mockParam_sptr);
    return pres_uptr;
  }

  void assertMocksUsedCorrectly() {
    TSM_ASSERT("View mock not used as expected: some EXPECT_CALL conditions "
               "not satisfied",
               testing::Mock::VerifyAndClearExpectations(m_mockModelPtr));
    TSM_ASSERT("Model mock not used as expected: some EXPECT_CALL conditions "
               "not satisfied",
               testing::Mock::VerifyAndClearExpectations(m_mockViewPtr));
    if (m_mockViewPtr) {
      delete m_mockViewPtr;
    }
  }

  void setRefinementParamsExpectations(
      const GSASIIRefineFitPeaksParameters &params) {
    EXPECT_CALL(*m_mockMultiRunWidgetPtr, getSelectedRunLabel())
        .Times(1)
        .WillOnce(Return(params.runLabel));

    EXPECT_CALL(*m_mockMultiRunWidgetPtr, getFocusedRun(params.runLabel))
        .Times(1)
        .WillOnce(Return(params.inputWorkspace));
    EXPECT_CALL(*m_mockViewPtr, getRefinementMethod())
        .Times(1)
        .WillOnce(Return(params.refinementMethod));
    EXPECT_CALL(*m_mockViewPtr, getInstrumentFileName())
        .Times(1)
        .WillOnce(Return(params.instParamsFile));
    EXPECT_CALL(*m_mockViewPtr, getPhaseFileNames())
        .Times(1)
        .WillOnce(Return(params.phaseFiles));
    EXPECT_CALL(*m_mockViewPtr, getPathToGSASII())
        .Times(1)
        .WillOnce(Return(params.gsasHome));
    EXPECT_CALL(*m_mockViewPtr, getGSASIIProjectPath())
        .Times(1)
        .WillOnce(Return(params.gsasProjectFile));
    EXPECT_CALL(*m_mockViewPtr, getPawleyDMin())
        .Times(1)
        .WillOnce(Return(params.dMin));
    EXPECT_CALL(*m_mockViewPtr, getPawleyNegativeWeight())
        .Times(1)
        .WillOnce(Return(params.negativeWeight));
    EXPECT_CALL(*m_mockViewPtr, getXMin())
        .Times(1)
        .WillOnce(Return(params.xMin));
    EXPECT_CALL(*m_mockViewPtr, getXMax())
        .Times(1)
        .WillOnce(Return(params.xMax));
    EXPECT_CALL(*m_mockViewPtr, getRefineSigma())
        .Times(1)
        .WillOnce(Return(params.refineSigma));
    EXPECT_CALL(*m_mockViewPtr, getRefineGamma())
        .Times(1)
        .WillOnce(Return(params.refineGamma));
  }
};

#endif // MANTID_CUSTOM_INTERFACES_ENGGDIFFGSASFITTINGPRESENTERTEST_H_
