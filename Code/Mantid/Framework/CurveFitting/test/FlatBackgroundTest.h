#ifndef MANTID_CURVEFITTING_FLATBACKGROUNDTEST_H_
#define MANTID_CURVEFITTING_FLATBACKGROUNDTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidCurveFitting/FlatBackground.h"

using namespace Mantid;
using namespace Mantid::CurveFitting;
using namespace Mantid::API;

class FlatBackgroundTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FlatBackgroundTest *createSuite() { return new FlatBackgroundTest(); }
  static void destroySuite( FlatBackgroundTest *suite ) { delete suite; }


  void testFunctionMW()
  {
    std::size_t numPoints = 100;
    double expValue = 10.;
    double yValues[numPoints];

    FlatBackground *bkgd = new FlatBackground();
    bkgd->initialize();
    bkgd->setParameter("A0", expValue);
    bkgd->functionMW(yValues, NULL, numPoints); // don't need x-values
    delete bkgd;

    for (std::size_t i = 0; i < numPoints; i++)
    {
      TS_ASSERT_EQUALS(yValues[i], expValue);
    }
  }

  void testForCategories()
  {
    FlatBackground forCat;
    const std::vector<std::string> categories = forCat.categories();
    TS_ASSERT( categories.size() == 1 );
    TS_ASSERT( categories[0] == "Background" );
  }

};


#endif /* MANTID_CURVEFITTING_FLATBACKGROUNDTEST_H_ */
