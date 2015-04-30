#ifndef MANTID_MDEVENTS_CLONEMDEVENTWORKSPACETEST_H_
#define MANTID_MDEVENTS_CLONEMDEVENTWORKSPACETEST_H_

#include "LoadMDTest.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidMDAlgorithms/CloneMDWorkspace.h"
#include "MantidTestHelpers/MDAlgorithmsTestHelper.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"

#include <cxxtest/TestSuite.h>

#include <Poco/File.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::MDAlgorithms;

class CloneMDWorkspaceTest : public CxxTest::TestSuite
{
public:

    
  void test_Init()
  {
    CloneMDWorkspace alg;
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

  void test_exec_FileBacked_withNeedsUpdating()
  {
    do_test(true, "", true);
  }

  void test_exec_FileBacked_withFilename()
  {
    do_test(true, "CloneMDWorkspaceTest_ws_custom_cloned_name.nxs");
  }

  void test_exec_FileBacked_withFilename_withNeedsUpdating()
  {
    do_test(true, "CloneMDWorkspaceTest_ws_custom_cloned_name2.nxs", true);
  }


  void do_test(bool fileBacked, std::string Filename = "", bool file_needs_updating = false)
  {
    // Name of the output workspace.
    std::string outWSName("CloneMDWorkspaceTest_OutputWS");

    // Make a fake file-backed (or not) MDEW
    MDEventWorkspace3Lean::sptr ws1 = MDAlgorithmsTestHelper::makeFileBackedMDEW("CloneMDWorkspaceTest_ws", fileBacked);
    ws1->setFileNeedsUpdating( file_needs_updating );
  
    CloneMDWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", "CloneMDWorkspaceTest_ws") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", Filename) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    // Retrieve the workspace from data service.
    MDEventWorkspace3Lean::sptr ws2;
    TS_ASSERT_THROWS_NOTHING( ws2 = AnalysisDataService::Instance().retrieveWS<MDEventWorkspace3Lean>(outWSName) );
    TS_ASSERT(ws2); if (!ws2) return;
    
    // Compare the two workspaces
    LoadMDTest::do_compare_MDEW(ws1, ws2);
    
    // Check that the custom file name file exists
    std::string file1;
    std::string file2;
    if (fileBacked)// && !Filename.empty())
    {
      if (!Filename.empty())
      {
        file1 = alg.getPropertyValue("Filename");
        TS_ASSERT( Poco::File( file1 ).exists() );
        file2 = ws1->getBoxController()->getFileIO()->getFileName();
      }
      else
      {
        file1 = ws1->getBoxController()->getFileIO()->getFileName();
        file2 = ws2->getBoxController()->getFileIO()->getFileName();
      }
    }

    // Clean up files
    ws1->clearFileBacked(false);
    ws2->clearFileBacked(false);

    // Modifying the cloned dimension does not change the original
    double oldMin = ws1->getDimension(0)->getMinimum();
    std::vector<double> scaling(ws1->getNumDims(), 20.0);
    std::vector<double> offset(ws1->getNumDims(), 1.0);
    ws2->transformDimensions(scaling, offset);
    TS_ASSERT_DELTA( ws1->getDimension(0)->getMinimum(), oldMin, 1e-5);
    TSM_ASSERT_DELTA("Dimensions of the cloned WS are deep copies of the original.", ws2->getDimension(0)->getMinimum(), oldMin*20.0+1.0, 1e-5);

    MDEventsTestHelper::checkAndDeleteFile(file1);
    MDEventsTestHelper::checkAndDeleteFile(file2);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove("CloneMDWorkspaceTest_ws");
    AnalysisDataService::Instance().remove(outWSName);
  }

  /** Clone a workspace and check that the clone matches */
  void do_test_MDHisto(MDHistoWorkspace_sptr ws1)
  {
    // Name of the output workspace.
    std::string outWSName("CloneMDWorkspaceTest_OutputWS");
    // Add the input workspace
    AnalysisDataService::Instance().addOrReplace("CloneMDWorkspaceTest_ws", ws1);

    CloneMDWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", "CloneMDWorkspaceTest_ws") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    // Retrieve the workspace from data service.
    MDHistoWorkspace_sptr ws2;
    TS_ASSERT_THROWS_NOTHING( ws2 = AnalysisDataService::Instance().retrieveWS<MDHistoWorkspace>(outWSName) );
    TS_ASSERT(ws2); if (!ws2) return;

    // Compare the WS
    TS_ASSERT_EQUALS( ws1->getNumDims(), ws2->getNumDims());
    TS_ASSERT_EQUALS( ws1->getNPoints(), ws2->getNPoints());
    TS_ASSERT_DELTA( ws1->getSignalAt(0), ws2->getSignalAt(0), 1e-5);
    TS_ASSERT_DELTA( ws1->getErrorAt(0), ws2->getErrorAt(0), 1e-5);

    if (ws1->getNPoints() == ws2->getNPoints())
    {
      for (size_t i=0; i < ws1->getNPoints(); i++)
      {
        TS_ASSERT_DELTA( ws1->getSignalAt(i), ws2->getSignalAt(i), 1e-5);
        TS_ASSERT_DELTA( ws1->getErrorAt(i), ws2->getErrorAt(i), 1e-5);
        TS_ASSERT_DELTA( ws1->getSignalNormalizedAt(i), ws2->getSignalNormalizedAt(i), 1e-5);
      }
    }

    for (size_t d=0; d<ws1->getNumDims(); d++)
    {
      TS_ASSERT_EQUALS( ws1->getDimension(d)->getName(), ws2->getDimension(d)->getName());
      TS_ASSERT_EQUALS( ws1->getDimension(d)->getNBins(), ws2->getDimension(d)->getNBins());
    }

    // Modifying the cloned dimension does not change the original
    double oldMin = ws1->getDimension(0)->getMinimum();
    std::vector<double> scaling(ws1->getNumDims(), 20.0);
    std::vector<double> offset(ws1->getNumDims(), 1.0);
    ws2->transformDimensions(scaling, offset);
    TS_ASSERT_DELTA( ws1->getDimension(0)->getMinimum(), oldMin, 1e-5);
    TS_ASSERT_DELTA( ws2->getDimension(0)->getMinimum(), oldMin*20.0+1.0, 1e-5);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove("CloneMDWorkspaceTest_ws");
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_MDHistoWorkspace_1D()
  {
    MDHistoWorkspace_sptr ws1 = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.23, 1, 5, 10.0, 2.34);
    do_test_MDHisto(ws1);
  }

  void test_MDHistoWorkspace_2D()
  {
    // Make a fake file-backed (or not) MDEW
    MDHistoWorkspace_sptr ws1 = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.23, 2, 5, 10.0, 2.34);
    do_test_MDHisto(ws1);
  }

  void test_MDHistoWorkspace_2D_uneven_bins()
  {
    // Make the number of bins uneven in both dimensions
    Mantid::DataObjects::MDHistoWorkspace * ws = NULL;
    ws = new Mantid::DataObjects::MDHistoWorkspace(
          MDHistoDimension_sptr(new MDHistoDimension("x","x","m", 0.0, 10.0, 50)),
          MDHistoDimension_sptr(new MDHistoDimension("y","y","m", 0.0, 10.0, 100))  );
    Mantid::DataObjects::MDHistoWorkspace_sptr ws1(ws);
    ws1->setTo(1.234, 5.678, 1.0);
    do_test_MDHisto(ws1);
  }

  

};


#endif /* MANTID_MDEVENTS_CLONEMDEVENTWORKSPACETEST_H_ */

