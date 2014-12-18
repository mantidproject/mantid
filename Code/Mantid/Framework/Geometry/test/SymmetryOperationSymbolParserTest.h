#ifndef MANTID_GEOMETRY_SYMMETRYOPERATIONSYMBOLPARSERTEST_H_
#define MANTID_GEOMETRY_SYMMETRYOPERATIONSYMBOLPARSERTEST_H_

#include <cxxtest/TestSuite.h>
#include <boost/lexical_cast.hpp>

#include "MantidGeometry/Crystal/SymmetryOperationSymbolParser.h"

using Mantid::Geometry::SymmetryOperationSymbolParser;
using namespace Mantid::Geometry;

class SymmetryOperationSymbolParserTest : public CxxTest::TestSuite
{
public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static SymmetryOperationSymbolParserTest *createSuite() { return new SymmetryOperationSymbolParserTest(); }
    static void destroySuite( SymmetryOperationSymbolParserTest *suite ) { delete suite; }

    void testGetFactorForSign()
    {
        TS_ASSERT_EQUALS(TestableSymmetryOperationSymbolParser::getFactorForSign('-'), -1);
        TS_ASSERT_EQUALS(TestableSymmetryOperationSymbolParser::getFactorForSign('+'), 1);
        TS_ASSERT_THROWS(TestableSymmetryOperationSymbolParser::getFactorForSign('f'), std::runtime_error);
        TS_ASSERT_THROWS(TestableSymmetryOperationSymbolParser::getFactorForSign('t'), std::runtime_error);
        TS_ASSERT_THROWS(TestableSymmetryOperationSymbolParser::getFactorForSign('1'), std::runtime_error);
    }

    void testGetVectorForSymbol()
    {
        std::vector<int> x;
        TS_ASSERT_THROWS_NOTHING(x = TestableSymmetryOperationSymbolParser::getVectorForSymbol('x'));
        TS_ASSERT_EQUALS(x.size(), 3);
        TS_ASSERT_EQUALS(x[0], 1);
        TS_ASSERT_EQUALS(x[1], 0);
        TS_ASSERT_EQUALS(x[2], 0);

        std::vector<int> y;
        TS_ASSERT_THROWS_NOTHING(y = TestableSymmetryOperationSymbolParser::getVectorForSymbol('y'));
        TS_ASSERT_EQUALS(y.size(), 3);
        TS_ASSERT_EQUALS(y[0], 0);
        TS_ASSERT_EQUALS(y[1], 1);
        TS_ASSERT_EQUALS(y[2], 0);

        std::vector<int> z;
        TS_ASSERT_THROWS_NOTHING(z = TestableSymmetryOperationSymbolParser::getVectorForSymbol('z'));
        TS_ASSERT_EQUALS(z.size(), 3);
        TS_ASSERT_EQUALS(z[0], 0);
        TS_ASSERT_EQUALS(z[1], 0);
        TS_ASSERT_EQUALS(z[2], 1);

        std::vector<int> yMinus;
        TS_ASSERT_THROWS_NOTHING(yMinus = TestableSymmetryOperationSymbolParser::getVectorForSymbol('y', '-'));
        TS_ASSERT_EQUALS(yMinus.size(), 3);
        TS_ASSERT_EQUALS(yMinus[0], 0);
        TS_ASSERT_EQUALS(yMinus[1], -1);
        TS_ASSERT_EQUALS(yMinus[2], 0);

        TS_ASSERT_THROWS(TestableSymmetryOperationSymbolParser::getVectorForSymbol('t'), std::runtime_error);
        TS_ASSERT_THROWS(TestableSymmetryOperationSymbolParser::getVectorForSymbol('1'), std::runtime_error);
        TS_ASSERT_THROWS(TestableSymmetryOperationSymbolParser::getVectorForSymbol('+'), std::runtime_error);
    }

    void testAddToVector()
    {
        std::vector<int> one(3, 1);
        std::vector<int> two(3, 2);
        std::vector<int> wrongSize(1, 3);

        TS_ASSERT_THROWS_NOTHING(TestableSymmetryOperationSymbolParser::addToVector(one, two));

        TS_ASSERT_EQUALS(one[0], 3);
        TS_ASSERT_EQUALS(one[1], 3);
        TS_ASSERT_EQUALS(one[2], 3);

        TS_ASSERT_THROWS(TestableSymmetryOperationSymbolParser::addToVector(one, wrongSize), std::runtime_error);
    }

    void testProcessMatrixRowToken()
    {
        std::vector<int> matrixRow(3, 0);
        TS_ASSERT_THROWS_NOTHING(TestableSymmetryOperationSymbolParser::processMatrixRowToken("+x", matrixRow));

        TS_ASSERT_EQUALS(matrixRow[0], 1);
        TS_ASSERT_EQUALS(matrixRow[1], 0);
        TS_ASSERT_EQUALS(matrixRow[2], 0);

        TS_ASSERT_THROWS_NOTHING(TestableSymmetryOperationSymbolParser::processMatrixRowToken("+y", matrixRow));

        TS_ASSERT_EQUALS(matrixRow[0], 1);
        TS_ASSERT_EQUALS(matrixRow[1], 1);
        TS_ASSERT_EQUALS(matrixRow[2], 0);

        TS_ASSERT_THROWS_NOTHING(TestableSymmetryOperationSymbolParser::processMatrixRowToken("-y", matrixRow));

        TS_ASSERT_EQUALS(matrixRow[0], 1);
        TS_ASSERT_EQUALS(matrixRow[1], 0);
        TS_ASSERT_EQUALS(matrixRow[2], 0);

        TS_ASSERT_THROWS_NOTHING(TestableSymmetryOperationSymbolParser::processMatrixRowToken("-z", matrixRow));

        TS_ASSERT_EQUALS(matrixRow[0], 1);
        TS_ASSERT_EQUALS(matrixRow[1], 0);
        TS_ASSERT_EQUALS(matrixRow[2], -1);

        TS_ASSERT_THROWS_NOTHING(TestableSymmetryOperationSymbolParser::processMatrixRowToken("z", matrixRow));

        TS_ASSERT_EQUALS(matrixRow[0], 1);
        TS_ASSERT_EQUALS(matrixRow[1], 0);
        TS_ASSERT_EQUALS(matrixRow[2], 0);

        TS_ASSERT_THROWS(TestableSymmetryOperationSymbolParser::processMatrixRowToken("g", matrixRow), std::runtime_error);
        TS_ASSERT_THROWS(TestableSymmetryOperationSymbolParser::processMatrixRowToken("", matrixRow), std::runtime_error);
        TS_ASSERT_THROWS(TestableSymmetryOperationSymbolParser::processMatrixRowToken("+-g", matrixRow), std::runtime_error);
        TS_ASSERT_THROWS(TestableSymmetryOperationSymbolParser::processMatrixRowToken("-+", matrixRow), std::runtime_error);
        TS_ASSERT_THROWS(TestableSymmetryOperationSymbolParser::processMatrixRowToken("xx", matrixRow), std::runtime_error);
    }

    void testProcessVectorComponentToken()
    {
        RationalNumber num;

        TS_ASSERT_THROWS_NOTHING(TestableSymmetryOperationSymbolParser::processVectorComponentToken("+1/4", num) );
        TS_ASSERT_EQUALS(num, RationalNumber(1, 4));

        TS_ASSERT_THROWS_NOTHING(TestableSymmetryOperationSymbolParser::processVectorComponentToken("+1/2", num) );
        TS_ASSERT_EQUALS(num, RationalNumber(3, 4));

        TS_ASSERT_THROWS_NOTHING(TestableSymmetryOperationSymbolParser::processVectorComponentToken("-10/20", num) );
        TS_ASSERT_EQUALS(num, RationalNumber(1, 4));

        TS_ASSERT_THROWS_NOTHING(TestableSymmetryOperationSymbolParser::processVectorComponentToken("-1/4", num) );
        TS_ASSERT_EQUALS(num, 0);

        TS_ASSERT_THROWS_NOTHING(TestableSymmetryOperationSymbolParser::processVectorComponentToken("12", num) );
        TS_ASSERT_EQUALS(num, 12);

        TS_ASSERT_THROWS_NOTHING(TestableSymmetryOperationSymbolParser::processVectorComponentToken("-12", num) );
        TS_ASSERT_EQUALS(num, 0);

        TS_ASSERT_THROWS(TestableSymmetryOperationSymbolParser::processVectorComponentToken("1/2/3", num), std::runtime_error);
        TS_ASSERT_THROWS(TestableSymmetryOperationSymbolParser::processVectorComponentToken("/2/3", num), std::runtime_error);
        TS_ASSERT_THROWS(TestableSymmetryOperationSymbolParser::processVectorComponentToken("-/2/3", num), std::runtime_error);

        TS_ASSERT_THROWS(TestableSymmetryOperationSymbolParser::processVectorComponentToken("", num), boost::bad_lexical_cast);
        TS_ASSERT_THROWS(TestableSymmetryOperationSymbolParser::processVectorComponentToken("g/d", num), boost::bad_lexical_cast);
        TS_ASSERT_THROWS(TestableSymmetryOperationSymbolParser::processVectorComponentToken("--2", num), boost::bad_lexical_cast);
        TS_ASSERT_THROWS(TestableSymmetryOperationSymbolParser::processVectorComponentToken("+3e", num), boost::bad_lexical_cast);
        TS_ASSERT_THROWS(TestableSymmetryOperationSymbolParser::processVectorComponentToken("1/f", num), boost::bad_lexical_cast);
    }

    void testParseComponent()
    {
        std::pair<std::vector<int>, RationalNumber> result;
        TS_ASSERT_THROWS_NOTHING(result = TestableSymmetryOperationSymbolParser::parseComponent("x+1/4"));
        TS_ASSERT_EQUALS(result.first[0], 1);
        TS_ASSERT_EQUALS(result.first[1], 0);
        TS_ASSERT_EQUALS(result.first[2], 0);
        TS_ASSERT_EQUALS(result.second, RationalNumber(1, 4));

        TS_ASSERT_THROWS_NOTHING(result = TestableSymmetryOperationSymbolParser::parseComponent("x+y-1/4"));
        TS_ASSERT_EQUALS(result.first[0], 1);
        TS_ASSERT_EQUALS(result.first[1], 1);
        TS_ASSERT_EQUALS(result.first[2], 0);
        TS_ASSERT_EQUALS(result.second, RationalNumber(-1, 4));

        TS_ASSERT_THROWS_NOTHING(result = TestableSymmetryOperationSymbolParser::parseComponent("1/4-x"));
        TS_ASSERT_EQUALS(result.first[0], -1);
        TS_ASSERT_EQUALS(result.first[1], 0);
        TS_ASSERT_EQUALS(result.first[2], 0);
        TS_ASSERT_EQUALS(result.second, RationalNumber(1, 4));

        TS_ASSERT_THROWS_NOTHING(result = TestableSymmetryOperationSymbolParser::parseComponent("-x+z-1/4"));
        TS_ASSERT_EQUALS(result.first[0], -1);
        TS_ASSERT_EQUALS(result.first[1], 0);
        TS_ASSERT_EQUALS(result.first[2], 1);
        TS_ASSERT_EQUALS(result.second, RationalNumber(-1, 4));

        TS_ASSERT_THROWS(result = TestableSymmetryOperationSymbolParser::parseComponent("x+x+1/4"), std::runtime_error);
        TS_ASSERT_THROWS(result = TestableSymmetryOperationSymbolParser::parseComponent("--1/4"), std::runtime_error);
        TS_ASSERT_THROWS(result = TestableSymmetryOperationSymbolParser::parseComponent("-s/4"), std::runtime_error);
        TS_ASSERT_THROWS(result = TestableSymmetryOperationSymbolParser::parseComponent("argwertq"), std::runtime_error);
        TS_ASSERT_THROWS(result = TestableSymmetryOperationSymbolParser::parseComponent("x/4+z"), std::runtime_error);
    }

    void testGetCleanComponentString()
    {
        TestableSymmetryOperationSymbolParser symOpParser;

        TS_ASSERT_EQUALS(TestableSymmetryOperationSymbolParser::getCleanComponentString("x + 1/2"), "x+1/2");
        TS_ASSERT_EQUALS(TestableSymmetryOperationSymbolParser::getCleanComponentString(" x + 1/2 "), "x+1/2");
        TS_ASSERT_EQUALS(TestableSymmetryOperationSymbolParser::getCleanComponentString(" x + 1 / 2 "), "x+1/2");
    }

    void testParseComponents()
    {
        std::vector<std::string> components;
        components.push_back("x+z");
        components.push_back("1/4-x");
        components.push_back("y");

        std::pair<Mantid::Kernel::IntMatrix, V3R> parsed;
        TS_ASSERT_THROWS_NOTHING(parsed = TestableSymmetryOperationSymbolParser::parseComponents(components));

        TS_ASSERT_EQUALS(parsed.first[0][0], 1);
        TS_ASSERT_EQUALS(parsed.first[0][1], 0);
        TS_ASSERT_EQUALS(parsed.first[0][2], 1);

        TS_ASSERT_EQUALS(parsed.first[1][0], -1);
        TS_ASSERT_EQUALS(parsed.first[1][1], 0);
        TS_ASSERT_EQUALS(parsed.first[1][2], 0);

        TS_ASSERT_EQUALS(parsed.first[2][0], 0);
        TS_ASSERT_EQUALS(parsed.first[2][1], 1);
        TS_ASSERT_EQUALS(parsed.first[2][2], 0);

        TS_ASSERT_EQUALS(parsed.second, V3R(0, RationalNumber(1, 4), 0));
    }

    void testParseIdentifier()
    {
        TS_ASSERT_THROWS_NOTHING(TestableSymmetryOperationSymbolParser::parseIdentifier("x, y, z"));
        TS_ASSERT_THROWS_NOTHING(TestableSymmetryOperationSymbolParser::parseIdentifier("x, -y, -z"));
        TS_ASSERT_THROWS_NOTHING(TestableSymmetryOperationSymbolParser::parseIdentifier("-x, y, z"));
        TS_ASSERT_THROWS_NOTHING(TestableSymmetryOperationSymbolParser::parseIdentifier("1/4 - x, 1/2+y, z-x"));

        TS_ASSERT_THROWS(TestableSymmetryOperationSymbolParser::parseIdentifier("1/4, x, -z-x"), std::runtime_error);
        TS_ASSERT_THROWS(TestableSymmetryOperationSymbolParser::parseIdentifier("x, -z-x"), std::runtime_error);
        TS_ASSERT_THROWS(TestableSymmetryOperationSymbolParser::parseIdentifier("y, x, -z-x, z"), std::runtime_error);
    }

    void testGetNormalizedIdentifier()
    {
        std::pair<Mantid::Kernel::IntMatrix, V3R> param1 = SymmetryOperationSymbolParser::parseIdentifier("x+1/2, y, -z-1/2");
        TS_ASSERT_EQUALS(SymmetryOperationSymbolParser::getNormalizedIdentifier(param1), "x+1/2,y,-z-1/2");

        std::pair<Mantid::Kernel::IntMatrix, V3R> param2 = TestableSymmetryOperationSymbolParser::parseIdentifier("1/2+x, y, -1/2-z");
        TS_ASSERT_EQUALS(SymmetryOperationSymbolParser::getNormalizedIdentifier(param2), "x+1/2,y,-z-1/2");
    }

private:
    class TestableSymmetryOperationSymbolParser : SymmetryOperationSymbolParser
    {
        friend class SymmetryOperationSymbolParserTest;
    public:
        TestableSymmetryOperationSymbolParser() : SymmetryOperationSymbolParser() { }
        ~TestableSymmetryOperationSymbolParser() { }
    };
};


#endif /* MANTID_GEOMETRY_SYMMETRYOPERATIONSYMBOLPARSERTEST_H_ */
