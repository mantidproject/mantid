#ifndef MANTID_LIVEDATA_LIVEDATAALGORITHMTEST_H_
#define MANTID_LIVEDATA_LIVEDATAALGORITHMTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>
#include "MantidLiveData/LiveDataAlgorithm.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/FacilityHelper.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"

using namespace Mantid;
using namespace Mantid::LiveData;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Kernel;

//------------------------------------------------------------------------------------------------
/** Concrete declaration of LiveDataAlgorithm for testing */
class LiveDataAlgorithmImpl : public LiveDataAlgorithm {
  // Make all the members public so I can test them.
  friend class LiveDataAlgorithmTest;

public:
  virtual const std::string name() const { return "LiveDataAlgorithmImpl"; }
  virtual int version() const { return 1; }
  virtual const std::string category() const { return "Testing"; }
  virtual const std::string summary() const { return "Test summary"; }
  void init() { this->initProps(); }
  void exec() {}
};

class LiveDataAlgorithmTest : public CxxTest::TestSuite {
public:
  void test_initProps() {
    LiveDataAlgorithmImpl alg;
    TS_ASSERT_THROWS_NOTHING(alg.initProps())
  }

  void test_exec() {
    // Name of the output workspace.
    std::string outWSName("LiveDataAlgorithmTest_OutputWS");

    LiveDataAlgorithmImpl alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("StartTime", "2010-09-14T04:20:12.95"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));

    TS_ASSERT(!alg.hasPostProcessing());

    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("PostProcessingAlgorithm", "RenameWorkspace"));
    TS_ASSERT(alg.hasPostProcessing());

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_validateInputs() {
    FacilityHelper::ScopedFacilities loadTESTFacility(
        "IDFs_for_UNIT_TESTING/UnitTestFacilities.xml", "TEST");

    LiveDataAlgorithmImpl alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT(!alg.hasPostProcessing());

    alg.setPropertyValue("Instrument", "FakeEventDataListener");

    TSM_ASSERT("No OutputWorkspace",
               !alg.validateInputs()["OutputWorkspace"].empty());
    alg.setPropertyValue("OutputWorkspace", "out_ws");
    TSM_ASSERT("Is OK now", alg.validateInputs().empty());

    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("PostProcessingScript", "Pause(1)"));
    TS_ASSERT(alg.hasPostProcessing());

    TSM_ASSERT("No AccumulationWorkspace",
               !alg.validateInputs()["AccumulationWorkspace"].empty());
    alg.setPropertyValue("AccumulationWorkspace", "accum_ws");
    TSM_ASSERT("Is OK now", alg.validateInputs().empty());

    alg.setPropertyValue("AccumulationWorkspace", "out_ws");
    TSM_ASSERT("AccumulationWorkspace == OutputWorkspace",
               !alg.validateInputs()["AccumulationWorkspace"].empty());

    alg.setPropertyValue("Instrument", "TESTHISTOLISTENER");
    alg.setPropertyValue("AccumulationMethod", "Add");
    TSM_ASSERT("Shouldn't add histograms",
               !alg.validateInputs()["AccumulationMethod"].empty());
  }

  /** Test creating the processing algorithm.
   * NOTE: RunPythonScript is not available from unit tests, so
   * this is tested in LoadLiveDataTest.py
   */
  void test_makeAlgorithm() {
    FrameworkManager::Instance();
    AlgorithmManager::Instance();
    for (int post = 0; post < 2; post++) {
      // Try both the regular and the Post-Processing algorithm
      std::string prefix = "";
      if (post > 0)
        prefix = "Post";
      std::cout << prefix << "Processing algo" << std::endl;

      LiveDataAlgorithmImpl alg;
      TS_ASSERT_THROWS_NOTHING(alg.initialize())
      TS_ASSERT(alg.isInitialized())

      IAlgorithm_sptr procAlg;
      procAlg = alg.makeAlgorithm(post > 0);
      TSM_ASSERT("NULL algorithm pointer returned if nothing is specified.",
                 !procAlg);

      TS_ASSERT_THROWS_NOTHING(
          alg.setPropertyValue(prefix + "ProcessingAlgorithm", "Rebin"));
      TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(
          prefix + "ProcessingProperties", "Params=0,100,1000"));

      procAlg = alg.makeAlgorithm(post > 0);
      TSM_ASSERT("Non-NULL algorithm pointer", procAlg);
      TS_ASSERT(procAlg->isInitialized());
      TS_ASSERT_EQUALS(procAlg->getPropertyValue("Params"), "0,100,1000");
    }
  }
};

#endif /* MANTID_LIVEDATA_LIVEDATAALGORITHMTEST_H_ */
