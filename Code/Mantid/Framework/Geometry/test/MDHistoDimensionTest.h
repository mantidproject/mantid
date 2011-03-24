#ifndef MANTID_GEOMETRY_MDHISTODIMENSIONTEST_H_
#define MANTID_GEOMETRY_MDHISTODIMENSIONTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidGeometry/MDGeometry/MDHistoDimension.h"

using namespace Mantid::Geometry;

class MDHistoDimensionTest : public CxxTest::TestSuite
{
public:

  void test_constructor()
  {
    MDHistoDimension d("name", "id", -10, 20.0, 15);
    TS_ASSERT_EQUALS(d.getName(), "name");
    TS_ASSERT_EQUALS(d.getDimensionId(), "id");
    TS_ASSERT_EQUALS(d.getMinimum(), -10);
    TS_ASSERT_EQUALS(d.getMaximum(), +20);
    TS_ASSERT_EQUALS(d.getNBins(), 15);
  }


};


#endif /* MANTID_GEOMETRY_MDHISTODIMENSIONTEST_H_ */

