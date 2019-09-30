
#ifndef LOADEMUAUTEST_H_
#define LOADEMUAUTEST_H_

#include <cxxtest/TestSuite.h>
#include <fstream>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/LoadEMU.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include <Poco/Path.h>
#include <Poco/TemporaryFile.h>

#ifdef _MSC_VER
// Disable warning on 'no suitable definition ..' as the extern
// does not clear the warning. No issue linking.
#pragma warning(disable : 4661)
#endif

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class LoadEMUauTest : public CxxTest::TestSuite {
public:
  void test_load_emu_algorithm_init() {
    LoadEMUTar algToBeTested;

    TS_ASSERT_THROWS_NOTHING(algToBeTested.initialize());
    TS_ASSERT(algToBeTested.isInitialized());
  }

  void test_load_emu_algorithm() {
    LoadEMUTar algToBeTested;

    if (!algToBeTested.isInitialized())
      algToBeTested.initialize();

    std::string outputSpace = "LoadEMUauTest";
    algToBeTested.setPropertyValue("OutputWorkspace", outputSpace);

    // should fail because mandatory parameter has not been set
    TS_ASSERT_THROWS(algToBeTested.execute(), const std::runtime_error &);

    // should succeed now
    std::string inputFile = "EMU0006330.tar";
    algToBeTested.setPropertyValue("Filename", inputFile);
    algToBeTested.setPropertyValue("SelectDetectorTubes", "16-50");
    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    TS_ASSERT(algToBeTested.isExecuted());

    // get workspace generated
    MatrixWorkspace_sptr output =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputSpace);

    // check number of histograms
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 6528);
    double sum = 0.0;
    for (size_t i = 0; i < output->getNumberHistograms(); i++)
      sum += output->readY(i)[0];
    TS_ASSERT_EQUALS(sum, 55126);

    // check that all required log values are there
    auto run = output->run();

    // test start and end time
    TS_ASSERT(
        run.getProperty("start_time")->value().compare("2018-07-26T10:13:12") ==
        0)
    TS_ASSERT(
        run.getProperty("end_time")->value().find("2018-07-26T10:17:12.6") == 0)

    // test some data properties
    auto logpm = [&run](std::string tag) {
      return dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty(tag))
          ->firstValue();
    };
    TS_ASSERT_DELTA(logpm("DopplerFrequency"), 9.974, 1.0e-3);
    TS_ASSERT_DELTA(logpm("DopplerAmplitude"), 0.075, 1.0e-4);

    // test some instrument parameters
    auto instr = output->getInstrument();
    auto iparam = [&instr](std::string tag) {
      return instr->getNumberParameter(tag)[0];
    };
    TS_ASSERT_DELTA(iparam("AnalysedV2"), 630.866, 1.0e-3);
    TS_ASSERT_DELTA(iparam("SampleAnalyser"), 1.8, 1.0e-3);
  }
};

#endif /*LOADEMUAUTEST_H_*/
