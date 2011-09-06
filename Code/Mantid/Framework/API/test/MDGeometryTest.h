#ifndef MANTID_API_MDGEOMETRYTEST_H_
#define MANTID_API_MDGEOMETRYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidAPI/MDGeometry.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::API;

class MDGeometryTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MDGeometryTest *createSuite() { return new MDGeometryTest(); }
  static void destroySuite( MDGeometryTest *suite ) { delete suite; }


  void test_Something()
  {
  }


};


#endif /* MANTID_API_MDGEOMETRYTEST_H_ */

