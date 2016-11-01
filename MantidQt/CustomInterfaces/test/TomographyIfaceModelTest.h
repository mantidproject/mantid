#ifndef MANTID_CUSTOMINTERFACES_TOMOGRAPHYIFACEMODELTEST_H
#define MANTID_CUSTOMINTERFACES_TOMOGRAPHYIFACEMODELTEST_H

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidQtCustomInterfaces/Tomography/TomographyIfaceModel.h"

#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces;

class TomographyIfaceModelTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TomographyIfaceModelTest *createSuite() {
    return new TomographyIfaceModelTest();
  }

  static void destroySuite(TomographyIfaceModelTest *suite) { delete suite; }

  TomographyIfaceModelTest() {
    Mantid::API::FrameworkManager::Instance(); // make sure framework is
                                               // initialized
  }

  void test_noInit() {
    auto model = Mantid::Kernel::make_unique<TomographyIfaceModel>();

    TSM_ASSERT_EQUALS("Unexpected number of compute resources",
                      model->computeResources().size(), 2);

    TSM_ASSERT_EQUALS("Unexpected number of compute resources (status)",
                      model->computeResourcesStatus().size(), 0);

    TSM_ASSERT_EQUALS("Unexpected default tool", model->usingTool(), "TomoPy");

    TSM_ASSERT_EQUALS("Unexpected name for local machine",
                      model->localComputeResource(), "Local");
  }

  void test_setupComputeResource() {
    auto model = Mantid::Kernel::make_unique<TomographyIfaceModel>();

    model->setupComputeResource();

    TSM_ASSERT_EQUALS("Unexpected number of compute resources",
                      model->computeResources().size(), 2);

    auto status = model->computeResourcesStatus();
    TSM_ASSERT_EQUALS("Unexpected number of compute resources (status)",
                      status.size(), 2);
    TSM_ASSERT("Unexpected status for first compute resource ", status[0]);
    TSM_ASSERT("Unexpected status for second compute resource ", status[1]);

    TSM_ASSERT_THROWS_NOTHING("Problem in cleanup", model->cleanup());
  }

  void test_setupToolsDefaults() {
    TomographyIfaceModel model;

    model.setupRunTool("Local");

    TSM_ASSERT_EQUALS("Unexpected number of reconstruction tools",
                      model.reconTools().size(), 5);

    const auto status = model.reconToolsStatus();

    const std::vector<bool> expected = {true, true, false, false, true};
    TSM_ASSERT_EQUALS("Unexpected number of reconstruction tools (status)",
                      status.size(), 5);
    for (size_t idx = 0; idx < status.size(); ++idx) {
      TSM_ASSERT("Unexpected status for tool number " + std::to_string(idx + 1),
                 status[idx] == expected[idx]);
    }
  }

  void test_facilities() {
    TomographyIfaceModel model;

    // save original facility
    const Mantid::Kernel::FacilityInfo &prevFac =
        Mantid::Kernel::ConfigService::Instance().getFacility();

    Mantid::Kernel::ConfigService::Instance().setFacility("ISIS");
    auto isSupported = model.facilitySupported();
    TSM_ASSERT("This facility should be supported", isSupported);

    const std::vector<std::string> otherFacilities = {"SNS", "HFIR", "ILL",
                                                      "ANSTO", "TEST_LIVE"};
    for (const auto &facility : otherFacilities) {
      Mantid::Kernel::ConfigService::Instance().setFacility(facility);
      TSM_ASSERT("This facility should not be supported",
                 !model.facilitySupported());
    }

    // restore facility
    Mantid::Kernel::ConfigService::Instance().setFacility(prevFac.name());
  }

  void test_jobsStatus() {
    TomographyIfaceModel model;

    model.setupComputeResource();
    model.setupRunTool("Local");

    TSM_ASSERT_THROWS_NOTHING("Problem with experiment number",
                              model.updateExperimentReference("RB0001234"));

    auto sts = model.jobsStatus();
    TSM_ASSERT_EQUALS("Unexpected number of jobs", sts.size(), 0);

    auto localSts = model.jobsStatusLocal();
    TSM_ASSERT_EQUALS("Unexpected number of jobs (local)", localSts.size(), 0);

    model.refreshLocalJobsInfo();
    localSts = model.jobsStatusLocal();
    TSM_ASSERT_EQUALS("Unexpected number of jobs (local), after refreshing",
                      localSts.size(), 0);

    model.doRefreshJobsInfo("phony");
    sts = model.jobsStatus();
    TSM_ASSERT_EQUALS("Unexpected number of jobs, after refreshing", sts.size(),
                      0);
  }

  void test_pingFail() {
    TomographyIfaceModel model;

    TSM_ASSERT_THROWS("Exception not thrown as expected - ping local",
                      model.doPing("Local"), std::invalid_argument);
  }

  void test_loginFail() {
    TomographyIfaceModel model;

    TSM_ASSERT_EQUALS("Should not be logged in", model.loggedIn(), "");

    TSM_ASSERT_THROWS("Exception not thrown as expected - login local",
                      model.doLogin("Local", "foo_user", "password"),
                      std::invalid_argument);
  }

  void test_logoutFail() {
    TomographyIfaceModel model;

    TSM_ASSERT_THROWS("Exception not thrown as expected - logout local",
                      model.doLogout("Local", "foo_user"),
                      std::invalid_argument);
  }

  void test_queryFail() {
    TomographyIfaceModel model;

    std::vector<std::string> ids;
    std::vector<std::string> names;
    std::vector<std::string> status;
    std::vector<std::string> cmds;
    TSM_ASSERT_THROWS("Exception not thrown as expected - ping local",
                      model.doQueryJobStatus("Local", ids, names, status, cmds),
                      std::invalid_argument);
    TS_ASSERT_EQUALS(ids.size(), 0);
    TS_ASSERT_EQUALS(names.size(), 0);
    TS_ASSERT_EQUALS(status.size(), 0);
    TS_ASSERT_EQUALS(cmds.size(), 0);
  }

  void test_submitFailEmptyDefinition() {
    TomographyIfaceModel model;

    TSM_ASSERT_THROWS("Exception not thrown as expected - submit local",
                      model.doSubmitReconstructionJob("Local"),
                      std::invalid_argument);
  }

  void test_submitFailWrongResource() {
    TomographyIfaceModel model;

    model.setupComputeResource();
    model.setupRunTool("Local");
    model.usingTool("TomoPy");
    TSM_ASSERT_THROWS("Exception not thrown as expected - submit local",
                      model.doSubmitReconstructionJob("Local"),
                      std::invalid_argument);
  }

  void test_cancelFail() {
    TomographyIfaceModel model;

    TSM_ASSERT_EQUALS("Should not be logged in", model.loggedIn(), "");

    const std::vector<std::string> ids = {"none", "inexistent"};
    TSM_ASSERT_THROWS("Exception not thrown as expected - login local",
                      model.doCancelJobs("Local", ids), std::invalid_argument);
  }

  void test_runCustomCommandLocally() {
    TomographyIfaceModel model;

    model.setupComputeResource();
    model.setupRunTool("Local");
    model.usingTool("Custom command");

    TomoReconToolsUserSettings toolsSettings;
    toolsSettings.custom = ToolConfigCustom("fail", "some params");
    model.updateReconToolsSettings(toolsSettings);
    model.doRunReconstructionJobLocal();
  }

  void test_setupToolsTomoPy() { dryRunToolLocal("TomoPy", "gridrec"); }

  void test_setupToolsAstra() { dryRunToolLocal("Astra", "FBP3D_CUDA"); }

  void test_loadFITSFail() {
    TomographyIfaceModel model;

    TSM_ASSERT_THROWS("Exception not thrown as expected - load FITS",
                      model.loadFITSImage("/i_dont_exist.nope"),
                      std::invalid_argument);
  }

private:
  void dryRunToolLocal(const std::string &tool, const std::string &method) {
    TomographyIfaceModel model;
    model.setupComputeResource();
    model.setupRunTool(model.localComputeResource());
    model.usingTool(tool);
    model.updateTomopyMethod(method);

    TSM_ASSERT_EQUALS("Unexpected number of reconstruction tools",
                      model.reconTools().size(), 5);

    auto localSts = model.jobsStatusLocal();
    TSM_ASSERT_EQUALS("Unexpected number of jobs (local)", localSts.size(), 0);

    // default/empty paths, to make sure nothing will be found
    TomoPathsConfig paths;
    model.updateTomoPathsConfig(paths);

    // paths that don't make sense, so nothing gets executed even if you have a
    // local installation of tomopy available
    TomoSystemSettings settings;
    settings.m_local.m_basePathTomoData = "/never_find_anything/";
    settings.m_local.m_reconScriptsPath = "/dont_find_the_scripts/";
    model.updateSystemSettings(settings);

    TomoReconToolsUserSettings toolsSettings;
    toolsSettings.tomoPy =
        ToolConfigTomoPy("fail", "/out/", "/dark/", "/flat/", "/sample/");
    model.updateReconToolsSettings(toolsSettings);
    model.doRunReconstructionJobLocal();

    model.refreshLocalJobsInfo();
    localSts = model.jobsStatusLocal();
    TSM_ASSERT_EQUALS("Unexpected number of jobs (local), after refreshing",
                      localSts.size(), 1);
  }
};

#endif // MANTID_CUSTOMINTERFACES_TOMOGRAPHYIFACEMODELTEST_H
