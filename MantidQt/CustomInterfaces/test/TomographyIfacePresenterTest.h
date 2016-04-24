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

  void setUp() override {
    m_view.reset(new testing::NiceMock<MockTomographyIfaceView>());
    m_presenter.reset(
        new MantidQt::CustomInterfaces::TomographyIfacePresenter(m_view.get()));
  }

  void tearDown() override {
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(m_view.get()));
  }

  void test_setupSystemSettings() {
    testing::NiceMock<MockTomographyIfaceView> mockView;
    MantidQt::CustomInterfaces::TomographyIfacePresenter pres(&mockView);

    // system settings should be self-contained - no need for any of these:
    EXPECT_CALL(mockView, currentPathsConfig()).Times(0);
    EXPECT_CALL(mockView, enableLoggedActions(false)).Times(0);
    EXPECT_CALL(mockView, setComputeResources(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, setReconstructionTools(testing::_, testing::_))
        .Times(0);

    EXPECT_CALL(mockView, systemSettings())
        .Times(1)
        .WillOnce(Return(TomoSystemSettings()));

    // No errors/warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ITomographyIfacePresenter::SystemSettingsUpdated);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_setupGood() {
    testing::NiceMock<MockTomographyIfaceView> mockView;
    MantidQt::CustomInterfaces::TomographyIfacePresenter pres(&mockView);

    // needs the basic paths at a very minimum
    EXPECT_CALL(mockView, currentPathsConfig())
        .Times(1)
        .WillOnce(Return(TomoPathsConfig()));

    EXPECT_CALL(mockView, systemSettings())
        .Times(1)
        .WillOnce(Return(TomoSystemSettings()));
    EXPECT_CALL(mockView, enableLoggedActions(false)).Times(1);
    EXPECT_CALL(mockView, setComputeResources(testing::_, testing::_)).Times(1);
    EXPECT_CALL(mockView, setReconstructionTools(testing::_, testing::_))
        .Times(1);

    // No errors/warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ITomographyIfacePresenter::SetupResourcesAndTools);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_setupWithWrongTool() {
    testing::NiceMock<MockTomographyIfaceView> mockView;
    MantidQt::CustomInterfaces::TomographyIfacePresenter pres(&mockView);

    EXPECT_CALL(mockView, systemSettings())
        .Times(1)
        .WillOnce(Return(TomoSystemSettings()));
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
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  // does not really fail, but it cannot do any of the expected updates
  void test_setupReconToolUnsupportedTool() {
    testing::NiceMock<MockTomographyIfaceView> mockView;
    MantidQt::CustomInterfaces::TomographyIfacePresenter pres(&mockView);

    EXPECT_CALL(mockView, systemSettings()).Times(0);
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
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_setupReconToolGood() {
    testing::NiceMock<MockTomographyIfaceView> mockView;
    MantidQt::CustomInterfaces::TomographyIfacePresenter pres(&mockView);

    EXPECT_CALL(mockView, systemSettings()).Times(0);
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
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_showImg_fails() {
    testing::NiceMock<MockTomographyIfaceView> mockView;
    MantidQt::CustomInterfaces::TomographyIfacePresenter pres(&mockView);

    // No errors, but one warning
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(1);

    pres.notify(ITomographyIfacePresenter::ViewImg);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
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
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_valuesAtInit() {
    testing::NiceMock<MockTomographyIfaceView> mockView;
    MantidQt::CustomInterfaces::TomographyIfacePresenter pres(&mockView);

    EXPECT_CALL(mockView, systemSettings())
        .Times(1)
        .WillOnce(Return(TomoSystemSettings()));
    // needs basic paths config - using defaults
    EXPECT_CALL(mockView, currentPathsConfig())
        .Times(1)
        .WillOnce(Return(TomoPathsConfig()));
    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ITomographyIfacePresenter::SetupResourcesAndTools);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
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
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_changeTool() {
    testing::NiceMock<MockTomographyIfaceView> mockView;
    MantidQt::CustomInterfaces::TomographyIfacePresenter pres(&mockView);

    std::vector<std::string> tools;
    tools.emplace_back("Astra Toolbox");
    tools.emplace_back("TomoPy");
    tools.push_back(g_ccpi);
    tools.emplace_back("Savu");

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
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
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
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_changePathsWithBrowseEmpty() {
    testing::NiceMock<MockTomographyIfaceView> mockView;
    MantidQt::CustomInterfaces::TomographyIfacePresenter pres(&mockView);

    EXPECT_CALL(mockView, updatePathsConfig(testing::_)).Times(0);
    EXPECT_CALL(mockView, experimentReference()).Times(0);

    // needs some basic paths config - using defaults from constructor
    EXPECT_CALL(mockView, currentPathsConfig())
        .Times(1)
        .WillOnce(Return(TomoPathsConfig()));

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ITomographyIfacePresenter::TomoPathsChanged);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_changePathsWithBrowseNonEmpty() {
    for (bool enableFlatDark : {true, false}) {
      testing::NiceMock<MockTomographyIfaceView> mockView;
      MantidQt::CustomInterfaces::TomographyIfacePresenter pres(&mockView);

      TomoPathsConfig cfg;
      cfg.updatePathSamples("/nowhere/foo_samples");
      cfg.updatePathOpenBeam("/nonexistent/bla_ob", enableFlatDark);
      cfg.updatePathDarks("/missing_place/bla_dark", enableFlatDark);
      mockView.updatePathsConfig(cfg);

      EXPECT_CALL(mockView, updatePathsConfig(testing::_)).Times(0);
      EXPECT_CALL(mockView, experimentReference()).Times(0);
      EXPECT_CALL(
          mockView,
          showImage(testing::Matcher<const Mantid::API::MatrixWorkspace_sptr &>(
              testing::_))).Times(0);
      EXPECT_CALL(mockView,
                  showImage(testing::Matcher<const std::string &>(testing::_)))
          .Times(0);

      // needs some basic paths config - using test / inexistent paths
      EXPECT_CALL(mockView, currentPathsConfig())
          .Times(1)
          .WillOnce(Return(cfg));

      // No errors, no warnings
      EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
      EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

      pres.notify(ITomographyIfacePresenter::TomoPathsChanged);
      TSM_ASSERT(
          "Mock not used as expected. Some EXPECT_CALL conditions were not "
          "satisfied.",
          testing::Mock::VerifyAndClearExpectations(&mockView))
    }
  }

  void test_changePathsEditingByHandEmpty() {
    testing::NiceMock<MockTomographyIfaceView> mockView;
    MantidQt::CustomInterfaces::TomographyIfacePresenter pres(&mockView);

    EXPECT_CALL(mockView, updatePathsConfig(testing::_)).Times(1);
    EXPECT_CALL(mockView, experimentReference()).Times(0);

    // needs some basic paths config - using defaults from constructor
    EXPECT_CALL(mockView, currentPathsConfig())
        .Times(1)
        .WillOnce(Return(TomoPathsConfig()));

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ITomographyIfacePresenter::TomoPathsEditedByUser);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_changePathsEditingByHand() {
    testing::NiceMock<MockTomographyIfaceView> mockView;
    MantidQt::CustomInterfaces::TomographyIfacePresenter pres(&mockView);

    EXPECT_CALL(mockView, experimentReference()).Times(0);

    // give only the samples path
    TomoPathsConfig cfg;
    cfg.updatePathSamples("/nowhere/foo_samples");
    mockView.updatePathsConfig(cfg);

    EXPECT_CALL(mockView, updatePathsConfig(testing::_)).Times(1);
    EXPECT_CALL(
        mockView,
        showImage(testing::Matcher<const Mantid::API::MatrixWorkspace_sptr &>(
            testing::_))).Times(0);
    EXPECT_CALL(mockView,
                showImage(testing::Matcher<const std::string &>(testing::_)))
        .Times(0);

    // needs some basic paths config - using defaults from constructor
    EXPECT_CALL(mockView, currentPathsConfig())
        .Times(1)
        .WillOnce(Return(TomoPathsConfig()));

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ITomographyIfacePresenter::TomoPathsEditedByUser);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_loginFails() {
    testing::NiceMock<MockTomographyIfaceView> mockView;
    MantidQt::CustomInterfaces::TomographyIfacePresenter pres(&mockView);

    // 1 Error, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(1);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ITomographyIfacePresenter::LogInRequested);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_runFails() {
    testing::NiceMock<MockTomographyIfaceView> mockView;
    MantidQt::CustomInterfaces::TomographyIfacePresenter pres(&mockView);

    EXPECT_CALL(mockView, systemSettings())
        .Times(1)
        .WillOnce(Return(TomoSystemSettings()));

    // needs basic paths config - using defaults
    EXPECT_CALL(mockView, currentPathsConfig())
        .Times(1)
        .WillOnce(Return(TomoPathsConfig()));

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);
    pres.notify(ITomographyIfacePresenter::SetupResourcesAndTools);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_log() {
    testing::NiceMock<MockTomographyIfaceView> mockView;
    MantidQt::CustomInterfaces::TomographyIfacePresenter pres(&mockView);

    std::vector<std::string> msgs;
    msgs.emplace_back("foo log");
    msgs.emplace_back("baz log");

    EXPECT_CALL(mockView, logMsgs()).Times(1).WillOnce(Return(msgs));

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ITomographyIfacePresenter::LogMsg);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
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
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
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
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_systemSettingsDefs() {
    TomoSystemSettings sets;

    const std::string msg = "Global tomography system settings default values "
                            "should be as expected";

    TSM_ASSERT_EQUALS(msg + " (number of path components for image data)",
                      sets.m_pathComponents.size(), 4);

    TSM_ASSERT_EQUALS(msg + " (prefix for sample images dir)",
                      sets.m_samplesDirPrefix, "data");

    TSM_ASSERT_EQUALS(msg + " (prefix for flat/open beam images dir)",
                      sets.m_flatsDirPrefix, "flat");

    TSM_ASSERT_EQUALS(msg + " (prefix for dark images dir)",
                      sets.m_darksDirPrefix, "dark");

    TSM_ASSERT_EQUALS(
        msg + " (path component in the output for pre-processed images)",
        sets.m_outputPathCompPreProcessed, "pre_processed");

    TSM_ASSERT_EQUALS(msg + " (path component in the output for "
                            "processed/reconstructed images/volumes)",
                      sets.m_outputPathCompReconst, "processed");
  }

  void test_systemSettingsDefsLocal() {
    TomoSystemSettings sets;

    const std::string msg =
        "Local system settings default values should be as expected";

    // Normally this would be 3 for Win32 (example: D:\) and
    // >=5 otherwise
    const std::string path = sets.m_local.m_basePathTomoData;
    TSM_ASSERT_LESS_THAN_EQUALS(
        msg + " (base path for tomography data too short)", 3, path.length());
  }

  void test_systemSettingsDefsRemote() {
    TomoSystemSettings sets;

    const std::string msg =
        "Remote system settings default values should be as expected";

    TSM_ASSERT_EQUALS(msg + " (base path for tomography data)",
                      sets.m_remote.m_basePathTomoData, "/work/imat");

    TSM_ASSERT_EQUALS(msg + " (base path for reconstruction scripts)",
                      sets.m_remote.m_basePathReconScripts,
                      "/work/imat/phase_commissioning");
  }

  void test_filtersSettings() {
    TomoReconFiltersSettings def;

    // When the presenter uses these settings for job submission there
    // the test should be extended with something like this:
    // EXPECT_CALL(mockView, prePostProcSettings()).Times(1).WillOnce(
    //             Return("TomoPy"));

    TSM_ASSERT_EQUALS("Pre-processing filter settings default values should be "
                      "as expected (normalization by air region)",
                      def.prep.normalizeByAirRegion, true);

    TSM_ASSERT_EQUALS("Pre-processing settings default values should be as "
                      "expected (proton-charge normalization)",
                      def.prep.normalizeByProtonCharge, false);

    TSM_ASSERT_EQUALS("Pre-processing filter settings default values should be "
                      "as expected (normalization by flat images)",
                      def.prep.normalizeByFlats, true);

    TSM_ASSERT_EQUALS("Pre-processing filter settings default values should be "
                      "as expected (normalization by dark images)",
                      def.prep.normalizeByDarks, true);

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

  void test_aggregateBandsFailsMissingPaths() {
    testing::NiceMock<MockTomographyIfaceView> mockView;
    MantidQt::CustomInterfaces::TomographyIfacePresenter pres(&mockView);

    // Empty params, missing the required ones for sure
    EXPECT_CALL(mockView, currentAggregateBandsParams())
        .Times(1)
        .WillOnce(Return(std::map<std::string, std::string>()));

    // Error
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(1);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ITomographyIfacePresenter::AggregateEnergyBands);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_aggregateBandsFailsWrongPaths() {
    testing::NiceMock<MockTomographyIfaceView> mockView;
    MantidQt::CustomInterfaces::TomographyIfacePresenter pres(&mockView);

    std::map<std::string, std::string> params;
    params["InputPath"] = "I_cannot_be_found_fail_";
    params["OutputPath"] = "fail_for_sure_i_m_not_found";
    params["UniformBands"] = "2";
    params["OutPrefixBands"] = "band_prefix_";
    params["InuptImageFormat"] = "FITS";
    params["OutputImageFormat"] = "FITS";

    // Will contain wrong values in the required paths
    EXPECT_CALL(mockView, currentAggregateBandsParams())
        .Times(1)
        .WillOnce(Return(params));

    // Error
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(1);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ITomographyIfacePresenter::AggregateEnergyBands);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  // disabled as this is I/O intensive and uses the algorithm runner (Qt)
  void disabled_test_aggregateBandsRuns() {
    testing::NiceMock<MockTomographyIfaceView> mockView;
    MantidQt::CustomInterfaces::TomographyIfacePresenter pres(&mockView);

    std::map<std::string, std::string> params;
    params["InputPath"] = "here_some_valid_input_images_path_";
    params["OutputPath"] = "here_some_valid_existing_output_path";
    params["UniformBands"] = "1";
    params["OutPrefixBands"] = "band_prefix_";
    params["InputImageFormat"] = "FITS";
    params["OutputImageFormat"] = "FITS";

    EXPECT_CALL(mockView, runAggregateBands(testing::_)).Times(1);

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ITomographyIfacePresenter::AggregateEnergyBands);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  // An attempt at testing a sequence of steps from the user.
  // TODO: more interesting sessions should follow, but how to do it
  // without loading too many and too big files?
  void test_sillySession() {
    // the user does a few silly things...
    testing::NiceMock<MockTomographyIfaceView> mockView;
    MantidQt::CustomInterfaces::TomographyIfacePresenter pres(&mockView);

    EXPECT_CALL(mockView, systemSettings()).Times(0);

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

    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    // finally, user tries to run a reconstruction job
    pres.notify(ITomographyIfacePresenter::RunReconstruct);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
  }

  void test_shutDown() {
    testing::NiceMock<MockTomographyIfaceView> mockView;
    MantidQt::CustomInterfaces::TomographyIfacePresenter pres(&mockView);

    EXPECT_CALL(mockView, saveSettings()).Times(1);
    // No errors, no warnings
    EXPECT_CALL(mockView, userError(testing::_, testing::_)).Times(0);
    EXPECT_CALL(mockView, userWarning(testing::_, testing::_)).Times(0);

    pres.notify(ITomographyIfacePresenter::ShutDown);
    TSM_ASSERT(
        "Mock not used as expected. Some EXPECT_CALL conditions were not "
        "satisfied.",
        testing::Mock::VerifyAndClearExpectations(&mockView))
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
