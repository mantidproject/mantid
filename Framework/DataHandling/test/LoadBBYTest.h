// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <fstream>
#include <iostream>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Run.h"
#include "MantidDataHandling/LoadBBY.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include <Poco/Path.h>
#include <Poco/TemporaryFile.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::Types::Core;

namespace {

void copyFile(std::string &srcPath, std::string &dstPath) {
  std::ifstream source(srcPath, std::ios::binary);
  std::ofstream dest(dstPath, std::ios::binary);

  dest << source.rdbuf();

  source.close();
  dest.close();
}

void replaceValue(std::string &tarPath, int offset, char invalid) {

  std::fstream destn(tarPath, std::ios::binary | std::ios::in | std::ios::out);
  destn.seekp(offset);
  destn.write(&invalid, 1);
  destn.close();
}
} // namespace

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
    TS_ASSERT_THROWS(algToBeTested.execute(), const std::runtime_error &);

    // should succeed now
    std::string inputFile = "BBY0000014.tar";
    algToBeTested.setPropertyValue("Filename", inputFile);
    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    TS_ASSERT(algToBeTested.isExecuted());

    // get workspace generated
    MatrixWorkspace_sptr output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace);

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
    TS_ASSERT(run.getProperty("start_time")->value().compare("2014-06-17T09:59:31") == 0)
    TS_ASSERT(run.getProperty("end_time")->value().find("2014-06-17T09:59:31.08") == 0)

    // test data properties
    TS_ASSERT_EQUALS(run.getPropertyValueAsType<int>("att_pos"), 1);
    TS_ASSERT_EQUALS(run.getPropertyValueAsType<int>("frame_count"), 4);
    TS_ASSERT_DELTA(run.getPropertyValueAsType<double>("period"), 20000.0, 1.0e-5);
    TS_ASSERT_DELTA(run.getPropertyValueAsType<double>("bm_counts"), 0.0800, 1.0e-5);

    // test string log properties
    TS_ASSERT(run.getProperty("rough_40")->value().compare("moving") == 0);
    TS_ASSERT(run.getProperty("rough_100")->value().compare("moving") == 0);

    // test instrument setup
    TS_ASSERT_DELTA(dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty("L1_chopper_value"))->firstValue(),
                    18.4726, 1.0e-3);
    TS_ASSERT_DELTA(dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty("L1"))->firstValue(), 9.35959, 1.0e-3);

    TS_ASSERT_DELTA(dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty("L2_det_value"))->firstValue(), 33.1562,
                    1.0e-3);
    TS_ASSERT_DELTA(dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty("L2_curtainl_value"))->firstValue(),
                    23.2845, 1.0e-3);
    TS_ASSERT_DELTA(dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty("L2_curtainr_value"))->firstValue(),
                    23.2820, 1.0e-3);
    TS_ASSERT_DELTA(dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty("L2_curtainu_value"))->firstValue(),
                    24.2862, 1.0e-3);
    TS_ASSERT_DELTA(dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty("L2_curtaind_value"))->firstValue(),
                    24.2824, 1.0e-3);

    TS_ASSERT_DELTA(dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty("D_curtainl_value"))->firstValue(),
                    0.3816, 1.0e-4);
    TS_ASSERT_DELTA(dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty("D_curtainr_value"))->firstValue(),
                    0.4024, 1.0e-4);
    TS_ASSERT_DELTA(dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty("D_curtainu_value"))->firstValue(),
                    0.3947, 1.0e-4);
    TS_ASSERT_DELTA(dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty("D_curtaind_value"))->firstValue(),
                    0.3978, 1.0e-4);
    TS_ASSERT_DELTA(dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty("curtain_rotation"))->firstValue(), 10,
                    1.0e-7);
  }

  void test_filter_bby_algorithm() {
    LoadBBY algToBeTested;

    if (!algToBeTested.isInitialized())
      algToBeTested.initialize();

    // Filter the event by pulse time when loading
    // and confirm that the events are within the range
    std::string outputSpace = "LoadBBYTest";
    algToBeTested.setPropertyValue("OutputWorkspace", outputSpace);
    std::string inputFile = "BBY0000014.tar";
    algToBeTested.setPropertyValue("Filename", inputFile);
    algToBeTested.setPropertyValue("FilterByTimeStart", "0.04");
    algToBeTested.setPropertyValue("FilterByTimeStop", "0.06");

    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    TS_ASSERT(algToBeTested.isExecuted());

    // check the filtered events

    // get workspace generated
    EventWorkspace_sptr output = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outputSpace);
    auto run = output->run();

    // check the number of events and the min and max pulse range
    TS_ASSERT_EQUALS(output->getNumberEvents(), 100000);
    auto minPulseTime = output->getPulseTimeMin();
    auto maxPulseTime = output->getPulseTimeMax();

    DateAndTime runstart(run.getProperty("run_start")->value());
    auto timeshift = runstart.totalNanoseconds();
    double minTime = static_cast<double>(minPulseTime.totalNanoseconds() - timeshift) * 1.0e-9;
    double maxTime = static_cast<double>(maxPulseTime.totalNanoseconds() - timeshift) * 1.0e-9;

    TS_ASSERT_LESS_THAN(maxTime, 0.0600001);
    TS_ASSERT_LESS_THAN(0.0399999, minTime);
  }

  void test_default_parameters_logged() {
    LoadBBY algToBeTested;

    if (!algToBeTested.isInitialized())
      algToBeTested.initialize();

    std::string outputSpace = "LoadBBYTestB";
    algToBeTested.setPropertyValue("OutputWorkspace", outputSpace);
    std::string inputFile = "BBY0000014.tar";
    algToBeTested.setPropertyValue("Filename", inputFile);

    // execute and get workspace generated
    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    TS_ASSERT(algToBeTested.isExecuted());
    EventWorkspace_sptr output = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outputSpace);
    auto run = output->run();

    // confirm that the sample_aperture which is not included in the hdf file
    // is present in the log and set to the default value
    TS_ASSERT_DELTA(dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty("sample_aperture"))->firstValue(), 0.0,
                    1.0e-3);
    TS_ASSERT_DELTA(dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty("source_aperture"))->firstValue(), 0.0,
                    1.0e-3);

    // confirm that the dummy test parameter in the xml without a default is not added to the log
    TS_ASSERT(!run.hasProperty("sample_xxx"))
  }

  void test_invalid_event_logged() {
    LoadBBY algToBeTested;

    if (!algToBeTested.isInitialized())
      algToBeTested.initialize();

    std::string outputSpace = "LoadBBYTestA";
    algToBeTested.setPropertyValue("OutputWorkspace", outputSpace);
    std::string inputFile = "BBY0000014.tar";
    algToBeTested.setPropertyValue("Filename", inputFile);

    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    TS_ASSERT(algToBeTested.isExecuted());
    EventWorkspace_sptr eventWS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outputSpace);
    auto goodEvents = eventWS->getNumberEvents();

    // corrupt the value at the offset in the tar file to be out of bounds
    // and confirm the file is loaded but with one less event. The offset
    // was manually determined from the good file.
    std::string filename = algToBeTested.getPropertyValue("Filename");
    Poco::TemporaryFile tempFile;
    std::string tempPath = tempFile.path();
    copyFile(filename, tempPath);
    replaceValue(tempPath, 595456 + 136, (char)0xff);

    outputSpace = "LoadBBYTestB";
    algToBeTested.setPropertyValue("OutputWorkspace", outputSpace);
    algToBeTested.setPropertyValue("Filename", tempPath);
    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    TS_ASSERT(algToBeTested.isExecuted());
    eventWS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outputSpace);
    TS_ASSERT_EQUALS(eventWS->getNumberEvents(), goodEvents - 1);
  }
};
