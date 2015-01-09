#ifndef MANTID_DATAHANDLING_LOADSPICEASCIITEST_H_
#define MANTID_DATAHANDLING_LOADSPICEASCIITEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadSpiceAscii.h"

using Mantid::DataHandling::LoadSpiceAscii;
using namespace Mantid::API;

class LoadSpiceAsciiTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadSpiceAsciiTest *createSuite() { return new LoadSpiceAsciiTest(); }
  static void destroySuite( LoadSpiceAsciiTest *suite ) { delete suite; }


  void test_Setup()
  {
    LoadSpiceAscii testalg;
    testalg.initialize();;
    TS_ASSERT(testalg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(testalg.setProperty("Filename", "HB2A_exp0231_scan0001.dat"));
    TS_ASSERT_THROWS_NOTHING(testalg.setProperty("OutputWorkspace", "HB2A_0231_0001_Data"));
    TS_ASSERT_THROWS_NOTHING(testalg.setProperty("RunInfoWorkspace", "HB2A_0231_Info"));
    TS_ASSERT_THROWS_NOTHING(testalg.setProperty("IgnoreUnlistedLogs", false));
  }

  void test_LoadSpiceHB2A()
  {
    LoadSpiceAscii testalg;
    testalg.initialize();;
    TS_ASSERT(testalg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(testalg.setProperty("Filename", "HB2A_exp0231_scan0001.dat"));
    TS_ASSERT_THROWS_NOTHING(testalg.setProperty("OutputWorkspace", "HB2A_0231_0001_Data"));
    TS_ASSERT_THROWS_NOTHING(testalg.setProperty("RunInfoWorkspace", "HB2A_0231_Info"));
    TS_ASSERT_THROWS_NOTHING(testalg.setProperty("IgnoreUnlistedLogs", false));
    TS_ASSERT_THROWS_NOTHING(testalg.setPropertyValue(
        "DateAndTimeLog", "date, M/D/Y, time, H:M:S M"));

    testalg.execute();
    TS_ASSERT(testalg.isExecuted());


    Mantid::API::ITableWorkspace_sptr datatbws = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(
          AnalysisDataService::Instance().retrieve("HB2A_0231_0001_Data"));
    TS_ASSERT(datatbws);

    size_t numcols = datatbws->columnCount();
    TS_ASSERT_EQUALS(numcols, 70);

    std::vector<std::string> colnames = datatbws->getColumnNames();
    TS_ASSERT_EQUALS(colnames[0].compare("Pt."), 0);

    size_t numrows = datatbws->rowCount();
    TS_ASSERT_EQUALS(numrows, 61);

    Mantid::API::MatrixWorkspace_sptr infows = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
          AnalysisDataService::Instance().retrieve("HB2A_0231_Info"));
    TS_ASSERT(infows);

    // With WorkspaceFactory, there is no run_title added automatically
    TS_ASSERT(!infows->run().hasProperty("run_title"));

    TS_ASSERT(infows->run().hasProperty("proposal"));
    Mantid::Kernel::Property *p1 = infows->run().getProperty("proposal");
    std::string ipts = p1->value();
    TS_ASSERT_EQUALS(ipts.compare("IPTS-6174"), 0);

    // Run start
    std::string runstart = infows->run().getProperty("run_start")->value();
    TS_ASSERT_EQUALS(runstart, "2012-08-13T13:07:33");

    // Run end
    std::string runend = infows->run().getProperty("runend")->value();
    TS_ASSERT_EQUALS(runend, "12:33:21 PM  8/13/2012");

    std::vector<Mantid::Kernel::Property*> properties = infows->run().getProperties();
    TS_ASSERT_EQUALS(properties.size(), 33);

    AnalysisDataService::Instance().remove("HB2A_0231_Info");
    AnalysisDataService::Instance().remove("HB2A_0231_0001_Data");
  }

  void test_SpiceHB3A()
  {
    LoadSpiceAscii testalg;
    testalg.initialize();

    testalg.setProperty("Filename", "HB2A_exp0231_scan0001.dat");
    testalg.setPropertyValue("StringSampleLogNames", "a,experiment, scan_title, b, proposal");
    testalg.setPropertyValue("IntegerSampleLogNames", "Sum of Counts, scan, mode, experiment_number");
    testalg.setPropertyValue("FloatSampleLogNames", "samplemosaic, preset_value, Full Width Half-Maximum, Center of Mass");
    testalg.setPropertyValue("OutputWorkspace", "HB2A_0231_0001_Data");
    testalg.setPropertyValue("RunInfoWorkspace", "HB2A_0231_Info2");
    testalg.setProperty("IgnoreUnlistedLogs", true);

    testalg.execute();
    TS_ASSERT(testalg.isExecuted());

    Mantid::API::ITableWorkspace_sptr datatbws = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(
          AnalysisDataService::Instance().retrieve("HB2A_0231_0001_Data"));
    TS_ASSERT(datatbws);

    size_t numcols = datatbws->columnCount();
    TS_ASSERT_EQUALS(numcols, 70);

    size_t numrows = datatbws->rowCount();
    TS_ASSERT_EQUALS(numrows, 61);

    std::vector<std::string> colnames = datatbws->getColumnNames();
    TS_ASSERT_EQUALS(colnames[0], "Pt.");

    Mantid::API::MatrixWorkspace_sptr runinfows = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
          AnalysisDataService::Instance().retrieve("HB2A_0231_Info2"));
    TS_ASSERT(runinfows);

    std::vector<Mantid::Kernel::Property*> properties = runinfows->run().getProperties();
    TS_ASSERT_EQUALS(properties.size(), 13);

    std::string ipts = runinfows->run().getProperty("proposal")->value();
    TS_ASSERT_EQUALS(ipts, "IPTS-6174");

    int mode = atoi(runinfows->run().getProperty("mode")->value().c_str());
    TS_ASSERT_EQUALS(mode, 3);

    double comerr = atof(runinfows->run().getProperty("Center of Mass.error")->value().c_str());
    TS_ASSERT_DELTA(comerr, 0.009214, 0.000001);

    AnalysisDataService::Instance().remove("HB2A_0231_0001_Data");
    AnalysisDataService::Instance().remove("HB2A_0231_Info2");

  }

  void test_ThrowsException()
  {
    LoadSpiceAscii testalg;
    testalg.initialize();

    testalg.setProperty("Filename", "HB2A_exp0231_scan0001.dat");
    testalg.setPropertyValue("StringSampleLogNames", "a, experiment, scan_title, b, proposal");
    testalg.setPropertyValue("IntegerSampleLogNames", "a, Sum of Counts, scan, mode, experiment_number");
    testalg.setPropertyValue("FloatSampleLogNames", "samplemosaic, preset_value, Full Width Half-Maximum, Center of Mass");
    testalg.setPropertyValue("OutputWorkspace", "HB2A_0231_0001_Data");
    testalg.setPropertyValue("RunInfoWorkspace", "HB2A_0231_Info2");
    testalg.setProperty("IgnoreUnlistedLogs", true);

    testalg.execute();
    TS_ASSERT(!testalg.isExecuted());
  }

};


#endif /* MANTID_DATAHANDLING_LOADSPICEASCIITEST_H_ */
