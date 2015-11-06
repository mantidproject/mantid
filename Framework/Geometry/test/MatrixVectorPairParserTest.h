#ifndef MANTID_GEOMETRY_MATRIXVECTORPAIRPARSERTEST_H_
#define MANTID_GEOMETRY_MATRIXVECTORPAIRPARSERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/MatrixVectorPairParser.h"

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class MatrixVectorPairParserTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MatrixVectorPairParserTest *createSuite() {
    return new MatrixVectorPairParserTest();
  }
  static void destroySuite(MatrixVectorPairParserTest *suite) { delete suite; }

  void test_Construction() {
    TS_ASSERT_THROWS_NOTHING(MatrixVectorPairParser parser);
  }

  void test_throws_on_additional_input() {
    std::string tooLong("1/2x,-4y,-2-z,x");
    MatrixVectorPairParser parser;

    TS_ASSERT_THROWS(parser.parseMatrixVectorPair<double>(tooLong),
                     Exception::ParseError);
  }

  void test_parseResult() {
    std::string allowed("1/2x,-4y,-2-z");
    MatrixVectorPairParser parser;
    MatrixVectorPair<double, V3R> pair =
        parser.parseMatrixVectorPair<double>(allowed);

    DblMatrix matrix = pair.getMatrix();
    TS_ASSERT_EQUALS(matrix[0][0], 0.5);
    TS_ASSERT_EQUALS(matrix[0][1], 0.0);
    TS_ASSERT_EQUALS(matrix[0][2], 0.0);

    TS_ASSERT_EQUALS(matrix[1][0], 0.0);
    TS_ASSERT_EQUALS(matrix[1][1], -4.0);
    TS_ASSERT_EQUALS(matrix[1][2], 0.0);

    TS_ASSERT_EQUALS(matrix[2][0], 0.0);
    TS_ASSERT_EQUALS(matrix[2][1], 0.0);
    TS_ASSERT_EQUALS(matrix[2][2], -1.0);

    TS_ASSERT_EQUALS(pair.getVector(), V3R(0, 0, -2));
  }
};

#endif /* MANTID_GEOMETRY_MATRIXVECTORPAIRPARSERTEST_H_ */
