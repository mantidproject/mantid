#ifndef MANTID_MDALGORITHMS_SAVEZODSTEST_H_
#define MANTID_MDALGORITHMS_SAVEZODSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidMDAlgorithms/SaveZODS.h"
#include <Poco/File.h>
#include "MantidTestHelpers/MDEventsTestHelper.h"

using namespace Mantid;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;
using namespace Mantid::MDEvents;

class SaveZODSTest : public CxxTest::TestSuite
{
public:

  void test_Init()
  {
    SaveZODS alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  std::string do_test(std::string InputWorkspace, std::string Filename, bool expectSuccess = true)
  {
    SaveZODS alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", InputWorkspace) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", Filename) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    if (expectSuccess)
    { TS_ASSERT( alg.isExecuted() ); }
    else
    { TS_ASSERT( !alg.isExecuted() ); }
    // Return full path to file
    return alg.getPropertyValue("Filename");
  }


  void test_exec()
  {
    size_t numBins[3] = {10, 12, 2};
    coord_t min[3] = {0, 10, 0};
    coord_t max[3] = {10, 34, 10};
    MDHistoWorkspace_sptr ws = MDEventsTestHelper::makeFakeMDHistoWorkspaceGeneral(
        3, 1.0, 2.0, numBins, min, max, "mdhisto3");
    for (size_t x=0; x<10; x++)
      ws->setSignalAt(ws->getLinearIndex(x,0,0), double(x)+1.0);
    std::string Filename =  do_test("mdhisto3", "SaveZODS_test.h5");

    // Check the results
    Poco::File file(Filename);
    TS_ASSERT( file.exists());
//    if (file.exists())
//      file.remove();
    
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove("mdhisto3");
  }



};


#endif /* MANTID_MDALGORITHMS_SAVEZODSTEST_H_ */
