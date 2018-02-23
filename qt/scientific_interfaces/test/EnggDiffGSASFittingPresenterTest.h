#ifndef MANTID_CUSTOM_INTERFACES_ENGGDIFFGSASFITTINGPRESENTERTEST_H_
#define MANTID_CUSTOM_INTERFACES_ENGGDIFFGSASFITTINGPRESENTERTEST_H_

#include "../EnggDiffraction/EnggDiffGSASFittingPresenter.h"
#include "EnggDiffGSASFittingModelMock.h"
#include "EnggDiffGSASFittingViewMock.h"
#include "EnggDiffMultiRunFittingWidgetPresenterMock.h"

#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/make_unique.h"
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
                userWarning("Could not load file", "Failure reason")).Times(1);

    presenter->notify(IEnggDiffGSASFittingPresenter::LoadRun);
    assertMocksUsedCorrectly();
  }

  void test_doRietveldRefinement() {
    auto presenter = setUpPresenter();

    const RunLabel runLabel(123, 1);
    const auto refinementMethod = GSASRefinementMethod::RIETVELD;
    const auto instParams = "Instrument file";
    const std::vector<std::string> phaseFiles({"phase1", "phase2"});
    const auto pathToGSASII = "GSASHOME";
    const auto GSASIIProjectFile = "GPX.gpx";

    EXPECT_CALL(*m_mockMultiRunWidgetPtr, getSelectedRunLabel())
        .Times(1)
        .WillOnce(Return(runLabel));

    const boost::optional<Mantid::API::MatrixWorkspace_sptr> inputWorkspace(
        WorkspaceCreationHelper::create2DWorkspaceBinned(1, 100));
    EXPECT_CALL(*m_mockMultiRunWidgetPtr, getFocusedRun(runLabel))
        .Times(1)
        .WillOnce(Return(inputWorkspace));

    EXPECT_CALL(*m_mockViewPtr, getRefinementMethod())
        .Times(1)
        .WillOnce(Return(refinementMethod));
    EXPECT_CALL(*m_mockViewPtr, getInstrumentFileName())
        .Times(1)
        .WillOnce(Return(instParams));
    EXPECT_CALL(*m_mockViewPtr, getPhaseFileNames())
        .Times(1)
        .WillOnce(Return(phaseFiles));
    EXPECT_CALL(*m_mockViewPtr, getPathToGSASII())
        .Times(1)
        .WillOnce(Return(pathToGSASII));
    EXPECT_CALL(*m_mockViewPtr, getGSASIIProjectPath())
        .Times(1)
        .WillOnce(Return(GSASIIProjectFile));

    EXPECT_CALL(*m_mockModelPtr,
                doRietveldRefinement(*inputWorkspace, runLabel, instParams,
                                     phaseFiles, pathToGSASII,
                                     GSASIIProjectFile))
        .Times(1)
        .WillOnce(Throw(std::runtime_error("Failure reason")));
    EXPECT_CALL(*m_mockViewPtr,
                userError("Refinement failed", "Failure reason"));

    presenter->notify(IEnggDiffGSASFittingPresenter::DoRefinement);
    assertMocksUsedCorrectly();
  }

  void test_doPawleyRefinement() {
    auto presenter = setUpPresenter();

    const RunLabel runLabel(123, 1);
    const auto refinementMethod = GSASRefinementMethod::PAWLEY;
    const auto instParams = "Instrument file";
    const std::vector<std::string> phaseFiles({"phase1", "phase2"});
    const auto pathToGSASII = "GSASHOME";
    const auto GSASIIProjectFile = "GPX.gpx";
    const auto dmin = 1.0;
    const auto negativeWeight = 2.0;

    EXPECT_CALL(*m_mockMultiRunWidgetPtr, getSelectedRunLabel())
        .Times(1)
        .WillOnce(Return(runLabel));

    const boost::optional<Mantid::API::MatrixWorkspace_sptr> inputWorkspace(
        WorkspaceCreationHelper::create2DWorkspaceBinned(1, 100));
    EXPECT_CALL(*m_mockMultiRunWidgetPtr, getFocusedRun(runLabel))
        .Times(1)
        .WillOnce(Return(inputWorkspace));

    EXPECT_CALL(*m_mockViewPtr, getRefinementMethod())
        .Times(1)
        .WillOnce(Return(refinementMethod));
    EXPECT_CALL(*m_mockViewPtr, getInstrumentFileName())
        .Times(1)
        .WillOnce(Return(instParams));
    EXPECT_CALL(*m_mockViewPtr, getPhaseFileNames())
        .Times(1)
        .WillOnce(Return(phaseFiles));
    EXPECT_CALL(*m_mockViewPtr, getPathToGSASII())
        .Times(1)
        .WillOnce(Return(pathToGSASII));
    EXPECT_CALL(*m_mockViewPtr, getGSASIIProjectPath())
        .Times(1)
        .WillOnce(Return(GSASIIProjectFile));
    EXPECT_CALL(*m_mockViewPtr, getPawleyDMin())
        .Times(1)
        .WillOnce(Return(dmin));
    EXPECT_CALL(*m_mockViewPtr, getPawleyNegativeWeight())
        .Times(1)
        .WillOnce(Return(negativeWeight));

    EXPECT_CALL(*m_mockModelPtr,
                doPawleyRefinement(*inputWorkspace, runLabel, instParams,
                                   phaseFiles, pathToGSASII, GSASIIProjectFile,
                                   dmin, negativeWeight))
        .Times(1)
        .WillOnce(Throw(std::runtime_error("Failure reason")));
    EXPECT_CALL(*m_mockViewPtr,
                userError("Refinement failed", "Failure reason"));

    presenter->notify(IEnggDiffGSASFittingPresenter::DoRefinement);
    assertMocksUsedCorrectly();
  }

  void test_selectValidRunFitResultsRequested() {
    auto presenter = setUpPresenter();
    const RunLabel runLabel(123, 1);

    EXPECT_CALL(*m_mockMultiRunWidgetPtr, showFitResultsSelected())
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(*m_mockMultiRunWidgetPtr, getSelectedRunLabel())
        .Times(1)
        .WillOnce(Return(runLabel));

    const double rwp = 50;
    EXPECT_CALL(*m_mockModelPtr, getRwp(runLabel))
        .Times(1)
        .WillOnce(Return(rwp));

    const auto latticeParams =
        Mantid::API::WorkspaceFactory::Instance().createTable();
    EXPECT_CALL(*m_mockModelPtr, getLatticeParams(runLabel))
        .Times(1)
        .WillOnce(Return(latticeParams));

    EXPECT_CALL(*m_mockViewPtr, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(*m_mockViewPtr, displayRwp(rwp)).Times(1);
    EXPECT_CALL(*m_mockViewPtr, displayLatticeParams(latticeParams)).Times(1);

    presenter->notify(IEnggDiffGSASFittingPresenter::SelectRun);
    assertMocksUsedCorrectly();
  }

  void test_selectRunInvalid() {
    auto presenter = setUpPresenter();
    const RunLabel runLabel(123, 1);

    EXPECT_CALL(*m_mockMultiRunWidgetPtr, showFitResultsSelected())
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(*m_mockMultiRunWidgetPtr, getSelectedRunLabel())
        .Times(1)
        .WillOnce(Return(runLabel));

    EXPECT_CALL(*m_mockModelPtr, getRwp(runLabel))
        .Times(1)
        .WillOnce(Return(50));
    EXPECT_CALL(*m_mockModelPtr, getLatticeParams(runLabel))
        .Times(1)
        .WillOnce(Return(boost::none));

    EXPECT_CALL(*m_mockViewPtr, userError("Invalid run label",
                                          "Tried to display fit results "
                                          "for run number 123 and bank ID 1. "
                                          "Please contact the development "
                                          "team with this message"));

    presenter->notify(IEnggDiffGSASFittingPresenter::SelectRun);
    assertMocksUsedCorrectly();
  }

  void test_selectRunFitResultsNotRequested() {
    auto presenter = setUpPresenter();
    EXPECT_CALL(*m_mockMultiRunWidgetPtr, showFitResultsSelected())
        .Times(1)
        .WillOnce(Return(false));
    EXPECT_CALL(*m_mockMultiRunWidgetPtr, getSelectedRunLabel()).Times(0);
    presenter->notify(IEnggDiffGSASFittingPresenter::SelectRun);
    assertMocksUsedCorrectly();
  }

private:
  MockEnggDiffGSASFittingModel *m_mockModelPtr;
  MockEnggDiffGSASFittingView *m_mockViewPtr;
  MockEnggDiffMultiRunFittingWidgetPresenter *m_mockMultiRunWidgetPtr;

  std::unique_ptr<EnggDiffGSASFittingPresenter> setUpPresenter() {
    auto mockModel = Mantid::Kernel::make_unique<
        testing::NiceMock<MockEnggDiffGSASFittingModel>>();
    m_mockModelPtr = mockModel.get();

    m_mockViewPtr = new testing::NiceMock<MockEnggDiffGSASFittingView>();

    auto mockMultiRunWidgetPresenter_sptr = boost::make_shared<
        testing::NiceMock<MockEnggDiffMultiRunFittingWidgetPresenter>>();
    m_mockMultiRunWidgetPtr = mockMultiRunWidgetPresenter_sptr.get();

    auto pres_uptr = Mantid::Kernel::make_unique<EnggDiffGSASFittingPresenter>(
        std::move(mockModel), m_mockViewPtr, mockMultiRunWidgetPresenter_sptr);
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
};

#endif // MANTID_CUSTOM_INTERFACES_ENGGDIFFGSASFITTINGPRESENTERTEST_H_
