#ifndef MANTID_CURVEFITTING_QUADRATICBACKGROUNDTEST_H_
#define MANTID_CURVEFITTING_QUADRATICBACKGROUNDTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidCurveFitting/QuadraticBackground.h"

using namespace Mantid;
using namespace Mantid::CurveFitting;
using namespace Mantid::API;

class QuadraticBackgroundTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static QuadraticBackgroundTest *createSuite() { return new QuadraticBackgroundTest(); }
  static void destroySuite( QuadraticBackgroundTest *suite ) { delete suite; }


  void test_Something()
  {
  }

    
  void testForCategories()
  {
    QuadraticBackground forCat;
    const std::vector<std::string> categories = forCat.categories();
    TS_ASSERT( categories.size() == 1 );
    TS_ASSERT( categories[0] == "Background" );
  }


};


#endif /* MANTID_CURVEFITTING_QUADRATICBACKGROUNDTEST_H_ */

