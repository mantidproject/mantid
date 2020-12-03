// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Run.h"
#include "MantidAlgorithms/CorelliPowderCalibrationLoad.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/Logger.h"

#include <stdexcept>

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

///
/// The base class CxxTest doc is available at
/// http://cxxtest.com/guide.html
class CorelliPowderCalibrationLoadTest : public CxxTest::TestSuite {
public:
  void testName() {
    CorelliPowderCalibrationLoad corelliLoader;
    TS_ASSERT_EQUALS(corelliLoader.name(), "CorelliPowderCalibrationLoad");
  }

  void testInit() {
    CorelliPowderCalibrationLoad corelliLoader;
    corelliLoader.initialize();
    TS_ASSERT(corelliLoader.isInitialized());
  }

  void testValidateWSType() {
    // generate a mock workspace with wrong instrument name
    IAlgorithm_sptr lei =
        AlgorithmFactory::Instance().create("LoadEmptyInstrument", 1);
    lei->initialize();
    lei->setPropertyValue("Filename", "NOW4_Definition.xml");
    lei->setPropertyValue("OutputWorkspace", "wrongTypeWs");
    lei->setPropertyValue("MakeEventWorkspace", "1");
    lei->execute();
    EventWorkspace_sptr ws =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>(
            "wrongTypeWs");
    ws->mutableRun().addProperty<std::string>("start_time",
                                              "2020-11-17T12:57:17", "", true);

    // setup alg
    CorelliPowderCalibrationLoad alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", "wrongTypeWs");
    alg.setPropertyValue("DatabaseDir", ".");
    alg.setPropertyValue("OutputWorkspace", "outWS");

    // ensure that an exception is thrown here
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
  }

  void testValidateWSTime() {
    // generate a mock workspace with correct instrument name
    IAlgorithm_sptr lei =
        AlgorithmFactory::Instance().create("LoadEmptyInstrument", 1);
    lei->initialize();
    lei->setPropertyValue("Filename", "CORELLI_Definition.xml");
    lei->setPropertyValue("OutputWorkspace", "correctTypeWs");
    lei->setPropertyValue("MakeEventWorkspace", "1");
    lei->execute();
    EventWorkspace_sptr ws =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>(
            "correctTypeWs");

    // setup alg
    CorelliPowderCalibrationLoad alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", "correctTypeWs");
    alg.setPropertyValue("DatabaseDir", ".");
    alg.setPropertyValue("OutputWorkspace", "outWS");

    // ensure that an exception is thrown here
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
  }

  void testExec() {
    // generate a mock workspace with correct instrument name
    IAlgorithm_sptr lei =
        AlgorithmFactory::Instance().create("LoadEmptyInstrument", 1);
    lei->initialize();
    lei->setPropertyValue("Filename", "CORELLI_Definition.xml");
    lei->setPropertyValue("OutputWorkspace", "correctTypeWs");
    lei->setPropertyValue("MakeEventWorkspace", "1");
    lei->execute();
    EventWorkspace_sptr ws =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>(
            "correctTypeWs");

    // add a starting time to log
    ws->mutableRun().addProperty<std::string>("start_time",
                                              "2020-11-17T12:57:17", "", true);

    // setup alg
    CorelliPowderCalibrationLoad alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", "correctTypeWs");
    alg.setPropertyValue("DatabaseDir", ".");
    alg.setPropertyValue("OutputWorkspace", "outWS");

    // NOTE:
    // 1. When using executedAsChild, the error is indeeded thrown.
    // 2. Due to the error from child alg, the main/parent one will be marked as
    //    "not executed."
    // 3. The strange thing here is that the error from child alg is not
    //    captured by the TS_ASSERT, which is why it seems like nothing is
    //    thrown while in reality it is thrown, just not catched.
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(!alg.isExecuted());
  }
};