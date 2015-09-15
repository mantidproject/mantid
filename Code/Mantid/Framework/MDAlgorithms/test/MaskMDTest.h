#ifndef MANTID_MDALGORITHMS_MASKMDTEST_H_
#define MANTID_MDALGORITHMS_MASKMDTEST_H_

#include "MantidMDAlgorithms/MaskMD.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::MDAlgorithms;

class MaskMDTest : public CxxTest::TestSuite
{
private:

  void do_exec(const std::string& dimensionString, const std::string& extentsString, size_t expectedNMasked)
  {
    std::string wsName = "test_workspace";
    MDEventsTestHelper::makeAnyMDEW<MDEvent<3>, 3>(10, 0, 10, 1, wsName);
   
    MaskMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Workspace", wsName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Dimensions", dimensionString) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Extents", extentsString) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    // Retrieve the workspace from data service. TODO: Change to your desired type
    IMDWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<IMDWorkspace>(wsName) );
    TS_ASSERT(ws);
    if (!ws) return;
    
    IMDIterator* it = ws->createIterator();
    size_t nMasked = 0;
    for(size_t i = 0; i < it->getDataSize(); ++i)
    {
      if(it->getIsMasked())
      {
        ++nMasked;
      }
      it->next(1);
    }
    
    TSM_ASSERT_EQUALS("The number actually masked is different from the expected value", expectedNMasked, nMasked);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(wsName);
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MaskMDTest *createSuite() { return new MaskMDTest(); }
  static void destroySuite( MaskMDTest *suite ) { delete suite; }


  void test_Init()
  {
    MaskMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_default_clear()
  {
    MaskMD alg;
    alg.initialize();
    bool clearBeforeExecution = alg.getProperty("ClearExistingMasks");
    TSM_ASSERT("Should clear before execution by default.", clearBeforeExecution);
  }

  void test_set_to_clear()
  {
    MaskMD alg;
    alg.initialize();
    alg.setProperty("ClearExistingMasks", false);
    bool clearBeforeExecution = alg.getProperty("ClearExistingMasks");
    TSM_ASSERT("Check setter is working.", !clearBeforeExecution);
  }

  void test_throw_if_dimension_cardinality_wrong()
  {
    std::string wsName = "test_workspace";
    MDEventsTestHelper::makeAnyMDEW<MDEvent<3>, 3>(10, 0, 10, 1, wsName);
   
    MaskMD alg;
    alg.initialize();
    alg.setRethrows(true);
    alg.setPropertyValue("Workspace", wsName);
    alg.setPropertyValue("Dimensions", "Axis0, Axis1");//wrong number of dimenion ids provided.
    alg.setPropertyValue("Extents", "0,10,0,10,0,10");
    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument );
  }

  void test_throw_if_extent_cardinality_wrong()
  {
    std::string wsName = "test_workspace";
    MDEventsTestHelper::makeAnyMDEW<MDEvent<3>, 3>(10, 0, 10, 1, wsName);
   
    MaskMD alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setPropertyValue("Workspace", wsName);
    alg.setPropertyValue("Dimensions", "Axis0, Axis1, Axis2");
    alg.setPropertyValue("Extents", "0,10");//wrong number of extents provided.
    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument );
  }

  void test_throw_if_min_greater_than_max_anywhere()
  {
    std::string wsName = "test_workspace";
    MDEventsTestHelper::makeAnyMDEW<MDEvent<3>, 3>(10, 0, 10, 1, wsName);
   
    MaskMD alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setPropertyValue("Workspace", wsName);
    alg.setPropertyValue("Dimensions", "Axis0, Axis1, Axis2");
    alg.setPropertyValue("Extents", "0,-10,0,-10,0,-10");
    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument );
  }

  void test_fall_back_to_dimension_names()
  {
    std::string wsName = "test_workspace";
    MDEventsTestHelper::makeAnyMDEW<MDEvent<3>, 3>(10, 0, 10, 1, wsName, "AxisName%d"); //Dimension names = AxisName%d, default dimension ids are AxisId%d
   
    MaskMD alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setPropertyValue("Workspace", wsName);
    alg.setPropertyValue("Dimensions", "AxisName0, Axis1, Axis2"); //Use dimenion name for one of the dimensions.
    alg.setPropertyValue("Extents", "0,10,0,10,0,10");
    TSM_ASSERT_THROWS_NOTHING("Should be okay to use either dimension names or ids.", alg.execute());
  }

  void test_throws_if_unknown_dimension_names()
  {
    std::string wsName = "test_workspace";
    MDEventsTestHelper::makeAnyMDEW<MDEvent<3>, 3>(10, 0, 10, 1, wsName);
   
    MaskMD alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setPropertyValue("Workspace", wsName);
    alg.setPropertyValue("Dimensions", "UnknownId, Axis1, Axis2");
    alg.setPropertyValue("Extents", "0,10,0,10,0,10");
    TSM_ASSERT_THROWS("Using an unknown name/id should throw", alg.execute(), std::runtime_error);
  }

  void test_mask_everything()
  {
    //Implicit function should cover total extents. Expect all (10*10*10) to be masked.
    do_exec("Axis0,Axis1,Axis2", "0,10,0,10,0,10", 1000);
  }  
  
  void test_mask_nothing()
  {
    //No intersection between implicit function and workspace.
    do_exec("Axis0,Axis1,Axis2", "-1,-0.1,-1,-0.1,-1,-0.1", 0);
  }

  void test_mask_half()
  {
    do_exec("Axis0,Axis1,Axis2", "0,10,0,10,0,4.99", 500);
  }  
  
  //Test resilience to mixing up input order.
  void test_mask_everything_mix_up_input_order()
  {
    do_exec("Axis1,Axis2,Axis0", "0,10,0,4.99,0,10", 500);
  }

  void test_multiple_mask()
  {
    //Mask out 3*3*3 twice.
    do_exec("Axis0, Axis1, Axis2, Axis0, Axis1, Axis2", "0,2.99,0,2.99,0,2.99,7.01,10,7.01,10,7.01,10", 2*(3*3*3));
  }


};


#endif /* MANTID_MDALGORITHMS_MASKMDTEST_H_ */
