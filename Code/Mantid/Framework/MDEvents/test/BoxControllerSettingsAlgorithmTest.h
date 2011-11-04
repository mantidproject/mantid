#ifndef MANTID_MDEVENTS_BOXCONTROLLERSETTINGSALGORITHMTEST_H_
#define MANTID_MDEVENTS_BOXCONTROLLERSETTINGSALGORITHMTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidMDEvents/BoxControllerSettingsAlgorithm.h"

using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace Mantid::API;

//------------------------------------------------------------------------------------------------
/** Concrete declaration of BoxControllerSettingsAlgorithm for testing */
class BoxControllerSettingsAlgorithmImpl : public BoxControllerSettingsAlgorithm
{
  // Make all the members public so I can test them.
  friend class BoxControllerSettingsAlgorithmTest;
public:
  virtual const std::string name() const { return "BoxControllerSettingsAlgorithmImpl";};
  virtual int version() const { return 1;};
  virtual const std::string category() const { return "Testing";}
  void init() {}
  void exec() {}
};


class BoxControllerSettingsAlgorithmTest : public CxxTest::TestSuite
{
public:

  void test_defaultProps()
  {
    BoxControllerSettingsAlgorithmImpl alg;
    alg.initBoxControllerProps();
    BoxController_sptr bc(new BoxController(3));
    alg.setBoxController(bc);
    TS_ASSERT_EQUALS( bc->getSplitInto(0), 5 );
    TS_ASSERT_EQUALS( bc->getSplitThreshold(), 1000 );
    TS_ASSERT_EQUALS( bc->getMaxDepth(), 5 );
  }

  /** You can change the defaults given to the props */
  void test_initProps_otherDefaults()
  {
    BoxControllerSettingsAlgorithmImpl alg;
    alg.initBoxControllerProps("6", 1234, 34);
    BoxController_sptr bc(new BoxController(3));
    alg.setBoxController(bc);
    TS_ASSERT_EQUALS( bc->getSplitInto(0), 6 );
    TS_ASSERT_EQUALS( bc->getSplitThreshold(), 1234 );
    TS_ASSERT_EQUALS( bc->getMaxDepth(), 34 );
  }

  void doTest(BoxController_sptr bc,
      std::string SplitInto="", std::string SplitThreshold="", std::string MaxRecursionDepth="")
  {
    BoxControllerSettingsAlgorithmImpl alg;
    alg.initBoxControllerProps();
    if (!SplitInto.empty()) alg.setPropertyValue("SplitInto",SplitInto);
    if (!SplitThreshold.empty()) alg.setPropertyValue("SplitThreshold",SplitThreshold);
    if (!MaxRecursionDepth.empty()) alg.setPropertyValue("MaxRecursionDepth",MaxRecursionDepth);
    alg.setBoxController(bc);
  }



  void test_SplitInto()
  {
    BoxController_sptr bc(new BoxController(3));
    TSM_ASSERT_THROWS_ANYTHING("Too few parameters", doTest(bc, "5,5") );
    TSM_ASSERT_THROWS_ANYTHING("Too many parameters", doTest(bc, "1,2,3,4") );
    doTest(bc,"4");
    TS_ASSERT_EQUALS( bc->getSplitInto(2), 4 );
    doTest(bc,"7,6,5");
    TS_ASSERT_EQUALS( bc->getSplitInto(0), 7 );
    TS_ASSERT_EQUALS( bc->getSplitInto(1), 6 );
    TS_ASSERT_EQUALS( bc->getSplitInto(2), 5 );
  }

  void test_SplitThreshold()
  {
    BoxController_sptr bc(new BoxController(3));
    TSM_ASSERT_THROWS_ANYTHING("Negative threshold", doTest(bc, "", "-3") );
    doTest(bc,"", "1234");
    TS_ASSERT_EQUALS( bc->getSplitThreshold(), 1234 );
  }

  void test_MaxRecursionDepth()
  {
    BoxController_sptr bc(new BoxController(3));
    TSM_ASSERT_THROWS_ANYTHING("Negative MaxRecursionDepth", doTest(bc, "", "", "-1") );
    doTest(bc,"", "", "34");
    TS_ASSERT_EQUALS( bc->getMaxDepth(), 34 );
  }


};


#endif /* MANTID_MDEVENTS_BOXCONTROLLERSETTINGSALGORITHMTEST_H_ */

