// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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

  void test_throws_on_additional_input() {
    std::string tooLong("1/2x,-4y,-2-z,x");

    TS_ASSERT_THROWS(parseMatrixVectorPair<double>(tooLong),
                     const Exception::ParseError &);
  }

  void test_throws_on_short_input() {
    std::string tooShort("2/3x,-x+y");

    TS_ASSERT_THROWS(parseMatrixVectorPair<double>(tooShort),
                     const Exception::ParseError &);
  }

  void test_throws_on_empty_element() {
    std::string emptyY("2/3x, ,-x+y");

    TS_ASSERT_THROWS(parseMatrixVectorPair<double>(emptyY),
                     const Exception::ParseError &);
  }

  void test_throws_on_zero_division() {
    std::string zeroDivision("2/0x,-4y,-2-z,x");

    TS_ASSERT_THROWS(parseMatrixVectorPair<double>(zeroDivision),
                     const Exception::ParseError &);
  }

  void test_parse_many_directions() {
    std::string allowed("x+y+z-2/3z+1/6x,33z+4x-2-3-1/8y,y-y-1-z+x");

    MatrixVectorPair<double, V3R> pair = parseMatrixVectorPair<double>(allowed);

    DblMatrix matrix = pair.getMatrix();
    TS_ASSERT_EQUALS(matrix[0][0], 7.0 / 6.0);
    TS_ASSERT_EQUALS(matrix[0][1], 1.0);
    TS_ASSERT_EQUALS(matrix[0][2], 1.0 / 3.0);

    TS_ASSERT_EQUALS(matrix[1][0], 4.0);
    TS_ASSERT_EQUALS(matrix[1][1], -1.0 / 8.0);
    TS_ASSERT_EQUALS(matrix[1][2], 33.0);

    TS_ASSERT_EQUALS(matrix[2][0], 1.0);
    TS_ASSERT_EQUALS(matrix[2][1], 0.0);
    TS_ASSERT_EQUALS(matrix[2][2], -1.0);

    TS_ASSERT_EQUALS(pair.getVector(), V3R(0, -5, -1));
  }

  void test_parseResult() {
    std::string allowed("1/2x,-4y,-2-z");
    MatrixVectorPair<double, V3R> pair = parseMatrixVectorPair<double>(allowed);

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

  void test_zeros() {
    std::string allowed("0x,0-0y,0z+0");
    MatrixVectorPair<double, V3R> pair = parseMatrixVectorPair<double>(allowed);

    DblMatrix matrix = pair.getMatrix();
    TS_ASSERT_EQUALS(matrix[0][0], 0.0);
    TS_ASSERT_EQUALS(matrix[0][1], 0.0);
    TS_ASSERT_EQUALS(matrix[0][2], 0.0);

    TS_ASSERT_EQUALS(matrix[1][0], 0.0);
    TS_ASSERT_EQUALS(matrix[1][1], 0.0);
    TS_ASSERT_EQUALS(matrix[1][2], 0.0);

    TS_ASSERT_EQUALS(matrix[2][0], 0.0);
    TS_ASSERT_EQUALS(matrix[2][1], 0.0);
    TS_ASSERT_EQUALS(matrix[2][2], 0.0);

    TS_ASSERT_EQUALS(pair.getVector(), V3R(0, 0, 0));
  }
};

#endif /* MANTID_GEOMETRY_MATRIXVECTORPAIRPARSERTEST_H_ */
