#ifndef MANTID_MDEVENTS_REFLECTOMETRYTRANSFORMPTEST_H_
#define MANTID_MDEVENTS_REFLECTOMETRYTRANSFORMPTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidMDEvents/ReflectometryTransformP.h"

//using namespace Mantid;
//using namespace Mantid::MDEvents;
//using namespace Mantid::API;

class ReflectometryTransformPTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflectometryTransformPTest *createSuite() { return new ReflectometryTransformPTest(); }
  static void destroySuite( ReflectometryTransformPTest *suite ) { delete suite; }


  void test_Something()
  {
    //TSM_ASSERT( "You forgot to write a test!", 0);
  }


};


#endif /* MANTID_MDEVENTS_REFLECTOMETRYTRANSFORMPTEST_H_ */