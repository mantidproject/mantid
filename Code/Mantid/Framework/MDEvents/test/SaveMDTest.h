#ifndef MANTID_MDEVENTS_SAVEMDEWTEST_H_
#define MANTID_MDEVENTS_SAVEMDEWTEST_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/SaveMD.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>
#include "MantidKernel/CPUTimer.h"
#include "MantidTestHelpers/AlgorithmHelper.h"
#include <Poco/File.h>

using namespace Mantid::MDEvents;
using namespace Mantid::API;
using Mantid::Kernel::CPUTimer;


/** Note: See the LoadMDTest class
 * for a more thorough test that does
 * a round-trip.
 */
class SaveMDTest : public CxxTest::TestSuite
{
public:

    
  void test_Init()
  {
    SaveMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {
    do_test_exec(23, "SaveMDTest.nxs");
  }

  void test_exec_noEvents()
  {
    do_test_exec(0, "SaveMDTest_noEvents.nxs");
  }

  void test_MakeFileBacked()
  {
    do_test_exec(23, "SaveMDTest.nxs", true);
  }


  void do_test_exec(size_t numPerBox, std::string filename, bool MakeFileBacked = false)
  {
    // Make a 1D MDEventWorkspace
    MDEventWorkspace1Lean::sptr ws = MDEventsTestHelper::makeMDEW<1>(10, 0.0, 10.0, numPerBox);
    // Make sure it is split
    if (numPerBox == 0) ws->splitBox();
    // Recurse split so that it has lots more boxes, recursively
    MDEventsTestHelper::recurseSplit<1>( dynamic_cast<MDGridBox<MDLeanEvent<1>,1>*>(ws->getBox()), 0, 4);

    // Add some points
    if (numPerBox > 0)
      MDEventsTestHelper::feedMDBox(ws->getBox(), 1, 9e3, 1e-3);

    AnalysisDataService::Instance().addOrReplace("SaveMDTest_ws", ws);

    ws->refreshCache();

    // There are this many boxes, so this is the max ID.
    TS_ASSERT_EQUALS( ws->getBoxController()->getMaxId(), 111111);

    IMDEventWorkspace_sptr iws = ws;

    CPUTimer tim;

    SaveMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", "SaveMDTest_ws") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", filename) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("MakeFileBacked", MakeFileBacked) );
    alg.execute();
    TS_ASSERT( alg.isExecuted() );

    std::cout << tim << " to save " << ws->getBoxController()->getMaxId() << " boxes." << std::endl;

    std::string this_filename = alg.getProperty("Filename");
    TSM_ASSERT( "File was indeed created", Poco::File(this_filename).exists());

    if (MakeFileBacked)
    {
      TSM_ASSERT( "Workspace was made file-backed", ws->isFileBacked() );
    }
  }
  

};



class SaveMDTestPerformance : public CxxTest::TestSuite
{
public:
  MDEventWorkspace3Lean::sptr  ws;
  void setUp()
  {
    CPUTimer tim;

    // Make a 1D MDEventWorkspace
    ws = MDEventsTestHelper::makeMDEW<3>(10, 0.0, 10.0, 0);
    ws->getBoxController()->setSplitInto(5);
    ws->getBoxController()->setSplitThreshold(2000);

    AnalysisDataService::Instance().addOrReplace("SaveMDTestPerformance_ws", ws);

    AlgorithmHelper::runAlgorithm("FakeMDEventData", 4,
        "InputWorkspace", "SaveMDTestPerformance_ws", "UniformParams", "10000000");

    std::cout << tim << " to fake the data." << std::endl;
    ws->refreshCache();
    std::cout << tim << " to refresh cache." << std::endl;

//    // There are this many boxes, so this is the max ID.
//    TS_ASSERT_EQUALS( ws->getBoxController()->getMaxId(), 11111);

  }

  void test_exec_3D()
  {
    CPUTimer tim;

    SaveMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", "SaveMDTestPerformance_ws") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", "SaveMDTestPerformance.nxs") );
    alg.execute();
    TS_ASSERT( alg.isExecuted() );

    std::cout << tim << " to save " << ws->getBoxController()->getMaxId() << " boxes with " << double(ws->getNPoints())/1e6 << " million events." << std::endl;
  }


};


#endif /* MANTID_MDEVENTS_SAVEMDEWTEST_H_ */

