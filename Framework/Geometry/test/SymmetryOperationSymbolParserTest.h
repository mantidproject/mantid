// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_SYMMETRYOPERATIONSYMBOLPARSERTEST_H_
#define MANTID_GEOMETRY_SYMMETRYOPERATIONSYMBOLPARSERTEST_H_

#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/SymmetryOperationSymbolParser.h"

using Mantid::Geometry::SymmetryOperationSymbolParser;
using namespace Mantid::Geometry;

class SymmetryOperationSymbolParserTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SymmetryOperationSymbolParserTest *createSuite() {
    return new SymmetryOperationSymbolParserTest();
  }
  static void destroySuite(SymmetryOperationSymbolParserTest *suite) {
    delete suite;
  }

  void testParseIdentifier() {
    TS_ASSERT_THROWS_NOTHING(
        TestableSymmetryOperationSymbolParser::parseIdentifier("x, y, z"));
    TS_ASSERT_THROWS_NOTHING(
        TestableSymmetryOperationSymbolParser::parseIdentifier("x, -y, -z"));
    TS_ASSERT_THROWS_NOTHING(
        TestableSymmetryOperationSymbolParser::parseIdentifier("-x, y, z"));
    TS_ASSERT_THROWS_NOTHING(
        TestableSymmetryOperationSymbolParser::parseIdentifier(
            "1/4 - x, 1/2+y, z-x"));

    TS_ASSERT_THROWS(
        TestableSymmetryOperationSymbolParser::parseIdentifier("1/4, x, -z-x"),
        const std::runtime_error &);
    TS_ASSERT_THROWS(
        TestableSymmetryOperationSymbolParser::parseIdentifier("x, -z-x"),
        const std::runtime_error &);
    TS_ASSERT_THROWS(
        TestableSymmetryOperationSymbolParser::parseIdentifier("y, x, -z-x, z"),
        const std::runtime_error &);
  }

  void testGetNormalizedIdentifier() {
    MatrixVectorPair<int, V3R> param1 =
        SymmetryOperationSymbolParser::parseIdentifier("x+1/2, y, -z-1/2");
    TS_ASSERT_EQUALS(
        SymmetryOperationSymbolParser::getNormalizedIdentifier(param1),
        "x+1/2,y,-z-1/2");

    MatrixVectorPair<int, V3R> param2 =
        TestableSymmetryOperationSymbolParser::parseIdentifier(
            "1/2+x, y, -1/2-z");
    TS_ASSERT_EQUALS(
        SymmetryOperationSymbolParser::getNormalizedIdentifier(param2),
        "x+1/2,y,-z-1/2");
  }

  void testGetNormalizedIdentifierDenominatorOne() {
    MatrixVectorPair<int, V3R> param1 =
        SymmetryOperationSymbolParser::parseIdentifier("2+x, y, -z-1");
    TS_ASSERT_EQUALS(
        SymmetryOperationSymbolParser::getNormalizedIdentifier(param1),
        "x+2,y,-z-1");
  }

private:
  class TestableSymmetryOperationSymbolParser : SymmetryOperationSymbolParser {
    friend class SymmetryOperationSymbolParserTest;

  public:
    TestableSymmetryOperationSymbolParser() : SymmetryOperationSymbolParser() {}
    ~TestableSymmetryOperationSymbolParser() {}
  };
};

#endif /* MANTID_GEOMETRY_SYMMETRYOPERATIONSYMBOLPARSERTEST_H_ */
