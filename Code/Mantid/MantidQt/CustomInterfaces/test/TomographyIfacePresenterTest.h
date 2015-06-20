#ifndef MANTID_CUSTOMINTERFACES_TOMOGRAPHYIFACEPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_TOMOGRAPHYIFACEPRESENTERTEST_H

#include "MantidAPI/FrameworkManager.h"
#include "MantidQtCustomInterfaces/Tomography/TomographyIfacePresenter.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

class MockTomographyIfaceView
    : public MantidQt::CustomInterfaces::ITomographyIfaceView {
public:
  void userWarning(const std::string &warn, const std::string &description) {}
  void userError(const std::string &err, const std::string &description) {}

  std::vector<std::string> logMsgs() const {}

  void setComputeResources(const std::vector<std::string> &resources,
                           const std::vector<bool> &enabled) {}
  void setReconstructionTools(const std::vector<std::string> &tools,
                              const std::vector<bool> &enabled) {}
  void saveSettings() const { }

  std::string getUsername() const {}
  std::string getPassword() const {}

  std::vector<std::string> processingJobsIDs() const { }
  std::string currentComputeResource() const {}

  std::string currentReconTool() const {}

  /// updates buttons and banners related to the current login status
  void updateLoginControls(bool loggedIn) {}

  void enableLoggedActions(bool enable) {}

  void enableConfigTool(bool on) {}

  void enableRunReconstruct(bool on) {}

  std::string showImagePath() {}

  void showImage(const Mantid::API::MatrixWorkspace_sptr &wsg) {}
  void showImage(const std::string &path) {}

  void showToolConfig(const std::string &name) {}

  void updateJobsInfoDisplay(const std::vector<
      Mantid::API::IRemoteJobManager::RemoteJobInfo> &status) {}
};

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
    m_view = new testing::NiceMock<MockTomographyIfaceView>();
    // m_model = new testing::NiceMock<MockALCBaselineModellingModel>();
    m_presenter = new MantidQt::CustomInterfaces::TomographyIfacePresenter(
        m_view /*, m_model */);
    // m_presenter->initialize();
  }

  void tearDown() {
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(m_view));
    // TS_ASSERT(testing::Mock::VerifyAndClearExpectations(m_model));

    delete m_presenter;
    // delete m_model;
    delete m_view;
  }

  void test_showImg() { TS_ASSERT(true); }

  void test_refreshRuns() { TS_ASSERT(true); }

private:
  // boost::shared_ptr
  MockTomographyIfaceView *m_view;
  // MockTomographyIfaceModel *m_model;
  MantidQt::CustomInterfaces::TomographyIfacePresenter *m_presenter;
};

#endif // MANTID_CUSTOMINTERFACES_TOMOGRAPHYIFACEPRESENTERTEST_H
