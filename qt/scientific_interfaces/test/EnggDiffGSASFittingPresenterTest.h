#ifndef MANTID_CUSTOM_INTERFACES_ENGGDIFFGSASFITTINGPRESENTERTEST_H_
#define MANTID_CUSTOM_INTERFACES_ENGGDIFFGSASFITTINGPRESENTERTEST_H_

#include "../EnggDiffraction/EnggDiffGSASFittingPresenter.h"
#include "EnggDiffGSASFittingModelMock.h"
#include "EnggDiffGSASFittingViewMock.h"

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
    auto mockModel = Mantid::Kernel::make_unique<
        testing::NiceMock<MockEnggDiffGSASFittingModel>>();
    auto mockModel_ptr = mockModel.get();

    auto mockView = Mantid::Kernel::make_unique<
        testing::NiceMock<MockEnggDiffGSASFittingView>>();
    auto mockView_ptr = mockView.get();

    EnggDiffGSASFittingPresenter pres(std::move(mockModel),
                                      std::move(mockView));

    const auto filename = "Valid filename";

    EXPECT_CALL(*mockView_ptr, getFocusedFileName())
        .Times(1)
        .WillOnce(Return(filename));
    EXPECT_CALL(*mockModel_ptr, loadFocusedRun(filename))
        .Times(1)
        .WillOnce(Return(""));

    const std::vector<std::pair<int, size_t>> runLabels(
        {std::make_pair(123, 1)});

    EXPECT_CALL(*mockModel_ptr, getRunLabels())
        .Times(1)
        .WillOnce(Return(runLabels));
    EXPECT_CALL(*mockView_ptr, updateRunList(runLabels)).Times(1);

    EXPECT_CALL(*mockView_ptr, userWarning(testing::_)).Times(0);

    pres.notify(IEnggDiffGSASFittingPresenter::LoadRun);
    TSM_ASSERT("View mock not used as expected: some EXPECT_CALL conditions "
               "not satisfied",
               testing::Mock::VerifyAndClearExpectations(mockView_ptr));
    TSM_ASSERT("Model mock not used as expected: some EXPECT_CALL conditions "
               "not satisfied",
               testing::Mock::VerifyAndClearExpectations(mockModel_ptr));
  }

  void test_loadInvalidFile() {
    auto mockModel = Mantid::Kernel::make_unique<
        testing::NiceMock<MockEnggDiffGSASFittingModel>>();
    auto mockModel_ptr = mockModel.get();

    auto mockView = Mantid::Kernel::make_unique<
        testing::NiceMock<MockEnggDiffGSASFittingView>>();
    auto mockView_ptr = mockView.get();

    EnggDiffGSASFittingPresenter pres(std::move(mockModel),
                                      std::move(mockView));

    const auto filename = "Invalid filename";

    EXPECT_CALL(*mockView_ptr, getFocusedFileName())
        .Times(1)
        .WillOnce(Return(filename));

    EXPECT_CALL(*mockModel_ptr, loadFocusedRun(filename))
        .Times(1)
        .WillOnce(Return(false));

    EXPECT_CALL(*mockModel_ptr, getRunLabels()).Times(0);

    EXPECT_CALL(*mockView_ptr,
                userWarning("Load failed, see the log for more details"))
        .Times(1);

    pres.notify(IEnggDiffGSASFittingPresenter::LoadRun);
    TSM_ASSERT("View mock not used as expected: some EXPECT_CALL conditions "
               "not satisfied",
               testing::Mock::VerifyAndClearExpectations(mockView_ptr));
    TSM_ASSERT("Model mock not used as expected: some EXPECT_CALL conditions "
               "not satisfied",
               testing::Mock::VerifyAndClearExpectations(mockModel_ptr));
  }

  void test_selectValidRun() {
    auto mockModel = Mantid::Kernel::make_unique<
        testing::NiceMock<MockEnggDiffGSASFittingModel>>();
    auto mockModel_ptr = mockModel.get();

    auto mockView = Mantid::Kernel::make_unique<
        testing::NiceMock<MockEnggDiffGSASFittingView>>();
    auto mockView_ptr = mockView.get();

    EnggDiffGSASFittingPresenter pres(std::move(mockModel),
                                      std::move(mockView));

    const std::pair<int, size_t> selectedRunLabel = std::make_pair(123, 1);
    EXPECT_CALL(*mockView_ptr, getSelectedRunLabel())
        .Times(1)
        .WillOnce(Return(selectedRunLabel));

    const boost::optional<Mantid::API::MatrixWorkspace_sptr> sampleWorkspace(
        WorkspaceCreationHelper::create2DWorkspaceBinned(1, 100));

    EXPECT_CALL(*mockModel_ptr, getFocusedWorkspace(123, 1))
        .Times(1)
        .WillOnce(Return(sampleWorkspace));

    EXPECT_CALL(*mockView_ptr, resetCanvas()).Times(1);
    EXPECT_CALL(*mockView_ptr, userWarning(testing::_)).Times(0);

    pres.notify(IEnggDiffGSASFittingPresenter::SelectRun);
    TSM_ASSERT("View mock not used as expected: some EXPECT_CALL conditions "
               "not satisfied",
               testing::Mock::VerifyAndClearExpectations(mockView_ptr));
    TSM_ASSERT("Model mock not used as expected: some EXPECT_CALL conditions "
               "not satisfied",
               testing::Mock::VerifyAndClearExpectations(mockModel_ptr));
  }

  void test_selectInvalidRun() {
    auto mockModel = Mantid::Kernel::make_unique<
        testing::NiceMock<MockEnggDiffGSASFittingModel>>();
    auto mockModel_ptr = mockModel.get();

    auto mockView = Mantid::Kernel::make_unique<
        testing::NiceMock<MockEnggDiffGSASFittingView>>();
    auto mockView_ptr = mockView.get();

    EnggDiffGSASFittingPresenter pres(std::move(mockModel),
                                      std::move(mockView));

    const std::pair<int, size_t> selectedRunLabel = std::make_pair(123, 1);
    EXPECT_CALL(*mockView_ptr, getSelectedRunLabel())
        .Times(1)
        .WillOnce(Return(selectedRunLabel));

    EXPECT_CALL(*mockModel_ptr, getFocusedWorkspace(123, 1))
        .Times(1)
        .WillOnce(Return(boost::none));

    EXPECT_CALL(
        *mockView_ptr,
        userWarning(
            "Tried to access invalid run, runNumber 123 and bank ID 1"));

    EXPECT_CALL(*mockView_ptr, resetCanvas()).Times(0);

    pres.notify(IEnggDiffGSASFittingPresenter::SelectRun);
    TSM_ASSERT("View mock not used as expected: some EXPECT_CALL conditions "
               "not satisfied",
               testing::Mock::VerifyAndClearExpectations(mockView_ptr));
    TSM_ASSERT("Model mock not used as expected: some EXPECT_CALL conditions "
               "not satisfied",
               testing::Mock::VerifyAndClearExpectations(mockModel_ptr));
  }

  void test_selectValidRunPlotFitResults() {
    auto mockModel = Mantid::Kernel::make_unique<
        testing::NiceMock<MockEnggDiffGSASFittingModel>>();
    auto mockModel_ptr = mockModel.get();

    auto mockView = Mantid::Kernel::make_unique<
        testing::NiceMock<MockEnggDiffGSASFittingView>>();
    auto mockView_ptr = mockView.get();

    EnggDiffGSASFittingPresenter pres(std::move(mockModel),
                                      std::move(mockView));

    const std::pair<int, size_t> selectedRunLabel = std::make_pair(123, 1);
    EXPECT_CALL(*mockView_ptr, getSelectedRunLabel())
        .Times(1)
        .WillOnce(Return(selectedRunLabel));

    const boost::optional<Mantid::API::MatrixWorkspace_sptr> sampleWorkspace(
        WorkspaceCreationHelper::create2DWorkspaceBinned(1, 100));
    EXPECT_CALL(*mockModel_ptr, getFocusedWorkspace(123, 1))
        .Times(1)
        .WillOnce(Return(sampleWorkspace));
    EXPECT_CALL(*mockView_ptr, showRefinementResultsSelected())
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(*mockModel_ptr, hasFittedPeaksForRun(123, 1))
        .Times(1)
        .WillOnce(Return(true));

    EXPECT_CALL(*mockModel_ptr, getFittedPeaks(123, 1))
        .Times(1)
        .WillOnce(Return(sampleWorkspace));

    const boost::optional<Mantid::API::ITableWorkspace_sptr> emptyTableWS(
        Mantid::API::WorkspaceFactory::Instance().createTable());

    EXPECT_CALL(*mockModel_ptr, getLatticeParams(123, 1))
        .Times(1)
        .WillOnce(Return(emptyTableWS));

    const boost::optional<double> rwp = 50.0;
    EXPECT_CALL(*mockModel_ptr, getRwp(123, 1)).Times(1).WillOnce(Return(rwp));

    EXPECT_CALL(*mockView_ptr, resetCanvas()).Times(1);
    EXPECT_CALL(*mockView_ptr, plotCurve(testing::_)).Times(2);
    EXPECT_CALL(*mockView_ptr, userWarning(testing::_)).Times(0);

    pres.notify(IEnggDiffGSASFittingPresenter::SelectRun);
    TSM_ASSERT("View mock not used as expected: some EXPECT_CALL conditions "
               "not satisfied",
               testing::Mock::VerifyAndClearExpectations(mockView_ptr));
    TSM_ASSERT("Model mock not used as expected: some EXPECT_CALL conditions "
               "not satisfied",
               testing::Mock::VerifyAndClearExpectations(mockModel_ptr));
  }

  void test_doRietveldRefinement() {
    auto mockModel = Mantid::Kernel::make_unique<
        testing::NiceMock<MockEnggDiffGSASFittingModel>>();
    auto mockModel_ptr = mockModel.get();

    auto mockView = Mantid::Kernel::make_unique<
        testing::NiceMock<MockEnggDiffGSASFittingView>>();
    auto mockView_ptr = mockView.get();

    EnggDiffGSASFittingPresenter pres(std::move(mockModel),
                                      std::move(mockView));

    const int runNumber = 123;
    const size_t bank = 1;
    const auto runLabel = std::make_pair(runNumber, bank);
    const auto refinementMethod = GSASRefinementMethod::RIETVELD;
    const auto instParams = "Instrument file";
    const std::vector<std::string> phaseFiles({"phase1", "phase2"});
    const auto pathToGSASII = "GSASHOME";
    const auto GSASIIProjectFile = "GPX.gpx";

    EXPECT_CALL(*mockView_ptr, getSelectedRunLabel())
        .Times(1)
        .WillOnce(Return(runLabel));
    EXPECT_CALL(*mockView_ptr, getRefinementMethod())
        .Times(1)
        .WillOnce(Return(refinementMethod));
    EXPECT_CALL(*mockView_ptr, getInstrumentFileName())
        .Times(1)
        .WillOnce(Return(instParams));
    EXPECT_CALL(*mockView_ptr, getPhaseFileNames())
        .Times(1)
        .WillOnce(Return(phaseFiles));
    EXPECT_CALL(*mockView_ptr, getPathToGSASII())
        .Times(1)
        .WillOnce(Return(pathToGSASII));
    EXPECT_CALL(*mockView_ptr, getGSASIIProjectPath())
        .Times(1)
        .WillOnce(Return(GSASIIProjectFile));

    EXPECT_CALL(*mockModel_ptr,
                doRietveldRefinement(runNumber, bank, instParams, phaseFiles,
                                     pathToGSASII, GSASIIProjectFile))
        .Times(1)
        .WillOnce(Return(false));
    EXPECT_CALL(*mockView_ptr,
                userWarning("Refinement failed, see the log for more details"));

    pres.notify(IEnggDiffGSASFittingPresenter::DoRefinement);
    TSM_ASSERT("View mock not used as expected: some EXPECT_CALL conditions "
               "not satisfied",
               testing::Mock::VerifyAndClearExpectations(mockView_ptr));
    TSM_ASSERT("Model mock not used as expected: some EXPECT_CALL conditions "
               "not satisfied",
               testing::Mock::VerifyAndClearExpectations(mockModel_ptr));
  }

  void test_doPawleyRefinement() {
    auto mockModel = Mantid::Kernel::make_unique<
        testing::NiceMock<MockEnggDiffGSASFittingModel>>();
    auto mockModel_ptr = mockModel.get();

    auto mockView = Mantid::Kernel::make_unique<
        testing::NiceMock<MockEnggDiffGSASFittingView>>();
    auto mockView_ptr = mockView.get();

    EnggDiffGSASFittingPresenter pres(std::move(mockModel),
                                      std::move(mockView));

    const int runNumber = 123;
    const size_t bank = 1;
    const auto runLabel = std::make_pair(runNumber, bank);
    const auto refinementMethod = GSASRefinementMethod::PAWLEY;
    const auto instParams = "Instrument file";
    const std::vector<std::string> phaseFiles({"phase1", "phase2"});
    const auto pathToGSASII = "GSASHOME";
    const auto GSASIIProjectFile = "GPX.gpx";
    const auto dmin = 1.0;
    const auto negativeWeight = 2.0;

    EXPECT_CALL(*mockView_ptr, getSelectedRunLabel())
        .Times(1)
        .WillOnce(Return(runLabel));
    EXPECT_CALL(*mockView_ptr, getRefinementMethod())
        .Times(1)
        .WillOnce(Return(refinementMethod));
    EXPECT_CALL(*mockView_ptr, getInstrumentFileName())
        .Times(1)
        .WillOnce(Return(instParams));
    EXPECT_CALL(*mockView_ptr, getPhaseFileNames())
        .Times(1)
        .WillOnce(Return(phaseFiles));
    EXPECT_CALL(*mockView_ptr, getPathToGSASII())
        .Times(1)
        .WillOnce(Return(pathToGSASII));
    EXPECT_CALL(*mockView_ptr, getGSASIIProjectPath())
        .Times(1)
        .WillOnce(Return(GSASIIProjectFile));
    EXPECT_CALL(*mockView_ptr, getPawleyDMin()).Times(1).WillOnce(Return(dmin));
    EXPECT_CALL(*mockView_ptr, getPawleyNegativeWeight())
        .Times(1)
        .WillOnce(Return(negativeWeight));

    EXPECT_CALL(*mockModel_ptr,
                doPawleyRefinement(runNumber, bank, instParams, phaseFiles,
                                   pathToGSASII, GSASIIProjectFile, dmin,
                                   negativeWeight))
        .Times(1)
        .WillOnce(Return(false));
    EXPECT_CALL(*mockView_ptr,
                userWarning("Refinement failed, see the log for more details"));

    pres.notify(IEnggDiffGSASFittingPresenter::DoRefinement);
    TSM_ASSERT("View mock not used as expected: some EXPECT_CALL conditions "
               "not satisfied",
               testing::Mock::VerifyAndClearExpectations(mockView_ptr));
    TSM_ASSERT("Model mock not used as expected: some EXPECT_CALL conditions "
               "not satisfied",
               testing::Mock::VerifyAndClearExpectations(mockModel_ptr));
  }
};

#endif // MANTID_CUSTOM_INTERFACES_ENGGDIFFGSASFITTINGPRESENTERTEST_H_
