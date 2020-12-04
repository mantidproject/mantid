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
#include "MantidTestHelpers/ScopedFileHelper.h"

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

    // generate the calibration table file
    const std::string califilename = "corelli_instrument_20201117.csv";
    ScopedFileHelper::ScopedFile f = generateCalibrationTableFile(califilename);

    // locate the temp folder location
    const std::string dbdir =
        Mantid::Kernel::ConfigService::Instance().getTempDir().c_str();

    // setup alg
    CorelliPowderCalibrationLoad alg;
    alg.initialize();
    // alg.setPropertyValue("InputWorkspace", "correctTypeWs");
    alg.setProperty("InputWorkspace", ws);
    alg.setPropertyValue("DatabaseDir", dbdir);
    alg.setPropertyValue("OutputWorkspace", "outWS");
    alg.execute();

    TS_ASSERT(alg.isExecuted());
  }

  /**
   * @brief generate a temperory calibration table for loading test
   *
   * @param califilename
   * @return ScopedFileHelper::ScopedFile
   */
  ScopedFileHelper::ScopedFile
  generateCalibrationTableFile(const std::string &califilename) {
    std::ostringstream os;

    os << "# Component , Xposition , Yposition , Zposition , XdirectionCosine "
          ", YdirectionCosine , ZdirectionCosine , RotationAngle\n";
    os << "# str , double , double , double , double , double , double , "
          "double \n";
    os << "moderator,0,0,-19.9997,0,0,0,0\n";
    os << "sample-position,0,0,0,0,0,0,0\n";
    os << "bank7/"
          "sixteenpack,2.25637,-0.814864,-0.883485,-0.0244456,-0.99953,-0."
          "0184843,69.4926\n";
    os << "bank8/"
          "sixteenpack,2.31072,-0.794864,-0.667308,-0.0191907,-0.999553,-0."
          "0229249,73.6935\n";

    return ScopedFileHelper::ScopedFile(os.str(), califilename);
  }
};