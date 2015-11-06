#ifndef MANTID_GEOMETRY_MATRIXVECTORPAIRTEST_H_
#define MANTID_GEOMETRY_MATRIXVECTORPAIRTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/MatrixVectorPair.h"
#include "MantidGeometry/Crystal/V3R.h"
#include "MantidKernel/Matrix.h"

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class MatrixVectorPairTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MatrixVectorPairTest *createSuite() {
    return new MatrixVectorPairTest();
  }
  static void destroySuite(MatrixVectorPairTest *suite) { delete suite; }

  void test_Construction() {
    MatrixVectorPair<int, V3R> pair(IntMatrix(3, 3, true), V3R(1, 1, 1));
  }
};

#endif /* MANTID_GEOMETRY_MATRIXVECTORPAIRTEST_H_ */
