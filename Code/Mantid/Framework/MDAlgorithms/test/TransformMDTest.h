#ifndef MANTID_MDALGORITHMS_TRANSFORMMDTEST_H_
#define MANTID_MDALGORITHMS_TRANSFORMMDTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidMDAlgorithms/TransformMD.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"

using namespace Mantid;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;
using namespace Mantid::MDEvents;

class TransformMDTest : public CxxTest::TestSuite
{
public:

  void test_Init()
  {
    TransformMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  

  void test_exec_InMemory()
  {
    do_test(false);
  }

  void test_exec_FileBacked()
  {
    do_test(true);
  }

  void test_exec_InMemory_InPlace()
  {
    do_test(false, true);
  }

  void do_test(bool fileBacked, bool inPlace=false)
  {
    // Name of the output workspace.
    std::string outWSName("TransformMDTest_OutputWS");
    std::string inWSName("TransformMDTest_ws");
    if (inPlace) outWSName = inWSName;

    // Make a fake file-backed (or not) MDEW
    MDEventWorkspace3Lean::sptr ws1 = MDEventsTestHelper::makeFileBackedMDEW(inWSName, fileBacked);
  
    TransformMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", inWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Scaling", "2") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Offset", "21") );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    // Retrieve the workspace from data service.
    MDEventWorkspace3Lean::sptr  ws2;
    TS_ASSERT_THROWS_NOTHING( ws2 = boost::dynamic_pointer_cast<MDEventWorkspace3Lean>(AnalysisDataService::Instance().retrieve(outWSName)) );
    TS_ASSERT(ws2); if (!ws2) return;
    
    for (size_t d=0; d<ws2->getNumDims(); d++)
    {
      TS_ASSERT_DELTA( ws2->getDimension(d)->getMinimum(), 21.0, 1e-5);
      TS_ASSERT_DELTA( ws2->getDimension(d)->getMaximum(), 41.0, 1e-5);
    }
    std::vector<IMDBox3Lean*> boxes;
    ws2->getBox()->getBoxes(boxes, 1000, true);
    for (size_t i=0; i<boxes.size(); i++)
    {
      IMDBox3Lean* box = boxes[i];
      TSM_ASSERT_LESS_THAN( "Box extents was offset", 20.0, box->getExtents(0).min );
      // More detailed tests are in MDBox, IMDBox and MDGridBox.
    }

    // Clean up files
    ws1->getBoxController()->closeFile(true);
    ws2->getBoxController()->closeFile(true);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(inWSName);
    if (!inPlace) AnalysisDataService::Instance().remove(outWSName);
  }



  //--------------------------------------------------------------------------------------------
  void do_test_histo(MDHistoWorkspace_sptr ws1, bool inPlace=false)
  {
    // Name of the output workspace.
    std::string outWSName("TransformMDTest_OutputWS");
    std::string inWSName("TransformMDTest_ws");
    if (inPlace) outWSName = inWSName;
    AnalysisDataService::Instance().addOrReplace(inWSName, ws1);

    TransformMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", inWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Scaling", "2") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Offset", "21") );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    // Retrieve the workspace from data service.
    MDHistoWorkspace_sptr ws2;
    TS_ASSERT_THROWS_NOTHING( ws2 = boost::dynamic_pointer_cast<MDHistoWorkspace>(AnalysisDataService::Instance().retrieve(outWSName)) );
    TS_ASSERT(ws2); if (!ws2) return;

    for (size_t d=0; d<2; d++)
    {
      TS_ASSERT_DELTA( ws2->getDimension(d)->getMinimum(), 21.0, 1e-5);
      TS_ASSERT_DELTA( ws2->getDimension(d)->getMaximum(), 41.0, 1e-5);
    }

    TS_ASSERT_DELTA( ws2->getInverseVolume(), 1./16., 1e-6);
    coord_t point[2] = {21.1, 21.1};
    TS_ASSERT_DELTA( ws2->getSignalAtCoord(point), 1.23 * ws2->getInverseVolume(), 1e-6);


    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(inWSName);
    if (!inPlace) AnalysisDataService::Instance().remove(outWSName);
  }

  void test_MDHistoWorkspace_2D()
  {
    MDHistoWorkspace_sptr ws1 = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.23, 2, 5, 10.0, 2.34);
    do_test_histo(ws1);
  }

  void test_MDHistoWorkspace_2D_InPlace()
  {
    MDHistoWorkspace_sptr ws1 = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.23, 2, 5, 10.0, 2.34);
    do_test_histo(ws1, true);
  }


};


#endif /* MANTID_MDALGORITHMS_TRANSFORMMDTEST_H_ */
