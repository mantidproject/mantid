#ifndef MANTID_CUSTOMINTERFACES_TOMOGRAPHYIFACEMODELTEST_H
#define MANTID_CUSTOMINTERFACES_TOMOGRAPHYIFACEMODELTEST_H

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"

#include "MantidQtCustomInterfaces/Tomography/TomographyIfaceModel.h"

#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigDialogBase.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
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
                              model.setExperimentReference("RB0001234"));

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

  void test_cancelFail() {
    TomographyIfaceModel model;

    TSM_ASSERT_EQUALS("Should not be logged in", model.loggedIn(), "");

    const std::vector<std::string> ids = {"none", "inexistent"};
    TSM_ASSERT_THROWS("Exception not thrown as expected - login local",
                      model.doCancelJobs("Local", ids), std::invalid_argument);
  }

  void test_loadFITSFail() {
    TomographyIfaceModel model;

    TSM_ASSERT_THROWS("Exception not thrown as expected - load FITS",
                      model.loadFITSImage("/i_dont_exist.nope"),
                      std::invalid_argument);
  }

  // this currently just transforms the names to lower case
  void test_prepareToolNameForArgs() {

    TestableTomographyIfaceModel model;

    const std::string exp1 = model.prepareToolNameForArgs("TomoPy");
    const std::string exp2 = model.prepareToolNameForArgs("Astra");
    const std::string exp3 = model.prepareToolNameForArgs("Savu");
    const std::string exp4 = model.prepareToolNameForArgs("Custom Command");

    TS_ASSERT_EQUALS(exp1, "tomopy");
    TS_ASSERT_EQUALS(exp2, "astra");
    TS_ASSERT_EQUALS(exp3, "savu");
    TS_ASSERT_EQUALS(exp4, "custom command");
    // although custom command never reaches that function
  }

  void test_makeRemoteRunnableWithOptionsCustom() {
    std::string inputRunnable = "/scriptPath/";
    // the custom one just processes a single member
    std::vector<std::string> inputArgsVector{
        "--some params --some other params"};

    TestableTomographyIfaceModel model;

    std::string inputArgsString =
        model.constructSingleStringFromVector(inputArgsVector);

    std::shared_ptr<TomoRecToolConfig> d = std::shared_ptr<TomoRecToolConfig>(
        new ToolConfigCustom(inputRunnable, inputArgsString));

    model.usingTool(TestableTomographyIfaceModel::g_customCmdTool);
    model.setCurrentToolMethod("gridrec");

    model.setCurrentToolSettings(d);

    const bool local = false;

    std::string actualRunnable;
    std::string allOpts;
    std::vector<std::string> actualArgsVector;
    model.prepareSubmissionArguments(local, actualRunnable, actualArgsVector,
                                     allOpts);

    std::string expectedRunnable = "/scriptPath/";
    // the space at the end is necessary, because of how
    // constructSingleStringFromVector works
    std::vector<std::string> expectedArgsVector{
        "--some params --some other params "};
    doTestExpectedRunnableAndArguments(expectedRunnable, actualRunnable,
                                       expectedArgsVector, actualArgsVector);
  }

  void test_makeLocalRunnableWithOptionsCustom() {
    std::string inputRunnable = "python /scriptPath/";
    // the custom one just processes a single member
    std::vector<std::string> inputArgsVector{
        "--some params --some other params"};

    TestableTomographyIfaceModel model;

    std::string inputArgsString =
        model.constructSingleStringFromVector(inputArgsVector);

    std::shared_ptr<TomoRecToolConfig> d = std::shared_ptr<TomoRecToolConfig>(
        new ToolConfigCustom(inputRunnable, inputArgsString));

    model.usingTool(TestableTomographyIfaceModel::g_customCmdTool);
    model.setCurrentToolMethod("gridrec");

    model.setCurrentToolSettings(d);

    const bool local = true;

    std::string actualRunnable;
    std::string allOpts;

    std::vector<std::string> actualArgsVector;
    model.prepareSubmissionArguments(local, actualRunnable, actualArgsVector,
                                     allOpts);

    std::string expectedRunnable = "python";
    // the space at the end is necessary, because of how
    // constructSingleStringFromVector works
    std::vector<std::string> expectedArgsVector{
        "/scriptPath/", "--some params --some other params "};
    doTestExpectedRunnableAndArguments(expectedRunnable, actualRunnable,
                                       expectedArgsVector, actualArgsVector);
  }

  void test_makeRemoteRunnableWithOptions() {
    std::string expectedRunnable =
        "/work/imat/phase_commissioning/scripts/Imaging/"
        "IMAT/tomo_reconstruct.py";
    TomoPathsConfig pathConfig;

    const std::string pathOut = "/work/imat";
    static size_t reconIdx = 1;
    const std::string localOutNameAppendix = std::string("/processed/") +
                                             "reconstruction_" +
                                             std::to_string(reconIdx);

    std::shared_ptr<TomoRecToolConfig> d = std::shared_ptr<TomoRecToolConfig>(
        new ToolConfigTomoPy(expectedRunnable, pathOut + localOutNameAppendix,
                             pathConfig.pathDarks(), pathConfig.pathOpenBeam(),
                             pathConfig.pathSamples()));

    TestableTomographyIfaceModel model;

    model.usingTool(TestableTomographyIfaceModel::g_TomoPyTool);
    model.setCurrentToolMethod("gridrec");

    model.setCurrentToolSettings(d);

    const bool local = false;

    std::string allOpts;
    std::string actualRunnable;
    std::vector<std::string> actualArgsVector;
    model.prepareSubmissionArguments(local, actualRunnable, actualArgsVector,
                                     allOpts);

    std::vector<std::string> expectedArgsVector{
        "--tool=tomopy", "--algorithm=gridrec", "--num-iter=5",
        "--input-path=" + pathConfig.pathSamples(),
        "--input-path-flat=" + pathConfig.pathOpenBeam(),
        "--input-path-dark=" + pathConfig.pathDarks(),
        "--output=\"/work/imat/phase_commissioning/processed/"
        "reconstruction_TomoPy_gridrec_2016October20_113701_413275000",
        "--median-filter-size=3", "--cor=0.000000", "--rotation=0",
        "--max-angle=360.000000", "--circular-mask=0.940000",
        "--out-img-format=png"};
    doTestExpectedRunnableAndArguments(expectedRunnable, actualRunnable,
                                       expectedArgsVector, actualArgsVector);
  }

  void test_makeLocalRunnableWithOptions() {
    std::string inputRunnable = "python "
                                "/work/imat/phase_commissioning/scripts/"
                                "Imaging/IMAT/tomo_reconstruct.py";

    TomoPathsConfig pathConfig;
    const std::string pathOut = "~/imat/RB000XXX";
    static size_t reconIdx = 1;
    const std::string localOutNameAppendix = std::string("/processed/") +
                                             "reconstruction_" +
                                             std::to_string(reconIdx);

    std::shared_ptr<TomoRecToolConfig> d = std::shared_ptr<TomoRecToolConfig>(
        new ToolConfigTomoPy(inputRunnable, pathOut + localOutNameAppendix,
                             pathConfig.pathDarks(), pathConfig.pathOpenBeam(),
                             pathConfig.pathSamples()));

    TestableTomographyIfaceModel model;

    model.usingTool(TestableTomographyIfaceModel::g_TomoPyTool);
    model.setCurrentToolMethod("gridrec");

    model.setCurrentToolSettings(d);

    const bool local = true;

    // should be just the externalInterpretor path
    std::string actualRunnable;
    std::string allOpts;
    std::vector<std::string> actualArgsVector;
    model.prepareSubmissionArguments(local, actualRunnable, actualArgsVector,
                                     allOpts);

    std::string expectedRunnable = "python";
    std::vector<std::string> expectedArgsVector{
        "/work/imat/phase_commissioning/scripts/Imaging/IMAT/"
        "tomo_reconstruct.py",
        "--tool=tomopy", "--algorithm=gridrec", "--num-iter=5",
        "--input-path=/work/imat/phase_commissioning/data",
        "--input-path-flat=/work/imat/phase_commissioning/flat",
        "--input-path-dark=/work/imat/phase_commissioning/dark",
        "--output=/work/imat/phase_commissioning/processed/"
        "reconstruction_TomoPy_gridrec_2016October20_113701_413275000",
        "--median-filter-size=3", "--cor=0.000000", "--rotation=0",
        "--max-angle=360.000000", "--circular-mask=0.940000",
        "--out-img-format=png"};

    doTestExpectedRunnableAndArguments(expectedRunnable, actualRunnable,
                                       expectedArgsVector, actualArgsVector);
  }

private:
  // inner class to access the model's protected functions
  class TestableTomographyIfaceModel : public TomographyIfaceModel {
    friend class TomographyIfaceModelTest;
    TestableTomographyIfaceModel() : TomographyIfaceModel() {}
  };

  void doTestExpectedRunnableAndArguments(
      const std::string &expectedRunnable, const std::string &actualRunnable,
      const std::vector<std::string> &expectedArguments,
      const std::vector<std::string> &actualArguments) {
    TSM_ASSERT_EQUALS("Local interpreter executable not properly separated",
                      actualRunnable, expectedRunnable);
    TSM_ASSERT_EQUALS("Invalid argument size", expectedArguments.size(),
                      actualArguments.size());

    for (size_t i = 0; i < expectedArguments.size(); ++i) {
      // this is the --output one that varies depending on time, so just skip
      if (expectedArguments[i].substr(0, 8) == "--output") {
        continue;
      }
      TS_ASSERT_EQUALS(expectedArguments[i], actualArguments[i]);
    }
  }
};

#endif // MANTID_CUSTOMINTERFACES_TOMOGRAPHYIFACEMODELTEST_H
