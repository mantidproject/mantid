#ifndef LOADLOGTEST_H_
#define LOADLOGTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadLog.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Instrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <vector>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class LoadLogTest : public CxxTest::TestSuite {
public:
  static LoadLogTest *createSuite() { return new LoadLogTest(); }
  static void destroySuite(LoadLogTest *suite) { delete suite; }

  LoadLogTest() {
    // initialise framework manager to allow logging
    // Mantid::API::FrameworkManager::Instance().initialize();
  }

  void testInit() {
    TS_ASSERT(!loader.isInitialized());
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
  }

  void testThreeColumnLogFile() {
    if (!loader.isInitialized())
      loader.initialize();
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("Filename", "NIMROD00001097.log"))
    inputFile = loader.getPropertyValue("Filename");

    outputSpace = "LoadLogTest-threeColumnFile";
    TS_ASSERT_THROWS(loader.setPropertyValue("Workspace", outputSpace),
                     std::invalid_argument)

    Workspace_sptr ws =
        WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);

    TS_ASSERT_THROWS_NOTHING(
        AnalysisDataService::Instance().add(outputSpace, ws));

    std::string result;
    TS_ASSERT_THROWS_NOTHING(result = loader.getPropertyValue("Filename"))
    TS_ASSERT(!result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING(result = loader.getPropertyValue("Workspace"))
    TS_ASSERT(!result.compare(outputSpace));

    TS_ASSERT_THROWS_NOTHING(loader.execute());

    TS_ASSERT(loader.isExecuted());

    // Get back the saved workspace
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputSpace));

    // This value is at the start of the log file.
    Property *l_property = output->run().getLogData(std::string("BeamCurrent"));
    TimeSeriesProperty<double> *l_timeSeriesDouble =
        dynamic_cast<TimeSeriesProperty<double> *>(l_property);
    std::string timeSeriesString = l_timeSeriesDouble->value();
    TS_ASSERT_EQUALS(timeSeriesString.substr(0, 20), "2009-Nov-10 10:14:03");
    // While this value is at the end.
    l_property = output->run().getLogData(std::string("J6CX"));
    l_timeSeriesDouble = dynamic_cast<TimeSeriesProperty<double> *>(l_property);
    timeSeriesString = l_timeSeriesDouble->value();
    TS_ASSERT_EQUALS(timeSeriesString.substr(0, 20), "2009-Nov-10 17:22:14");

    AnalysisDataService::Instance().remove(outputSpace);
  }

  void testTwoColumnLogFile() {
    if (!loader.isInitialized())
      loader.initialize();

    // Path to test input file assumes Test directory checked out from SVN
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("Filename", "HRP37129_ICPevent.txt"))
    inputFile = loader.getPropertyValue("Filename");

    outputSpace = "LoadLogTest-singleLogFile";
    TS_ASSERT_THROWS(loader.setPropertyValue("Workspace", outputSpace),
                     std::invalid_argument)
    // Create an empty workspace and put it in the AnalysisDataService
    Workspace_sptr ws =
        WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);

    TS_ASSERT_THROWS_NOTHING(
        AnalysisDataService::Instance().add(outputSpace, ws));

    std::string result;
    TS_ASSERT_THROWS_NOTHING(result = loader.getPropertyValue("Filename"))
    TS_ASSERT(!result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING(result = loader.getPropertyValue("Workspace"))
    TS_ASSERT(!result.compare(outputSpace));

    TS_ASSERT_THROWS_NOTHING(loader.execute());

    TS_ASSERT(loader.isExecuted());

    // Get back the saved workspace
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputSpace));

    // boost::shared_ptr<Sample> sample = output->getSample();

    Property *l_property = output->run().getLogData("HRP37129_ICPevent");
    TimeSeriesProperty<std::string> *l_timeSeries =
        dynamic_cast<TimeSeriesProperty<std::string> *>(l_property);
    std::string timeSeriesString = l_timeSeries->value();
    TS_ASSERT_EQUALS(timeSeriesString.substr(0, 26),
                     "2007-Nov-16 13:25:48   END");

    AnalysisDataService::Instance().remove(outputSpace);
  }

  void do_test_SNSTextFile(std::string names, std::string units, bool willFail,
                           bool createWorkspace = true,
                           std::string expectedLastUnit = "Furlongs") {
    // Create an empty workspace and put it in the AnalysisDataService

    outputSpace = "test_SNSTextFile";
    if (createWorkspace) {
      Workspace_sptr ws =
          WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);
      TS_ASSERT_THROWS_NOTHING(
          AnalysisDataService::Instance().addOrReplace(outputSpace, ws));
    }

    // Set up the algo
    LoadLog alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("Filename", "VULCAN_furnace4208.txt"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Workspace", outputSpace));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Names", names));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Units", units));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    if (willFail) {
      TS_ASSERT(!alg.isExecuted());
      return;
    } else {
      TS_ASSERT(alg.isExecuted());
    }

    // Get back the saved workspace
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputSpace));

    Property *prop = 0;
    TimeSeriesProperty<double> *tsp;
    std::vector<double> vals;
    std::vector<DateAndTime> times;

    TS_ASSERT_THROWS_NOTHING(prop = output->run().getLogData("Temp1"));
    tsp = dynamic_cast<TimeSeriesProperty<double> *>(prop);
    TS_ASSERT(tsp);
    if (!tsp)
      return;
    vals = tsp->valuesAsVector();
    times = tsp->timesAsVector();
    TS_ASSERT_EQUALS(vals.size(), 33);
    TS_ASSERT_EQUALS(times.size(), 33);

    TS_ASSERT_DELTA(vals[1], 0.296875, 1e-6);
    TS_ASSERT_DELTA(vals[14], 3.906250, 1e-6);
    TS_ASSERT_DELTA(vals[32], 9.000000, 1e-6);

    if (units != "")
      TS_ASSERT_EQUALS(tsp->units(), "C");

    TS_ASSERT_THROWS_NOTHING(prop = output->run().getLogData("Temp2"));
    tsp = dynamic_cast<TimeSeriesProperty<double> *>(prop);
    TS_ASSERT(tsp);
    if (!tsp)
      return;
    vals = tsp->valuesAsVector();
    times = tsp->timesAsVector();
    TS_ASSERT_DELTA(vals[1], 24, 1e-6);
    if (units != "")
      TS_ASSERT_EQUALS(tsp->units(), "K");
    TS_ASSERT_EQUALS(tsp->size(), 33);

    TS_ASSERT_THROWS_NOTHING(prop = output->run().getLogData("Temp3"));
    tsp = dynamic_cast<TimeSeriesProperty<double> *>(prop);
    TS_ASSERT(tsp);
    if (!tsp)
      return;
    if (units != "")
      TS_ASSERT_EQUALS(tsp->units(), "F");
    TS_ASSERT_EQUALS(tsp->size(), 33);

    TS_ASSERT_THROWS_NOTHING(prop = output->run().getLogData("Extra"));
    tsp = dynamic_cast<TimeSeriesProperty<double> *>(prop);
    TS_ASSERT(tsp);
    if (!tsp)
      return;
    if (units != "")
      TS_ASSERT_EQUALS(tsp->units(), expectedLastUnit);
    TS_ASSERT_EQUALS(tsp->size(), 33);
  }

  void test_SNSTextFile_noNames_fails() { do_test_SNSTextFile("", "", true); }

  void test_SNSTextFile_tooFewNames_fails() {
    do_test_SNSTextFile("Yadda,Yadda", "", true);
  }

  void test_SNSTextFile_tooManyNames_fails() {
    do_test_SNSTextFile("Yadda,Yadda,Yadda,Yadda,Yadda,Yadda", "", true);
  }

  void test_SNSTextFile() {
    do_test_SNSTextFile("Temp1,Temp2,Temp3,Extra", "C,K,F,Furlongs", false);
  }

  void test_SNSTextFile_noUnits() {
    do_test_SNSTextFile("Temp1,Temp2,Temp3,Extra", "", false);
  }

  void test_SNSTextFile_wrongNumberOfUnits_fails() {
    do_test_SNSTextFile("Temp1,Temp2,Temp3,Extra", "Dynes,Ergs", true);
  }

  void test_SNSTextFile_twice_overwrites_logs() {
    do_test_SNSTextFile("Temp1,Temp2,Temp3,Extra", "C,K,F,Furlongs", false,
                        true, "Furlongs");
    // Dont re-create the workspace the 2nd time around.
    // Switch a unit around to make sure the new one got overwritten
    do_test_SNSTextFile("Temp1,Temp2,Temp3,Extra", "C,K,F,Fortnights", false,
                        false, "Fortnights");
  }

private:
  LoadLog loader;
  std::string inputFile;
  std::string outputSpace;
  std::string inputSpace;
};

#endif /*LOADLOGTEST_H_*/
