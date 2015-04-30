#ifndef MANTID_DATAOBJECTS_MDEVENTFACTORYTEST_H_
#define MANTID_DATAOBJECTS_MDEVENTFACTORYTEST_H_

#include <cxxtest/TestSuite.h>
#include <MantidKernel/Timer.h>
#include <MantidKernel/System.h>
#include <iostream>
#include <iomanip>

#include <MantidDataObjects/MDEventFactory.h>


using namespace Mantid::DataObjects;
using namespace Mantid::API;

class MDEventFactoryTest : public CxxTest::TestSuite
{
public:

  /** Create MDEW's with various number of dimensions */
  void test_factory()
  {
    IMDEventWorkspace_sptr ew;
    ew = MDEventFactory::CreateMDWorkspace(4, "MDLeanEvent");
    TS_ASSERT_EQUALS( ew->getNumDims(), 4);

    size_t n = 9;
    ew = MDEventFactory::CreateMDWorkspace(n);
    TS_ASSERT_EQUALS( ew->getNumDims(), n);

    TS_ASSERT_THROWS( ew = MDEventFactory::CreateMDWorkspace(0), std::invalid_argument);
  }


  void test_box_factory()
  {
      BoxController_sptr bc = boost::shared_ptr<BoxController>(new BoxController(4));

      IMDNode *Box = MDEventFactory::createBox(4,MDEventFactory::BoxType::MDBoxWithLean,bc);
      TS_ASSERT_EQUALS( Box->getNumDims(), 4);
      MDBox<MDLeanEvent<4>,4>* leanBox = dynamic_cast<MDBox<MDLeanEvent<4>,4>*>(Box);
      TS_ASSERT(leanBox!=NULL);
      delete Box;

      bc.reset(new BoxController(9));
      Box = MDEventFactory::createBox(9,MDEventFactory::BoxType::MDBoxWithFat,bc);
      TS_ASSERT_EQUALS( Box->getNumDims(), 9);
      MDBox<MDEvent<9>,9>* fatBox = dynamic_cast<MDBox<MDEvent<9>,9>*>(Box);
      TS_ASSERT(fatBox!=NULL);
      delete Box;



      bc.reset(new BoxController(3));
      Box = MDEventFactory::createBox(3,MDEventFactory::BoxType::MDGridBoxWithLean,bc);
      TS_ASSERT_EQUALS( Box->getNumDims(), 3);
      MDGridBox<MDLeanEvent<3>,3>* leanGridBox = dynamic_cast<MDGridBox<MDLeanEvent<3>,3>*>(Box);
      TS_ASSERT(leanGridBox!=NULL);
      delete Box;

      bc.reset(new BoxController(1));
      Box = MDEventFactory::createBox(1,MDEventFactory::BoxType::MDGridBoxWithFat,bc);
      TS_ASSERT_EQUALS( Box->getNumDims(), 1);
      MDGridBox<MDEvent<1>,1>* fatGridBox = dynamic_cast<MDGridBox<MDEvent<1>,1>*>(Box);
      TS_ASSERT(fatGridBox!=NULL);
      delete Box;

      TS_ASSERT_THROWS(MDEventFactory::createBox(0,MDEventFactory::BoxType::MDBoxWithLean,bc), std::invalid_argument);
      TS_ASSERT_THROWS(MDEventFactory::createBox(10,MDEventFactory::BoxType::MDGridBoxWithFat,bc), std::invalid_argument);
  }


  // Templated function that will be called for a specific MDEW
  template<typename MDE, size_t nd>
  void functionTest(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    test_value = ws->getNumDims();
  }

  void test_CALL_MDEVENT_FUNCTION_macro()
  {
    IMDEventWorkspace_sptr ew(new MDEventWorkspace<MDLeanEvent<1>, 1>() );
    TS_ASSERT_EQUALS( ew->getNumDims(), 1);
    TS_ASSERT_EQUALS( ew->getNPoints(), 0);
    test_value = 0;
    CALL_MDEVENT_FUNCTION( functionTest, ew );
    TS_ASSERT_EQUALS( test_value, 1 );
  }

  void test_CALL_MDEVENT_FUNCTION_macro_2()
  {
    IMDEventWorkspace_sptr ew(new MDEventWorkspace<MDLeanEvent<8>, 8>() );
    TS_ASSERT_EQUALS( ew->getNumDims(), 8);
    TS_ASSERT_EQUALS( ew->getNPoints(), 0);
    test_value = 0;
    CALL_MDEVENT_FUNCTION( functionTest, ew );
    TS_ASSERT_EQUALS( test_value, 8 );
  }


  size_t test_value;

};


#endif /* MANTID_DATAOBJECTS_MDEVENTFACTORYTEST_H_ */

