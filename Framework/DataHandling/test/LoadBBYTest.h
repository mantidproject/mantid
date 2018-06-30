
#ifndef LOADMCSTASTEST_H_
#define LOADMCSTASTEST_H_

#include <cxxtest/TestSuite.h>
#include <fstream>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/LoadBBY.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include <Poco/Path.h>
#include <Poco/TemporaryFile.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class LoadBBYTest : public CxxTest::TestSuite {
public:
  void test_load_bby_algorithm_init() {
    LoadBBY algToBeTested;

    TS_ASSERT_THROWS_NOTHING(algToBeTested.initialize());
    TS_ASSERT(algToBeTested.isInitialized());
  }

  void test_load_bby_algorithm() {
    LoadBBY algToBeTested;

    if (!algToBeTested.isInitialized())
      algToBeTested.initialize();

    std::string outputSpace = "LoadBBYTest";
    algToBeTested.setPropertyValue("OutputWorkspace", outputSpace);

    // should fail because mandatory parameter has not been set
    TS_ASSERT_THROWS(algToBeTested.execute(), std::runtime_error);

    // should succeed now
    std::string inputFile = "BBY0000014.tar";
    algToBeTested.setPropertyValue("Filename", inputFile);
    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    TS_ASSERT(algToBeTested.isExecuted());

    // get workspace generated
    MatrixWorkspace_sptr output =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputSpace);

    // check number of histograms
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 61440);
    double sum = 0.0;
    for (size_t i = 0; i < output->getNumberHistograms(); i++)
      sum += output->readY(i)[0];
    sum *= 1.0e22;
    TS_ASSERT_DELTA(sum / 1.0E27, 2.0, 0.0001);

    // check that all required log values are there
    auto run = output->run();

    // test start and end time
    TS_ASSERT(
        run.getProperty("start_time")->value().compare("2000-01-01T00:00:00") ==
        0)
    TS_ASSERT(
        run.getProperty("end_time")->value().find("2000-01-01T00:00:00.08") ==
        0)

    // test data properties
    TS_ASSERT_EQUALS(run.getPropertyValueAsType<int>("att_pos"), 1);
    TS_ASSERT_EQUALS(run.getPropertyValueAsType<int>("frame_count"), 4);
    TS_ASSERT_DELTA(run.getPropertyValueAsType<double>("period"), 20000.0,
                    1.0e-5);
    TS_ASSERT_DELTA(run.getPropertyValueAsType<double>("bm_counts"), 0.0800,
                    1.0e-5);

    // test instrument setup
    TS_ASSERT_DELTA(dynamic_cast<TimeSeriesProperty<double> *>(
                        run.getProperty("L1_chopper_value"))
                        ->firstValue(),
                    18.4726, 1.0e-3);
    TS_ASSERT_DELTA(
        dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty("L1"))
            ->firstValue(),
        9.35959, 1.0e-3);

    TS_ASSERT_DELTA(dynamic_cast<TimeSeriesProperty<double> *>(
                        run.getProperty("L2_det_value"))
                        ->firstValue(),
                    33.1562, 1.0e-3);
    TS_ASSERT_DELTA(dynamic_cast<TimeSeriesProperty<double> *>(
                        run.getProperty("L2_curtainl_value"))
                        ->firstValue(),
                    23.2845, 1.0e-3);
    TS_ASSERT_DELTA(dynamic_cast<TimeSeriesProperty<double> *>(
                        run.getProperty("L2_curtainr_value"))
                        ->firstValue(),
                    23.2820, 1.0e-3);
    TS_ASSERT_DELTA(dynamic_cast<TimeSeriesProperty<double> *>(
                        run.getProperty("L2_curtainu_value"))
                        ->firstValue(),
                    24.2862, 1.0e-3);
    TS_ASSERT_DELTA(dynamic_cast<TimeSeriesProperty<double> *>(
                        run.getProperty("L2_curtaind_value"))
                        ->firstValue(),
                    24.2824, 1.0e-3);

    TS_ASSERT_DELTA(dynamic_cast<TimeSeriesProperty<double> *>(
                        run.getProperty("D_curtainl_value"))
                        ->firstValue(),
                    0.3816, 1.0e-4);
    TS_ASSERT_DELTA(dynamic_cast<TimeSeriesProperty<double> *>(
                        run.getProperty("D_curtainr_value"))
                        ->firstValue(),
                    0.4024, 1.0e-4);
    TS_ASSERT_DELTA(dynamic_cast<TimeSeriesProperty<double> *>(
                        run.getProperty("D_curtainu_value"))
                        ->firstValue(),
                    0.3947, 1.0e-4);
    TS_ASSERT_DELTA(dynamic_cast<TimeSeriesProperty<double> *>(
                        run.getProperty("D_curtaind_value"))
                        ->firstValue(),
                    0.3978, 1.0e-4);
    TS_ASSERT_DELTA(dynamic_cast<TimeSeriesProperty<double> *>(
                        run.getProperty("curtain_rotation"))
                        ->firstValue(),
                    10, 1.0e-7);
  }
};

#endif /*LoadBBYTEST_H_*/
