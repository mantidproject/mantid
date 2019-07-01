#ifndef LOADPLNTEST_H_
#define LOADPLNTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Run.h"
#include "MantidDataHandling/LoadPLN.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class LoadPLNTest : public CxxTest::TestSuite {
public:
  void test_load_pln_algorithm_init() {
    LoadPLN algToBeTested;

    TS_ASSERT_THROWS_NOTHING(algToBeTested.initialize());
    TS_ASSERT(algToBeTested.isInitialized());
  }

  void test_load_pln_algorithm() {
    LoadPLN algToBeTested;

    if (!algToBeTested.isInitialized())
      algToBeTested.initialize();

    std::string outputSpace = "LoadPLNTest";
    algToBeTested.setPropertyValue("OutputWorkspace", outputSpace);

    // should fail because mandatory parameter has not been set
    TS_ASSERT_THROWS(algToBeTested.execute(), const std::runtime_error &);

    // missing event file - should fail execution
    std::string inputFile = "PLN0044464.hdf";
    algToBeTested.setPropertyValue("Filename", inputFile);
    algToBeTested.setPropertyValue("BinaryEventPath", "./");
    TS_ASSERT(!algToBeTested.execute());

    // should succeed now
    algToBeTested.setPropertyValue("Filename", inputFile);
    algToBeTested.setPropertyValue("BinaryEventPath", "./PLN0044464.bin");
    algToBeTested.setPropertyValue("CalibrateTOFBias", "1");
    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    TS_ASSERT(algToBeTested.isExecuted());

    // get workspace generated
    MatrixWorkspace_sptr output =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputSpace);

    // check number of histograms
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 12808);
    double sum = 0.0;
    for (size_t i = 0; i < output->getNumberHistograms(); i++)
      sum += output->y(i)[0];
    TS_ASSERT_EQUALS(sum, 163118);

    // check that all required log values are there
    auto run = output->run();

    // test start and end time
    TS_ASSERT(
        run.getProperty("start_time")->value().compare("2018-11-12T10:45:06") ==
        0)
    TS_ASSERT(
        run.getProperty("end_time")->value().find("2018-11-12T11:45:06.6") == 0)

    // test some data properties
    auto logpm = [&run](std::string tag) {
      return dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty(tag))
          ->firstValue();
    };
    TS_ASSERT_DELTA(logpm("GatePeriod"), 5000.8, 1.0);
    TS_ASSERT_DELTA(logpm("DetectorTankAngle"), 57.513, 1.0e-3);
    TS_ASSERT_DELTA(logpm("TOFCorrection"), -256.456, 1.0e-3);
    TS_ASSERT_DELTA(logpm("Wavelength"), 4.6866, 1.0e-4);
    TS_ASSERT_DELTA(logpm("SampleRotation"), 13.001, 1.0e-3);
  }
};

#endif /*LOADPLNTEST_H_*/
