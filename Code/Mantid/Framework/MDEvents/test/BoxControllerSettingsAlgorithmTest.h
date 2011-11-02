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
  void test_initProps()
  {
    BoxControllerSettingsAlgorithmImpl alg;
    TSM_ASSERT_THROWS_NOTHING("Can init properties", alg.initBoxControllerProps());
  }
  
  void test_one_SplitInto()
  {
    //TODO:
  }


};


#endif /* MANTID_MDEVENTS_BOXCONTROLLERSETTINGSALGORITHMTEST_H_ */

