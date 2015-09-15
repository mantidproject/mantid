#ifndef MANTID_MDALGORITHMS_TRANSFORMMDTEST_H_
#define MANTID_MDALGORITHMS_TRANSFORMMDTEST_H_

#include "MantidDataObjects/MDEventFactory.h"
#include "MantidMDAlgorithms/TransformMD.h"
#include "MantidTestHelpers/MDAlgorithmsTestHelper.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::MDAlgorithms;


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
    MDEventWorkspace3Lean::sptr ws1 = MDAlgorithmsTestHelper::makeFileBackedMDEW(inWSName, fileBacked);
  
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
    TS_ASSERT_THROWS_NOTHING( ws2 = AnalysisDataService::Instance().retrieveWS<MDEventWorkspace3Lean>(outWSName) );
    TS_ASSERT(ws2); if (!ws2) return;
    
    for (size_t d=0; d<ws2->getNumDims(); d++)
    {
      TS_ASSERT_DELTA( ws2->getDimension(d)->getMinimum(), 21.0, 1e-5);
      TS_ASSERT_DELTA( ws2->getDimension(d)->getMaximum(), 41.0, 1e-5);
    }
    std::vector<API::IMDNode *> boxes;
    ws2->getBox()->getBoxes(boxes, 1000, true);
    for (size_t i=0; i<boxes.size(); i++)
    {
      API::IMDNode * box = boxes[i];
      TSM_ASSERT_LESS_THAN( "Box extents was offset", 20.0, box->getExtents(0).getMin() );
      // More detailed tests are in MDBox, MDBoxBase and MDGridBox.
    }

    // Clean up files
    std::string file1;
    std::string file2;
    if (fileBacked)
    {
      file1 = ws1->getBoxController()->getFileIO()->getFileName();
      file2 = ws2->getBoxController()->getFileIO()->getFileName();
    }
    ws1->clearFileBacked(false);
    ws2->clearFileBacked(false);

    MDEventsTestHelper::checkAndDeleteFile(file1);
    MDEventsTestHelper::checkAndDeleteFile(file2);

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
    TS_ASSERT_THROWS_NOTHING( ws2 = AnalysisDataService::Instance().retrieveWS<MDHistoWorkspace>(outWSName) );
    TS_ASSERT(ws2); if (!ws2) return;

    for (size_t d=0; d<2; d++)
    {
      TS_ASSERT_DELTA( ws2->getDimension(d)->getMinimum(), 21.0, 1e-5);
      TS_ASSERT_DELTA( ws2->getDimension(d)->getMaximum(), 41.0, 1e-5);
    }

    TS_ASSERT_DELTA( ws2->getInverseVolume(), 1./16., 1e-6);
    coord_t point[2] = {21.1f, 21.1f};
    TS_ASSERT_DELTA( ws2->getSignalAtCoord(point, Mantid::API::NoNormalization), 1.23, 1e-6);


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
