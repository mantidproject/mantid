// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidFrameworkTestHelpers/FacilityHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Timer.h"
#include "MantidLiveData/LiveDataAlgorithm.h"
#include <algorithm>
#include <cxxtest/TestSuite.h>

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
  const std::string name() const override { return "LiveDataAlgorithmImpl"; }
  int version() const override { return 1; }
  const std::string category() const override { return "Testing"; }
  const std::string summary() const override { return "Test summary"; }
  void init() override { this->initProps(); }
  void exec() override {}
};

class LiveDataAlgorithmTest : public CxxTest::TestSuite {
private:
  static bool contains(const std::vector<std::string> &values, const std::string &item) {
    return std::find(values.cbegin(), values.cend(), item) != values.cend();
  }

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
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("StartTime", "2010-09-14T04:20:12.95"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));

    TS_ASSERT(!alg.hasPostProcessing());

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PostProcessingAlgorithm", "RenameWorkspace"));
    TS_ASSERT(alg.hasPostProcessing());

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_validateInputs() {
    FacilityHelper::ScopedFacilities loadTESTFacility("unit_testing/UnitTestFacilities.xml", "TEST");

    LiveDataAlgorithmImpl alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT(!alg.hasPostProcessing());

    alg.setPropertyValue("Instrument", "FakeEventDataListener");

    TSM_ASSERT("No OutputWorkspace", !alg.validateInputs()["OutputWorkspace"].empty());
    alg.setPropertyValue("OutputWorkspace", "out_ws");
    TSM_ASSERT("Is OK now", alg.validateInputs().empty());

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PostProcessingScript", "Pause(1)"));
    TS_ASSERT(alg.hasPostProcessing());

    TSM_ASSERT("No AccumulationWorkspace", !alg.validateInputs()["AccumulationWorkspace"].empty());
    alg.setPropertyValue("AccumulationWorkspace", "accum_ws");
    TSM_ASSERT("Is OK now", alg.validateInputs().empty());

    alg.setPropertyValue("AccumulationWorkspace", "out_ws");
    TSM_ASSERT("AccumulationWorkspace == OutputWorkspace", !alg.validateInputs()["AccumulationWorkspace"].empty());

    alg.setPropertyValue("Instrument", "TESTHISTOLISTENER");
    alg.setPropertyValue("AccumulationMethod", "Add");
    TSM_ASSERT("Shouldn't add histograms", !alg.validateInputs()["AccumulationMethod"].empty());
  }

  /**
   * An instrument name is only meaningful with respect to a facility, so `Instrument` carries no list
   * validator: the valid set depends on `Facility`, whose value is not available until after
   * initialization.  Validation happens in `validateInputs` instead.
   */
  void test_instrument_has_no_list_validator() {
    FacilityHelper::ScopedFacilities loadTESTFacility("unit_testing/UnitTestFacilitiesMultiple.xml", "TESTDEFAULT");

    LiveDataAlgorithmImpl alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())

    const Mantid::Kernel::Property *instrumentProp = alg.getProperty("Instrument");
    TSM_ASSERT("no frozen allowed-values list", instrumentProp->allowedValues().empty());
  }

  /** Resolution is confined to one facility: the default one when 'Facility' is not given. */
  void test_liveListenerInstruments_uses_the_default_facility() {
    FacilityHelper::ScopedFacilities loadTESTFacility("unit_testing/UnitTestFacilitiesMultiple.xml", "TESTDEFAULT");

    LiveDataAlgorithmImpl alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())

    const auto instruments = alg.liveListenerInstruments();
    TSM_ASSERT("default facility's listener instrument", contains(instruments, "DEFAULTLISTENER"));
    TSM_ASSERT("other facility is not searched", !contains(instruments, "OTHERLISTENER"));
    TSM_ASSERT("instrument without live data", !contains(instruments, "NOLIVEDATA"));
  }

  /** Naming 'Facility' moves resolution to that facility. */
  void test_liveListenerInstruments_uses_the_named_facility() {
    FacilityHelper::ScopedFacilities loadTESTFacility("unit_testing/UnitTestFacilitiesMultiple.xml", "TESTDEFAULT");

    LiveDataAlgorithmImpl alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    alg.setPropertyValue("Facility", "TESTOTHER");

    const auto instruments = alg.liveListenerInstruments();
    TSM_ASSERT("named facility's listener instrument", contains(instruments, "OTHERLISTENER"));
    TSM_ASSERT("default facility is no longer searched", !contains(instruments, "DEFAULTLISTENER"));
  }

  /** The point of the facility argument: an instrument outside the default facility is selectable. */
  void test_instrument_in_named_facility_is_accepted() {
    FacilityHelper::ScopedFacilities loadTESTFacility("unit_testing/UnitTestFacilitiesMultiple.xml", "TESTDEFAULT");

    LiveDataAlgorithmImpl alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    alg.setPropertyValue("Instrument", "OTHERLISTENER");
    alg.setPropertyValue("Facility", "TESTOTHER");
    alg.setPropertyValue("OutputWorkspace", "out_ws");

    auto errors = alg.validateInputs();
    TSM_ASSERT("instrument in the named facility is accepted", errors["Instrument"].empty());
    TSM_ASSERT("named facility is accepted", errors["Facility"].empty());
  }

  void test_Facility_defaults_to_empty() {
    FacilityHelper::ScopedFacilities loadTESTFacility("unit_testing/UnitTestFacilitiesMultiple.xml", "TESTDEFAULT");

    LiveDataAlgorithmImpl alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_EQUALS(alg.getPropertyValue("Facility"), "")
  }

  void test_validateInputs_unknown_facility() {
    FacilityHelper::ScopedFacilities loadTESTFacility("unit_testing/UnitTestFacilitiesMultiple.xml", "TESTDEFAULT");

    LiveDataAlgorithmImpl alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    alg.setPropertyValue("Instrument", "DEFAULTLISTENER");
    alg.setPropertyValue("Facility", "NOT_A_FACILITY");
    alg.setPropertyValue("OutputWorkspace", "out_ws");

    TSM_ASSERT("unknown facility is reported", !alg.validateInputs()["Facility"].empty());
  }

  /** An instrument outside the resolved facility is rejected, rather than found elsewhere. */
  void test_validateInputs_instrument_outside_the_resolved_facility() {
    FacilityHelper::ScopedFacilities loadTESTFacility("unit_testing/UnitTestFacilitiesMultiple.xml", "TESTDEFAULT");

    LiveDataAlgorithmImpl alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    // 'OTHERLISTENER' exists, but in TESTOTHER, and no facility is named here.
    alg.setPropertyValue("Instrument", "OTHERLISTENER");
    alg.setPropertyValue("OutputWorkspace", "out_ws");

    TSM_ASSERT("instrument outside the default facility is reported", !alg.validateInputs()["Instrument"].empty());
  }

  void test_validateInputs_instrument_without_live_listener() {
    FacilityHelper::ScopedFacilities loadTESTFacility("unit_testing/UnitTestFacilitiesMultiple.xml", "TESTDEFAULT");

    LiveDataAlgorithmImpl alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    alg.setPropertyValue("Instrument", "NOLIVEDATA");
    alg.setPropertyValue("OutputWorkspace", "out_ws");

    TSM_ASSERT("instrument with no live listener is reported", !alg.validateInputs()["Instrument"].empty());
  }

  /** An unset default facility must be reported, not silently accepted as a blank facility. */
  void test_validateInputs_no_default_facility() {
    FacilityHelper::ScopedFacilities loadTESTFacility("unit_testing/UnitTestFacilitiesMultiple.xml", "TESTDEFAULT");

    auto &config = Mantid::Kernel::ConfigService::Instance();
    const std::string savedFacility = config.getString("default.facility");
    config.setString("default.facility", "");

    LiveDataAlgorithmImpl alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    alg.setPropertyValue("Instrument", "DEFAULTLISTENER");
    alg.setPropertyValue("OutputWorkspace", "out_ws");

    const auto errors = alg.validateInputs();
    config.setString("default.facility", savedFacility);

    TSM_ASSERT("unset default facility is reported", !errors.at("Facility").empty());
  }

  void test_validateInputs_missing_instrument() {
    FacilityHelper::ScopedFacilities loadTESTFacility("unit_testing/UnitTestFacilitiesMultiple.xml", "TESTDEFAULT");

    LiveDataAlgorithmImpl alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    alg.setPropertyValue("OutputWorkspace", "out_ws");

    TSM_ASSERT("missing instrument is reported", !alg.validateInputs()["Instrument"].empty());
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
      std::cout << prefix << "Processing algo\n";

      LiveDataAlgorithmImpl alg;
      TS_ASSERT_THROWS_NOTHING(alg.initialize())
      TS_ASSERT(alg.isInitialized())

      auto procAlg = alg.makeAlgorithm(post > 0);
      TSM_ASSERT("NULL algorithm pointer returned if nothing is specified.", !procAlg);

      TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(prefix + "ProcessingAlgorithm", "Rebin"));
      TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(prefix + "ProcessingProperties", "Params=0,100,1000"));

      procAlg = alg.makeAlgorithm(post > 0);
      TSM_ASSERT("Non-NULL algorithm pointer", procAlg);
      TS_ASSERT(procAlg->isInitialized());
      TS_ASSERT_EQUALS(procAlg->getPropertyValue("Params"), "0,100,1000");
    }
  }
};
