#ifndef MANTID_DATAHANDLING_RENAMELOGTEST_H_
#define MANTID_DATAHANDLING_RENAMELOGTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidDataHandling/RenameLog.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/Property.h"

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::API;

class RenameLogTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RenameLogTest *createSuite() { return new RenameLogTest(); }
  static void destroySuite( RenameLogTest *suite ) { delete suite; }


  void test_Init()
  {
    RenameLog renlog;

    TS_ASSERT_THROWS_NOTHING(renlog.initialize());
    TS_ASSERT(renlog.isInitialized());
  }

  void test_Rename(){

    // 1. Generate workspace & 2 logs
    API::MatrixWorkspace_sptr mWS = API::WorkspaceFactory::Instance().create("Workspace2D", 1, 100, 100);

    Kernel::TimeSeriesProperty<double> *p1 = new Kernel::TimeSeriesProperty<double>("OriginalLog");

    int64_t t1_ns = 1000000;
    int64_t dt_ns = 400;
    double v1 = -1.0;
    size_t num1 = 10;
    std::vector<Kernel::DateAndTime> rawtimes;
    std::vector<double> rawvalues;

    for (size_t i=0; i<num1; i ++){
      Kernel::DateAndTime time(t1_ns);
      p1->addValue(time, v1);

      rawtimes.push_back(time);
      rawvalues.push_back(v1);

      t1_ns += dt_ns;
      v1 = -v1;
    }

    mWS->mutableRun().addProperty(p1);

    // 2. Add workspace to data serive
    AnalysisDataService::Instance().addOrReplace("TestDummy", mWS);

    // 3. Running
    RenameLog renlog;
    renlog.initialize();

    TS_ASSERT_THROWS_NOTHING(renlog.setPropertyValue("Workspace", "TestDummy"));
    TS_ASSERT_THROWS_NOTHING(renlog.setProperty("OriginalLogName", "OriginalLog"));
    TS_ASSERT_THROWS_NOTHING(renlog.setProperty("NewLogName", "NewLog"));

    renlog.execute();
    TS_ASSERT(renlog.isExecuted());

    // 4. Check
    API::MatrixWorkspace_sptr rWS = AnalysisDataService::Instance().retrieveWS<API::MatrixWorkspace>("TestDummy");

    Kernel::TimeSeriesProperty<double> *rp;
    TS_ASSERT_THROWS_NOTHING(rp = dynamic_cast<Kernel::TimeSeriesProperty<double>* >(rWS->run().getProperty("NewLog")));
    rp = dynamic_cast<Kernel::TimeSeriesProperty<double>* >(rWS->run().getProperty("NewLog"));

    std::vector<Kernel::DateAndTime> newtimes = rp->timesAsVector();
    TS_ASSERT_EQUALS(newtimes.size(), num1);
    for (size_t i=0; i<num1; i ++){
      double newvalue;
      TS_ASSERT_THROWS_NOTHING(newvalue= rp->getSingleValue(rawtimes[i]));
      newvalue= rp->getSingleValue(rawtimes[i]);
      TS_ASSERT_DELTA(newvalue, rawvalues[i], 1.0E-8);
    }
  }


};


#endif /* MANTID_DATAHANDLING_RENAMELOGTEST_H_ */
