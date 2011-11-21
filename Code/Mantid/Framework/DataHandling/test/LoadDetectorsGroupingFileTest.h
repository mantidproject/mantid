#ifndef MANTID_DATAHANDLING_LOADDETECTORSGROUPINGFILETEST_H_
#define MANTID_DATAHANDLING_LOADDETECTORSGROUPINGFILETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidDataHandling/LoadDetectorsGroupingFile.h"
#include "MantidDataObjects/GroupingWorkspace.h"

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::API;

class LoadDetectorsGroupingFileTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadDetectorsGroupingFileTest *createSuite() { return new LoadDetectorsGroupingFileTest(); }
  static void destroySuite( LoadDetectorsGroupingFileTest *suite ) { delete suite; }


  void test_Init(){

    LoadDetectorsGroupingFile load;
    load.initialize();
    TS_ASSERT(load.isInitialized());

  }

  void test_DetectorsGroupingXMLFile()
  {
    /*
     * 0-3695 : 1.0
     * 3696-(-1): 2.0
     */

    LoadDetectorsGroupingFile load;
    load.initialize();

    TS_ASSERT(load.setProperty("InputFile", "vulcangroup.xml"));
    TS_ASSERT(load.setProperty("OutputWorkspace", "Vulcan_Group"));

    load.execute();
    TS_ASSERT(load.isExecuted());

    DataObjects::GroupingWorkspace_sptr gws = boost::dynamic_pointer_cast<DataObjects::GroupingWorkspace>(API::AnalysisDataService::Instance().retrieve("Vulcan_Group"));

    TS_ASSERT_DELTA(gws->dataY(0)[0],    1.0, 1.0E-5);
    TS_ASSERT_DELTA(gws->dataY(3695)[0], 1.0, 1.0E-5);
    TS_ASSERT_DELTA(gws->dataY(3696)[0], 2.0, 1.0E-5);
    TS_ASSERT_DELTA(gws->dataY(7000)[0], 2.0, 1.0E-5);

  }


};


#endif /* MANTID_DATAHANDLING_LOADDETECTORSGROUPINGFILETEST_H_ */
