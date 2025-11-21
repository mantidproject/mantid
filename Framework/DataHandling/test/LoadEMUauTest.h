// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include <cxxtest/TestSuite.h>
#include <fstream>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/InstrumentFileFinder.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/LoadEMU.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include <Poco/TemporaryFile.h>
#include <filesystem>

#ifdef _MSC_VER
// Disable warning on 'no suitable definition ..' as the extern
// does not clear the warning. No issue linking.
#pragma warning(disable : 4661)
#endif

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Types::Core;
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

    const std::string outputSpace = "LoadEMUauTest";
    algToBeTested.setPropertyValue("OutputWorkspace", outputSpace);

    // should fail because mandatory parameter has not been set
    TS_ASSERT_THROWS(algToBeTested.execute(), const std::runtime_error &);

    // should succeed now
    const std::string inputFile = "EMU0006330.tar";
    algToBeTested.setPropertyValue("Filename", inputFile);
    algToBeTested.setPropertyValue("SelectDetectorTubes", "16-50");
    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    TS_ASSERT(algToBeTested.isExecuted());

    // get workspace generated
    MatrixWorkspace_sptr output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace);

    // check number of histograms
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 6592);
    double sum = 0.0;
    for (size_t i = 0; i < output->getNumberHistograms(); i++)
      sum += output->readY(i)[0];
    TS_ASSERT_EQUALS(sum, 55126);

    // check that all required log values are there
    auto run = output->run();

    // test start and end time
    TS_ASSERT(run.getProperty("start_time")->value().compare("2018-07-26T10:13:12") == 0)
    TS_ASSERT(run.getProperty("end_time")->value().find("2018-07-26T10:17:12.6") == 0)

    // test some data properties
    auto logpm = [&run](const std::string &tag) {
      return dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty(tag))->firstValue();
    };
    TS_ASSERT_DELTA(logpm("DopplerFrequency"), 9.974, 1.0e-3);
    TS_ASSERT_DELTA(logpm("DopplerAmplitude"), 0.075, 1.0e-4);

    TS_ASSERT_DELTA(logpm("ReactorPower"), 19.066, 1.0e-3);
    TS_ASSERT_DELTA(logpm("ScanPeriod"), 240.733, 1.0e-3);
    TS_ASSERT_DELTA(logpm("env_P01PSP03"), 20.0, 1.0e-3);
    TS_ASSERT_DELTA(logpm("env_T01S00"), 295.002, 1.0e-3);
    TS_ASSERT_DELTA(logpm("env_T02SP06"), 300.0, 1.0e-3);
    TS_ASSERT_THROWS(logpm("env_T3S1"), const std::runtime_error &);

    // test some instrument parameters
    auto instr = output->getInstrument();
    auto iparam = [&instr](const std::string &tag) { return instr->getNumberParameter(tag)[0]; };
    TS_ASSERT_DELTA(iparam("AnalysedV2"), 630.866, 1.0e-3);
    TS_ASSERT_DELTA(iparam("SampleAnalyser"), 1.8, 1.0e-3);
  }

  void test_load_beam_monitor() {
    LoadEMUTar algToBeTested;

    if (!algToBeTested.isInitialized())
      algToBeTested.initialize();

    std::string outputSpace = "LoadEMUauTest";
    algToBeTested.setPropertyValue("OutputWorkspace", outputSpace);

    // The test is peculiar to the physical configuration and requirements from
    // the scientist. The pseudo beam monitor is located after the chopper and
    // maximum and minimum beam monitor rates are required. As the BM also
    // captures individual events this spectrum is available by setting the
    // "IncludeBeamMonitor" flag. The test confirms that the BM rates and
    // total counts are the same regardless of the BM spectrum and that the
    // spectrum data is available when the flag is set.

    const std::string inputFile = "EMU0020493.tar";
    algToBeTested.setPropertyValue("Filename", inputFile);
    algToBeTested.setPropertyValue("SelectDetectorTubes", "16-50");
    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    TS_ASSERT(algToBeTested.isExecuted());

    // get workspace generated and run
    EventWorkspace_sptr output = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outputSpace);
    auto run = output->run();

    // useful lambda fns
    auto logpm = [&run](const std::string &tag) {
      return dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty(tag))->firstValue();
    };
    auto logpmi = [&run](const std::string &tag) {
      return dynamic_cast<TimeSeriesProperty<int> *>(run.getProperty(tag))->firstValue();
    };
    // check number of histograms and the sum
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 6592);
    TS_ASSERT_EQUALS(output->getNumberEvents(), 3135);

    // check the beam monitor rates
    TS_ASSERT_DELTA(logpm("BeamMonitorRate"), 1482.76, 1.0e-2);
    TS_ASSERT_DELTA(logpm("BeamMonitorBkgRate"), 1.01, 1.0e-2);
    TS_ASSERT_EQUALS(logpmi("MonitorCounts"), 27510);

    // repeat with the beam spectrum included
    outputSpace = "LoadEMUauTestA";
    algToBeTested.setPropertyValue("OutputWorkspace", outputSpace);
    algToBeTested.setPropertyValue("IncludeBeamMonitor", "1");
    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    TS_ASSERT(algToBeTested.isExecuted());
    output = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outputSpace);
    run = output->run();

    // check total number of events, and counts
    TS_ASSERT_EQUALS(output->getNumberEvents(), 3135 + 27510);
    TS_ASSERT_DELTA(logpm("BeamMonitorRate"), 1482.76, 1.0e-2);
    TS_ASSERT_DELTA(logpm("BeamMonitorBkgRate"), 1.01, 1.0e-2);
    TS_ASSERT_EQUALS(logpmi("MonitorCounts"), 27510);
  }

  void test_find_definition_file() {
    const std::string instname = "EMUau";
    const std::string preMod("2018-07-26 10:13:12");
    auto filename = InstrumentFileFinder::getInstrumentFilename(instname, preMod);

    // confirm that file "EMUau_definition_2025.xml" is returned but ignore the file path
    std::filesystem::path prePath(filename);
    TS_ASSERT_EQUALS(prePath.filename(), "EMUau_Definition_2025.xml");

    std::string postMod("2025-07-26 10:13:12");
    filename = InstrumentFileFinder::getInstrumentFilename(instname, postMod);
    std::filesystem::path postPath(filename);
    TS_ASSERT_EQUALS(postPath.filename(), "EMUau_Definition.xml");
  }
};
