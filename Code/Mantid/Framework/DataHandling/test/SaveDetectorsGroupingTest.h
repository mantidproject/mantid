#ifndef MANTID_DATAHANDLING_SAVEDETECTORSGROUPINGTEST_H_
#define MANTID_DATAHANDLING_SAVEDETECTORSGROUPINGTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidDataHandling/SaveDetectorsGrouping.h"
#include "MantidDataHandling/LoadDetectorsGroupingFile.h"
#include "Poco/File.h"

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::API;

class SaveDetectorsGroupingTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveDetectorsGroupingTest *createSuite() { return new SaveDetectorsGroupingTest(); }
  static void destroySuite( SaveDetectorsGroupingTest *suite ) { delete suite; }


  void test_Initialize()
  {
    SaveDetectorsGrouping savegroup;
    savegroup.initialize();
    TS_ASSERT(savegroup.isInitialized());

  }

  void test_SaveXMLFile(){

    // 1. Get an object for SaveDetectorsGrouping
    SaveDetectorsGrouping savegroup;
    savegroup.initialize();

    // 2. Create a Workspace from an XML file
    LoadDetectorsGroupingFile load;
    load.initialize();

    TS_ASSERT(load.setProperty("InputFile", "vulcangroup.xml"));
    TS_ASSERT(load.setProperty("OutputWorkspace", "Vulcan_Group"));

    load.execute();
    TS_ASSERT(load.isExecuted());

    DataObjects::GroupingWorkspace_sptr gws = boost::dynamic_pointer_cast<DataObjects::GroupingWorkspace>(API::AnalysisDataService::Instance().retrieve("Vulcan_Group"));

    // 3. Save
    savegroup.setProperty("InputWorkspace", gws);
    savegroup.setProperty("OutputFile", "grouptemp.xml");

    savegroup.execute();
    TS_ASSERT(savegroup.isExecuted());

    // 4. Verify
    LoadDetectorsGroupingFile load2;
    load2.initialize();

    TS_ASSERT(load2.setProperty("InputFile", "grouptemp.xml"));
    TS_ASSERT(load2.setProperty("OutputWorkspace", "Vulcan_Group2"));

    load2.execute();
    TS_ASSERT(load2.isExecuted());

    DataObjects::GroupingWorkspace_sptr gws2 = boost::dynamic_pointer_cast<DataObjects::GroupingWorkspace>(API::AnalysisDataService::Instance().retrieve("Vulcan_Group2"));

    TS_ASSERT_DELTA(gws2->dataY(0)[0],    1.0, 1.0E-5);
    TS_ASSERT_DELTA(gws2->dataY(3695)[0], 1.0, 1.0E-5);
    TS_ASSERT_DELTA(gws2->dataY(3696)[0], 2.0, 1.0E-5);
    TS_ASSERT_DELTA(gws2->dataY(7000)[0], 2.0, 1.0E-5);

    // 5. Clear
    Poco::File file("grouptemp.xml");
    file.remove(false);

  }


};


#endif /* MANTID_DATAHANDLING_SAVEDETECTORSGROUPINGTEST_H_ */
