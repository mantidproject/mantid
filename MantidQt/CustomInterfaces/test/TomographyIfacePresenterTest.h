#ifndef MANTID_CUSTOMINTERFACES_TOMOGRAPHYIFACEPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_TOMOGRAPHYIFACEPRESENTERTEST_H

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtCustomInterfaces/Tomography/TomographyIfacePresenter.h"

#include <cxxtest/TestSuite.h>
#include "TomographyViewMock.h"

using namespace MantidQt::CustomInterfaces;
using testing::TypedEq;
using testing::Return;

class TomographyIfacePresenterTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TomographyIfacePresenterTest *createSuite() {
    return new TomographyIfacePresenterTest();
  }

  static void destroySuite(TomographyIfacePresenterTest *suite) {
    delete suite;
  }

  TomographyIfacePresenterTest() {
    Mantid::API::FrameworkManager::Instance(); // make sure framework is
                                               // initialized
  }

  void setUp() {
    m_view.reset(new testing::NiceMock<MockTomographyIfaceView>());
    m_presenter.reset(
        new MantidQt::CustomInterfaces::TomographyIfacePresenter(m_view.get()));
  }

  void tearDown() {
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(m_view.get()));
  }

  void test_setupGood() {
    testing::NiceMock<MockTomographyIfaceView> mockView;
    MantidQt::CustomInterfaces::TomographyIfacePresenter pres(&mockView);

    // needs the basic paths at a very minimum
    EXPECT_CALL(mockView, currentPathsConfig())
        .Times(1)
        .WillOnce(Return(TomoPathsConfig()));

    EXPECT_CALL(mockView, enableLoggedActions(false)).Times(1);
    EXPECT_CALL(mockView, setComputeResources(testing::_, testing::_)).Times(1);
    EXPECT_CALL(mockView, setReconstructionTools(testing::_, testing::_))
        .Times(1);

    // No errors/warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ITomographyIfacePresenter::SetupResourcesAndTools);
  }

  void test_setupWithWrongTool() {
    testing::NiceMock<MockTomographyIfaceView> mockView;
    MantidQt::CustomInterfaces::TomographyIfacePresenter pres(&mockView);

    EXPECT_CALL(mockView, enableLoggedActions(false)).Times(1);
    EXPECT_CALL(mockView, setComputeResources(testing::_, testing::_)).Times(1);
    EXPECT_CALL(mockView, setReconstructionTools(testing::_, testing::_))
        .Times(1);

    // but this should be called once at setup time
    EXPECT_CALL(mockView, currentPathsConfig())
        .Times(1)
        .WillOnce(Return(TomoPathsConfig()));

    // One error, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ITomographyIfacePresenter::SetupResourcesAndTools);

    // needs one tool at a very minimum
    EXPECT_CALL(mockView, currentReconTool()).Times(1).WillOnce(Return(g_ccpi));
    // and basic tools settings
    EXPECT_CALL(mockView, reconToolsSettings()).Times(0);

    // tool config not available
    EXPECT_CALL(mockView, showToolConfig(testing::_)).Times(0);
    // this also implies that a second call to reconToolsSettings doesn't occur

    pres.notify(ITomographyIfacePresenter::SetupReconTool);
  }

  // does not really fail, but it cannot do any of the expected updates
  void test_setupReconToolUnsupportedTool() {
    testing::NiceMock<MockTomographyIfaceView> mockView;
    MantidQt::CustomInterfaces::TomographyIfacePresenter pres(&mockView);

    EXPECT_CALL(mockView, currentReconTool())
        .Times(1)
        .WillRepeatedly(Return(g_ccpi));
    EXPECT_CALL(mockView, reconToolsSettings()).Times(0);

    // wrong tool => doesn't have a config dialog
    EXPECT_CALL(mockView, showToolConfig(testing::_)).Times(0);

    // No errors/warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ITomographyIfacePresenter::SetupReconTool);
  }

  void test_setupReconToolGood() {
    testing::NiceMock<MockTomographyIfaceView> mockView;
    MantidQt::CustomInterfaces::TomographyIfacePresenter pres(&mockView);

    // needs the basic paths at a very minimum
    EXPECT_CALL(mockView, currentReconTool())
        .Times(2)
        .WillRepeatedly(Return("TomoPy"));
    // and basic tools settings
    TomoReconToolsUserSettings toolsSettings;
    EXPECT_CALL(mockView, reconToolsSettings())
        .Times(1)
        .WillOnce(Return(toolsSettings));

    EXPECT_CALL(mockView, showToolConfig(testing::_)).Times(1);

    // No errors/warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ITomographyIfacePresenter::SetupReconTool);
  }

  void test_showImg_fails() {
    testing::NiceMock<MockTomographyIfaceView> mockView;
    MantidQt::CustomInterfaces::TomographyIfacePresenter pres(&mockView);

    // No errors, but one warning
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);

    pres.notify(ITomographyIfacePresenter::ViewImg);
  }

  void test_showImg_good() {
    testing::NiceMock<MockTomographyIfaceView> mockView;
    MantidQt::CustomInterfaces::TomographyIfacePresenter pres(&mockView);

    const std::string path = "FITS_small_02.fits";
    // needs image file name - re-uses a FITS from the unit tests
    ON_CALL(mockView, showImagePath()).WillByDefault(Return(path));
    EXPECT_CALL(mockView, showImagePath()).Times(1);

    EXPECT_CALL(
        mockView,
        showImage(testing::Matcher<const Mantid::API::MatrixWorkspace_sptr &>(
            testing::_))).Times(1);
    EXPECT_CALL(mockView,
                showImage(testing::Matcher<const std::string &>(testing::_)))
        .Times(0);

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ITomographyIfacePresenter::ViewImg);
  }

  void test_valuesAtInit() {
    testing::NiceMock<MockTomographyIfaceView> mockView;
    MantidQt::CustomInterfaces::TomographyIfacePresenter pres(&mockView);

    // needs basic paths config - using defaults
    EXPECT_CALL(mockView, currentPathsConfig())
        .Times(1)
        .WillOnce(Return(TomoPathsConfig()));
    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ITomographyIfacePresenter::SetupResourcesAndTools);
  }

  void test_logOut_notIn() {
    testing::NiceMock<MockTomographyIfaceView> mockView;
    MantidQt::CustomInterfaces::TomographyIfacePresenter pres(&mockView);

    // would need compute resource and username if logged in
    EXPECT_CALL(mockView, getUsername()).Times(0);
    EXPECT_CALL(mockView, currentComputeResource()).Times(0);
    EXPECT_CALL(mockView, updateLoginControls(testing::_)).Times(0);

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ITomographyIfacePresenter::LogOutRequested);
  }

  void test_changeTool() {
    testing::NiceMock<MockTomographyIfaceView> mockView;
    MantidQt::CustomInterfaces::TomographyIfacePresenter pres(&mockView);

    std::vector<std::string> tools;
    tools.push_back("Astra Toolbox");
    tools.push_back("TomoPy");
    tools.push_back(g_ccpi);
    tools.push_back("Savu");

    TSM_ASSERT_EQUALS("There should be 4 tools in this test", tools.size(), 4);
    // up to this index the tools are supported
    const size_t indexToolsWork = 1;
    for (size_t i = 0; i < 3; i++) {
      EXPECT_CALL(mockView, currentReconTool())
          .Times(1)
          .WillOnce(Return(tools[i]));
      if (i <= indexToolsWork) {
        EXPECT_CALL(mockView, currentComputeResource()).Times(1);
      } else {
        EXPECT_CALL(mockView, currentComputeResource()).Times(0);
      }

      EXPECT_CALL(mockView, enableRunReconstruct(testing::_)).Times(1);
      EXPECT_CALL(mockView, enableConfigTool(testing::_)).Times(1);

      // No errors, no warnings
      EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
      EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

      pres.notify(ITomographyIfacePresenter::ToolChanged);
    }
  }

  void test_changeResource() {
    testing::NiceMock<MockTomographyIfaceView> mockView;
    MantidQt::CustomInterfaces::TomographyIfacePresenter pres(&mockView);

    EXPECT_CALL(mockView, currentComputeResource())
        .Times(1)
        .WillOnce(Return(g_scarfName));
    EXPECT_CALL(mockView, currentReconTool()).Times(0);

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ITomographyIfacePresenter::CompResourceChanged);
  }

  void test_changePaths() {
    testing::NiceMock<MockTomographyIfaceView> mockView;
    MantidQt::CustomInterfaces::TomographyIfacePresenter pres(&mockView);

    // needs some basic paths config - using defaults from constructor
    EXPECT_CALL(mockView, currentPathsConfig())
        .Times(1)
        .WillOnce(Return(TomoPathsConfig()));

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ITomographyIfacePresenter::TomoPathsChanged);
  }

  void test_loginFails() {
    testing::NiceMock<MockTomographyIfaceView> mockView;
    MantidQt::CustomInterfaces::TomographyIfacePresenter pres(&mockView);

    // 1 Error, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(1);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ITomographyIfacePresenter::LogInRequested);
  }

  void test_runFails() {
    testing::NiceMock<MockTomographyIfaceView> mockView;
    MantidQt::CustomInterfaces::TomographyIfacePresenter pres(&mockView);

    // needs basic paths config - using defaults
    EXPECT_CALL(mockView, currentPathsConfig())
        .Times(1)
        .WillOnce(Return(TomoPathsConfig()));
    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);
    pres.notify(ITomographyIfacePresenter::SetupResourcesAndTools);
  }

  void test_log() {
    testing::NiceMock<MockTomographyIfaceView> mockView;
    MantidQt::CustomInterfaces::TomographyIfacePresenter pres(&mockView);

    std::vector<std::string> msgs;
    msgs.push_back("foo log");
    msgs.push_back("baz log");

    EXPECT_CALL(mockView, logMsgs()).Times(1).WillOnce(Return(msgs));

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ITomographyIfacePresenter::LogMsg);
  }

  void test_refreshJobsNotIn() {
    testing::NiceMock<MockTomographyIfaceView> mockView;
    MantidQt::CustomInterfaces::TomographyIfacePresenter pres(&mockView);

    EXPECT_CALL(mockView, currentComputeResource()).Times(0);
    EXPECT_CALL(mockView, updateJobsInfoDisplay(testing::_, testing::_))
        .Times(1);

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ITomographyIfacePresenter::RefreshJobs);
  }

  void test_cancelJobNotIn() {
    testing::NiceMock<MockTomographyIfaceView> mockView;
    MantidQt::CustomInterfaces::TomographyIfacePresenter pres(&mockView);

    EXPECT_CALL(mockView, currentComputeResource()).Times(0);
    EXPECT_CALL(mockView, updateJobsInfoDisplay(testing::_, testing::_))
        .Times(1);

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ITomographyIfacePresenter::RefreshJobs);
  }

  void test_filtersSettings() {
    TomoReconFiltersSettings def;

    // When the presenter uses these settings for job submission there
    // the test should be extended with something like this:
    // EXPECT_CALL(mockView, prePostProcSettings()).Times(1).WillOnce(
    //             Return("TomoPy"));

    TSM_ASSERT_EQUALS("Pre-processing filter settings default values should be "
                      "as expected (proton-charge normalization)",
                      def.prep.normalizeByFlatDark, true);

    TSM_ASSERT_EQUALS("Pre-processing settings default values should be as "
                      "expected (flat/dark normalization)",
                      def.prep.normalizeByProtonCharge, true);

    TSM_ASSERT_EQUALS("Pre-processing settings default values should be as "
                      "expected (median filter width)",
                      def.prep.medianFilterWidth, 3);

    TSM_ASSERT_EQUALS("Pre-processing settings default values should be as "
                      "expected (rotation)",
                      def.prep.rotation, 0);

    TSM_ASSERT_EQUALS("Pre-processing settings default values should be as "
                      "expected (maximum angle)",
                      def.prep.maxAngle, 360.0);

    TSM_ASSERT_LESS_THAN_EQUALS(
        "Pre-processing settings default values should be as "
        "expected (scale down)",
        def.prep.scaleDownFactor, 1);

    TSM_ASSERT_DELTA("Post-processing settings default values should be as "
                     "expected (circular mask)",
                     def.postp.circMaskRadius, 0.94, 1e-5);

    TSM_ASSERT_EQUALS("Post-processing settings default values should be as "
                      "expected (cut-off)",
                      def.postp.cutOffLevel, 0.0);

    TSM_ASSERT_EQUALS("Output settings default values should be as expected",
                      def.outputPreprocImages, true);
  }

  // An attempt at testing a sequence of steps from the user.
  // TODO: more interesting sessions should follow
  void test_sillySession() {
    // the user does a few silly things...
    testing::NiceMock<MockTomographyIfaceView> mockView;
    MantidQt::CustomInterfaces::TomographyIfacePresenter pres(&mockView);

    EXPECT_CALL(mockView, currentPathsConfig())
        .Times(1)
        .WillOnce(Return(TomoPathsConfig()));

    // user changes some paths
    pres.notify(ITomographyIfacePresenter::TomoPathsChanged);

    EXPECT_CALL(mockView, currentComputeResource())
        .Times(2)
        .WillRepeatedly(Return(g_scarfName));

    // user changes the compute resource
    pres.notify(ITomographyIfacePresenter::CompResourceChanged);

    EXPECT_CALL(mockView, currentReconTool())
        .Times(2)
        .WillRepeatedly(Return("TomoPy"));

    TomoReconToolsUserSettings toolsSettings;
    EXPECT_CALL(mockView, reconToolsSettings())
        .Times(1)
        .WillOnce(Return(toolsSettings));

    // user opens dialog and sets up a reconstruction tool
    pres.notify(ITomographyIfacePresenter::SetupReconTool);

    TomoPathsConfig pathsCfg;
    EXPECT_CALL(mockView, currentPathsConfig())
        .Times(1)
        .WillOnce(Return(pathsCfg));

    ImageStackPreParams roiEtc;
    EXPECT_CALL(mockView, currentROIEtcParams())
        .Times(1)
        .WillOnce(Return(roiEtc));

    EXPECT_CALL(mockView, tomopyMethod()).Times(1).WillOnce(Return(""));

    EXPECT_CALL(mockView, astraMethod()).Times(1).WillOnce(Return(""));

    TomoReconFiltersSettings filters;
    EXPECT_CALL(mockView, prePostProcSettings())
        .Times(1)
        .WillOnce(Return(filters));

    EXPECT_CALL(mockView, externalInterpreterPath())
        .Times(1)
        .WillOnce(Return(""));

    EXPECT_CALL(mockView, pathLocalReconScripts())
        .Times(1)
        .WillOnce(Return(""));

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    // finally, user tries to run a reconstruction job
    pres.notify(ITomographyIfacePresenter::RunReconstruct);
  }

  void test_shutDown() {
    testing::NiceMock<MockTomographyIfaceView> mockView;
    MantidQt::CustomInterfaces::TomographyIfacePresenter pres(&mockView);

    EXPECT_CALL(mockView, saveSettings()).Times(1);
    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ITomographyIfacePresenter::ShutDown);
  }

private:
  // boost::shared_ptr
  boost::scoped_ptr<testing::NiceMock<MockTomographyIfaceView>> m_view;
  // MockTomographyIfaceModel *m_model;
  boost::scoped_ptr<MantidQt::CustomInterfaces::TomographyIfacePresenter>
      m_presenter;
  // To have one FITS, etc.
  Mantid::API::MatrixWorkspace_sptr m_ws;
  static std::string g_scarfName;
  static std::string g_ccpi;
};

std::string TomographyIfacePresenterTest::g_scarfName = "SCARF@STFC";
std::string TomographyIfacePresenterTest::g_ccpi = "CCPi CGLS";

#endif // MANTID_CUSTOMINTERFACES_TOMOGRAPHYIFACEPRESENTERTEST_H
