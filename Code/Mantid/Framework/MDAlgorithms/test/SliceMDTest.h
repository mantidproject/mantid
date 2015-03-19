#ifndef MANTID_MDEVENTS_SLICEMDTEST_H_
#define MANTID_MDEVENTS_SLICEMDTEST_H_

#include "MantidAPI/FrameworkManager.h"
#include "MantidMDAlgorithms/SliceMD.h"
#include "MantidDataObjects/CoordTransformAffine.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::MDAlgorithms;

using Mantid::coord_t;

class SliceMDTest : public CxxTest::TestSuite
{

private:

  /// Helper method to verify the existance of a given property name on the algorithm
  void doTestPropertyExistance(const std::string& propertyName)
  {
    SliceMD alg;
    alg.initialize();

    TSM_ASSERT_THROWS_NOTHING("Property name has changed. This has broken SlicingAlgorithmDialog. Fix SlicingAlgorithmDialog!", alg.getProperty(propertyName));
  }

  /// Helper method to test application of the recusion depth
  void doTestRecursionDepth(const bool bTakeDepthFromInput, const int maxDepth = 0)
  {
    SliceMD alg;
    alg.initialize();

    IMDEventWorkspace_sptr in_ws = MDEventsTestHelper::makeAnyMDEW<MDEvent<3>,3>(10, 0.0, 10.0, 1);
    AnalysisDataService::Instance().addOrReplace("SliceMDTest_ws", in_ws);

    alg.setPropertyValue("InputWorkspace", "SliceMDTest_ws");
    alg.setPropertyValue("AlignedDim0", "Axis0,2.0,8.0, 3");
    alg.setPropertyValue("AlignedDim1", "Axis1,2.0,8.0, 3");
    alg.setPropertyValue("AlignedDim2", "Axis2,2.0,8.0, 3");
    alg.setRethrows(true);
    alg.setPropertyValue("OutputWorkspace", "SliceMDTest_outWS");

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TakeMaxRecursionDepthFromInput", bTakeDepthFromInput));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaxRecursionDepth", maxDepth));
    alg.execute(); 
    TS_ASSERT(alg.isExecuted());

    IMDEventWorkspace_sptr out = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>("SliceMDTest_outWS");
    
    /*
    Run some verifications according to whether TakeMaxRecursionDepthFromInput was chosen.
    */
    Mantid::Kernel::Property* p = alg.getProperty("MaxRecursionDepth");
    if(bTakeDepthFromInput)
    {
      TSM_ASSERT_EQUALS("MaxRecusionDepth property should NOT be enabled", false, p->getSettings()->isEnabled(&alg));
      TSM_ASSERT_EQUALS("Should have passed the maxium depth onto the ouput workspace.", in_ws->getBoxController()->getMaxDepth(), out->getBoxController()->getMaxDepth());
    }
    else
    {
      TSM_ASSERT_EQUALS("MaxRecusionDepth property should be enabled", true,  p->getSettings()->isEnabled(&alg));
      TSM_ASSERT_EQUALS("Should have passed the maxium depth onto the ouput workspace from the input workspace.", size_t(maxDepth), out->getBoxController()->getMaxDepth());
    }
    
    //Clean up test objects
    AnalysisDataService::Instance().remove("SliceMDTest_ws");
    AnalysisDataService::Instance().remove("SliceMDTest_outWS");
  }


public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SliceMDTest *createSuite() { return new SliceMDTest(); }
  static void destroySuite( SliceMDTest *suite ) { delete suite; }


  void test_Init()
  {
    SliceMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

    //DO NOT DISABLE THIS TEST
  void testAlgorithmProperties()
  {
    /*
    This unit test is designed to flag up changes here. If property values and names here do need to be changed, 
    1) They must also be updated in SlicingAlgorithmDialog.cpp. 
    2) It should then be confirmed that that the SliceMD dialog still works in MantidPlot (run SliceMD)!
    3) Finally this unit test should be updated so that the tests pass.
    */
    doTestPropertyExistance("InputWorkspace");
    doTestPropertyExistance("AxisAligned");
    doTestPropertyExistance("OutputWorkspace");
    doTestPropertyExistance("OutputExtents");
    doTestPropertyExistance("OutputBins");
    doTestPropertyExistance("NormalizeBasisVectors");
    doTestPropertyExistance("ForceOrthogonal");
    doTestPropertyExistance("Translation");
    doTestPropertyExistance("AlignedDim0");
    doTestPropertyExistance("AlignedDim1");
    doTestPropertyExistance("BasisVector0");
    doTestPropertyExistance("BasisVector1");
    doTestPropertyExistance("MaxRecursionDepth");
    doTestPropertyExistance("TakeMaxRecursionDepthFromInput");
    doTestPropertyExistance("Memory");
    doTestPropertyExistance("OutputFilename");
  }
  

  /** Test the algo
  * @param nameX : name of the axis
  * @param expectedNumPoints :: how many points in the output
  */
  template<typename MDE, size_t nd>
  void do_test_exec(std::string name1, std::string name2, std::string name3, std::string name4,
      uint64_t expectedNumPoints, size_t expectedNumDims,
      bool willFail = false,
      std::string OutputFilename = "")
  {
   

    SliceMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )

    IMDEventWorkspace_sptr in_ws = MDEventsTestHelper::makeAnyMDEW<MDE,nd>(10, 0.0, 10.0, 1);
    Mantid::Kernel::SpecialCoordinateSystem appliedCoord = Mantid::Kernel::QSample;
    in_ws->setCoordinateSystem(appliedCoord);
    AnalysisDataService::Instance().addOrReplace("SliceMDTest_ws", in_ws);

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "SliceMDTest_ws") );
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("AlignedDim0", name1));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("AlignedDim1", name2));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("AlignedDim2", name3));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("AlignedDim3", name4));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "SliceMDTest_outWS"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputFilename", OutputFilename));


    TS_ASSERT_THROWS_NOTHING( alg.execute(); )

    if (willFail)
    {
      TS_ASSERT( !alg.isExecuted() );
      return;
    }
    else
      TS_ASSERT( alg.isExecuted() );

    IMDEventWorkspace_sptr out ;
    TS_ASSERT_THROWS_NOTHING( out = boost::dynamic_pointer_cast<IMDEventWorkspace>(
        AnalysisDataService::Instance().retrieve("SliceMDTest_outWS")); )
    TS_ASSERT(out);
    if(!out) return;

    TSM_ASSERT_EQUALS("Should default to TakeMaxRecursionDepthFromInput == true", in_ws->getBoxController()->getMaxDepth(), out->getBoxController()->getMaxDepth());

    TS_ASSERT_EQUALS(appliedCoord, out->getSpecialCoordinateSystem());

    // Took this many events out with the slice
    TS_ASSERT_EQUALS(out->getNPoints(), expectedNumPoints);
    // Output has this number of dimensions
    TS_ASSERT_EQUALS(out->getNumDims(), expectedNumDims);

    // Clean up file
    out->clearFileBacked(false);

    MDEventsTestHelper::checkAndDeleteFile(alg.getPropertyValue("OutputFilename"));

    AnalysisDataService::Instance().remove("SliceMDTest_ws");
    AnalysisDataService::Instance().remove("SliceMDTest_outWS");
  }


  void test_exec_3D_lean()
  { do_test_exec<MDLeanEvent<3>,3>("Axis0,2.0,8.0, 3", "Axis1,2.0,8.0, 3", "Axis2,2.0,8.0, 3", "",  6*6*6 /*# of events*/, 3 /*dims*/);
  }

  void test_exec_3D_lean_scrambled()
  { do_test_exec<MDLeanEvent<3>,3>("Axis2,2.0,8.0, 3", "Axis1,2.0,8.0, 3", "Axis0,2.0,8.0, 3", "",  6*6*6 /*# of events*/, 3 /*dims*/);
  }

  void test_exec_2D_lean()
  { do_test_exec<MDLeanEvent<3>,3>("Axis0,2.0,8.0, 3", "Axis1,2.0,8.0, 3", "", "",  6*6*10 /*# of events*/, 2 /*dims*/);
  }

  void test_exec_1D_lean()
  { do_test_exec<MDLeanEvent<3>,3>("Axis0,2.0,8.0, 3", "", "", "",  6*10*10 /*# of events*/, 1 /*dims*/);
  }


  void test_exec_3D()
  { do_test_exec<MDEvent<3>,3>("Axis0,2.0,8.0, 3", "Axis1,2.0,8.0, 3", "Axis2,2.0,8.0, 3", "",  6*6*6 /*# of events*/, 3 /*dims*/);
  }

  void test_exec_4D_to_4D()
  { do_test_exec<MDEvent<4>,4>("Axis0,2.0,8.0, 3", "Axis1,2.0,8.0, 3", "Axis2,2.0,8.0, 3", "Axis3,2.0,8.0, 3",  6*6*6*6 /*# of events*/, 4 /*dims*/);
  }

  void test_exec_4D_to_1D()
  { do_test_exec<MDEvent<4>,4>("Axis0,2.0,8.0, 3", "", "", "",  6*10*10*10 /*# of events*/, 1 /*dims*/);
  }


  void test_exec_3D_fileBackedOutput()
  { 
    do_test_exec<MDEvent<3>,3>("Axis0,2.0,8.0, 3", "Axis1,2.0,8.0, 3", "Axis2,2.0,8.0, 3", "",  6*6*6 /*# of events*/, 3 /*dims*/, false /*WillFail*/, "SliceMDTest_output.nxs");
  }

  void test_dont_use_max_recursion_depth()
  {
    bool bTakeDepthFromInput = true;
    doTestRecursionDepth(bTakeDepthFromInput);
  }

  void test_max_recursion_depth()
  {
    bool bTakeDepthFromInput = false;
    doTestRecursionDepth(bTakeDepthFromInput, 4);
    //Test with another recursion depth just to make sure that there's nothing hard-coded.
    doTestRecursionDepth(bTakeDepthFromInput, 5);
  }

  /** Test the algorithm, with a coordinate transformation.
   *
  * @param lengthX : length to keep in each direction
  * @param expected_signal :: how many events in each resulting bin
  * @param expected_numBins :: how many points/bins in the output
  */
  void do_test_transform(double lengthX,double lengthY,double lengthZ,
      size_t expected_numBins)
  {
    SliceMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )

    // Make a workspace with events along a regular grid that is rotated and offset along x,y
    MDEventWorkspace3Lean::sptr in_ws = MDEventsTestHelper::makeMDEW<3>(10, -10.0, 20.0, 0);
    in_ws->splitBox();
    double theta = 0.1;
    VMD origin(-2.0, -3.0, -4.0);
    for (coord_t ox=0.5; ox<10; ox++)
      for (coord_t oy=0.5; oy<10; oy++)
        for (coord_t oz=0.5; oz<10; oz++)
        {
          double x = ox*cos(theta) - oy*sin(theta) + origin[0];
          double y = oy*cos(theta) + ox*sin(theta) + origin[1];
          double z = oz + origin[2];
          double center[3] = {x,y,z};
          MDLeanEvent<3> ev(1.0, 1.0, center);
//          std::cout << x << "," << y << "," << z << std::endl;
          in_ws->addEvent(ev);
        }
    in_ws->refreshCache();


    // Build the transformation (from eventWS to binned workspace)
    CoordTransformAffine ct(3,3);

    // Build the basis vectors, a 0.1 rad rotation along +Z
    double angle = 0.1;
    VMD baseX(cos(angle), sin(angle), 0.0);
    VMD baseY(-sin(angle), cos(angle), 0.0);
    VMD baseZ(0.0, 0.0, 1.0);

    AnalysisDataService::Instance().addOrReplace("SliceMDTest_ws", in_ws);
    if (false)
    {
      // Save to NXS file for debugging
      IAlgorithm_sptr saver = FrameworkManager::Instance().exec("SaveMD", 4,
          "InputWorkspace", "SliceMDTest_ws",
          "Filename", "SliceMDTest_ws_rotated.nxs");
    }

    // 1000 boxes with 1 event each
    TS_ASSERT_EQUALS( in_ws->getNPoints(), 1000);

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "SliceMDTest_ws") );
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("AxisAligned", false));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("BasisVector0", "OutX,m," + baseX.toString(",") ));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("BasisVector1", "OutY,m," + baseY.toString(",") ));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("BasisVector2", "OutZ,m," + baseZ.toString(",") ));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("BasisVector3", ""));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Translation", origin.toString(",") ));

    std::vector<double> OutputExtents;
    OutputExtents.push_back(0); OutputExtents.push_back(lengthX);
    OutputExtents.push_back(0); OutputExtents.push_back(lengthY);
    OutputExtents.push_back(0); OutputExtents.push_back(lengthZ);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputExtents", OutputExtents));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputBins", "3,3,3"));

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "SliceMDTest_outWS"));

    TS_ASSERT_THROWS_NOTHING( alg.execute(); )
    TS_ASSERT( alg.isExecuted() );

    IMDEventWorkspace_sptr out ;
    TS_ASSERT_THROWS_NOTHING( out = boost::dynamic_pointer_cast<IMDEventWorkspace>(
        AnalysisDataService::Instance().retrieve("SliceMDTest_outWS")); )
    TS_ASSERT(out);
    if(!out) return;

    // # of events left
    TS_ASSERT_EQUALS(out->getNPoints(), expected_numBins);

    AnalysisDataService::Instance().remove("SliceMDTest_outWS");

  }


  void test_exec_with_transform()
  {
    do_test_transform(10, 10, 10,
        1000 /*# of events*/);
  }

  void test_exec_with_transform_unevenSizes()
  {
    do_test_transform(5, 10, 2,
        100 /*# of events*/);
  }


};


#endif /* MANTID_MDEVENTS_SLICEMDTEST_H_ */

