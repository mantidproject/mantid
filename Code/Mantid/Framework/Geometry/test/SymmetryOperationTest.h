#ifndef MANTID_GEOMETRY_SYMMETRYOPERATIONTEST_H_
#define MANTID_GEOMETRY_SYMMETRYOPERATIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/SymmetryOperation.h"
#include "MantidKernel/V3D.h"

#include <boost/regex.hpp>
#include <boost/make_shared.hpp>
#include <boost/lexical_cast.hpp>

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class SymmetryOperationTest;

class TestableSymmetryOperation : SymmetryOperation
{
    friend class SymmetryOperationTest;
public:
    TestableSymmetryOperation() :
        SymmetryOperation(0, IntMatrix(3, 3, false), "0")
    { }
};

class SymmetryOperationTest : public CxxTest::TestSuite
{
public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static SymmetryOperationTest *createSuite() { return new SymmetryOperationTest(); }
    static void destroySuite( SymmetryOperationTest *suite ) { delete suite; }

    SymmetryOperationTest() :
        m_h(3.0), m_k(2.0), m_l(4.0),
        m_hkl(m_h, m_k, m_l),
        m_hhl(m_h, m_h, m_l),
        m_hk0(m_h, m_k, 0.0),
        m_h00(m_h, 0.0, 0.0),
        m_allHkl(),
        m_identifierRegex("^-?((1)|((2|3|4|6|m) \\[(-?\\d{1}){3}\\]h?))$")
    {
        m_allHkl.push_back(m_hkl);
        m_allHkl.push_back(m_hhl);
        m_allHkl.push_back(m_hk0);
        m_allHkl.push_back(m_h00);
    }

    void testAssignMatrixFromArray()
    {
        TestableSymmetryOperation emptyOp;

        TS_ASSERT_DIFFERS(emptyOp.m_matrix, IntMatrix(3, 3, true));

        int identity[] = {1, 0, 0,
                          0, 1, 0,
                          0, 0, 1};

        emptyOp.setMatrixFromArray(identity);

        TS_ASSERT_EQUALS(emptyOp.m_matrix, IntMatrix(3, 3, true));

        int someMatrix[] = {1, 2, 3,
                            4, 5, 6,
                            7, 8, 9};

        emptyOp.setMatrixFromArray(someMatrix);

        // first row
        TS_ASSERT_EQUALS(emptyOp.m_matrix[0][0], 1);
        TS_ASSERT_EQUALS(emptyOp.m_matrix[0][1], 2);
        TS_ASSERT_EQUALS(emptyOp.m_matrix[0][2], 3);

        // second row
        TS_ASSERT_EQUALS(emptyOp.m_matrix[1][0], 4);
        TS_ASSERT_EQUALS(emptyOp.m_matrix[1][1], 5);
        TS_ASSERT_EQUALS(emptyOp.m_matrix[1][2], 6);

        // third row
        TS_ASSERT_EQUALS(emptyOp.m_matrix[2][0], 7);
        TS_ASSERT_EQUALS(emptyOp.m_matrix[2][1], 8);
        TS_ASSERT_EQUALS(emptyOp.m_matrix[2][2], 9);
    }

    void testIdentifierRegEx()
    {
        std::vector<std::string> goodInput;
        goodInput.push_back("1");
        goodInput.push_back("-1");
        goodInput.push_back("2 [100]");
        goodInput.push_back("3 [100]");
        goodInput.push_back("4 [100]");
        goodInput.push_back("6 [100]");
        goodInput.push_back("m [100]");
        goodInput.push_back("2 [100]h");
        goodInput.push_back("m [-100]");
        goodInput.push_back("m [-1-1-1]");
        goodInput.push_back("-3 [100]");

        for(size_t i = 0; i < goodInput.size(); ++i) {
            TSM_ASSERT(goodInput[i] + " did not match regular expression.", boost::regex_match(goodInput[i], m_identifierRegex));
        }

        std::vector<std::string> badInput;
        badInput.push_back("1 [100]");
        badInput.push_back("-1 [100]");
        badInput.push_back("2");
        badInput.push_back("-2");
        badInput.push_back("2 [100");
        badInput.push_back("2 100");
        badInput.push_back("2 [10]");
        badInput.push_back("2 [1002]");
        badInput.push_back("2 [--120]");
        badInput.push_back("2 [120]k");

        for(size_t i = 0; i < badInput.size(); ++i) {
            TSM_ASSERT(badInput[i] + " unexpectedly matched regular expression.", !boost::regex_match(badInput[i], m_identifierRegex));
        }
    }

    void testIdentity()
    {
        auto identity = boost::make_shared<const SymOpIdentity>();

        checkCorrectOrder(identity, 1);
        TS_ASSERT_EQUALS(applyOrderTimes(identity, m_hkl), m_hkl);

        checkCorrectOrderAll(identity);
    }

    void testInversion()
    {
        testSymmetryOperation(boost::make_shared<const SymOpInversion>(),
                              2, m_hkl * -1.0, "-1");
    }

    // Rotations
    // 2-fold
    void testRotationTwoFoldX()
    {
        testSymmetryOperation(boost::make_shared<const SymOpRotationTwoFoldX>(),
                              2, V3D(m_h, -m_k, -m_l), "2 [100]");
    }

    void testRotationTwoFoldY()
    {
        testSymmetryOperation(boost::make_shared<const SymOpRotationTwoFoldY>(),
                              2, V3D(-m_h, m_k, -m_l), "2 [010]");
    }

    void testRotationTwoFoldZ()
    {
        testSymmetryOperation(boost::make_shared<const SymOpRotationTwoFoldZ>(),
                              2, V3D(-m_h, -m_k, m_l), "2 [001]");
    }

    void testRotationTwoFoldXHexagonal()
    {
        testSymmetryOperation(boost::make_shared<const SymOpRotationTwoFoldXHexagonal>(),
                              2, V3D(m_h-m_k, -m_k, -m_l), "2 [100]h");
    }

    void testRotationTwoFold210Hexagonal()
    {
        testSymmetryOperation(boost::make_shared<const SymOpRotationTwoFold210Hexagonal>(),
                              2, V3D(m_h, m_h-m_k, -m_l), "2 [210]h");
    }

    // 4-fold
    void testRotation4FoldZ()
    {
        testSymmetryOperation(boost::make_shared<const SymOpRotationFourFoldZ>(),
                              4, V3D(-m_k, m_h, m_l), "4 [001]");
    }

    // 3-fold
    void testRotationThreeFoldZHexagonal()
    {
        testSymmetryOperation(boost::make_shared<const SymOpRotationThreeFoldZHexagonal>(),
                              3, V3D(-m_k, m_h-m_k, m_l), "3 [001]h");
    }

    void testRotationThreeFold111()
    {
        testSymmetryOperation(boost::make_shared<const SymOpRotationThreeFold111>(),
                              3, V3D(m_l, m_h, m_k), "3 [111]");
    }

    // 6-fold
    void testRotationSixFoldZHexagonal()
    {
        testSymmetryOperation(boost::make_shared<const SymOpRotationSixFoldZHexagonal>(),
                              6, V3D(m_h-m_k, m_h, m_l), "6 [001]h");
    }

    // Mirror planes
    void testMirrorPlaneY()
    {
        testSymmetryOperation(boost::make_shared<const SymOpMirrorPlaneY>(),
                              2, V3D(m_h, -m_k, m_l), "m [010]");
    }

    void testMirrorPlaneZ()
    {
        testSymmetryOperation(boost::make_shared<const SymOpMirrorPlaneZ>(),
                              2, V3D(m_h, m_k, -m_l), "m [001]");
    }

    void testMirrorPlane210Hexagonal()
    {
        testSymmetryOperation(boost::make_shared<const SymOpMirrorPlane210Hexagonal>(),
                              2, V3D(-m_h, m_k-m_h, m_l), "m [210]h");
    }

private:
    V3D applyOrderTimes(const SymmetryOperation_const_sptr &symOp, const V3D &vector)
    {
        return applyNTimes(symOp, vector, symOp->order());
    }

    V3D applyLessThanOrderTimes(const SymmetryOperation_const_sptr &symOp, const V3D &vector)
    {
        return applyNTimes(symOp, vector, symOp->order() - 1);
    }

    V3D applyNTimes(const SymmetryOperation_const_sptr &symOp, const V3D &vector, size_t n)
    {
        V3D vectorCopy(vector);

        for(size_t i = 0; i < n; ++i) {
            vectorCopy = symOp->apply(vectorCopy);
        }

        return vectorCopy;
    }

    void testSymmetryOperation(const SymmetryOperation_const_sptr &symOp, size_t expectedOrder, const V3D &expectedHKL, const std::string &expectedIdentifier)
    {
        checkCorrectOrder(symOp, expectedOrder);
        checkCorrectTransformationGeneralHKL(symOp, expectedHKL);
        checkIdentifierString(symOp, expectedIdentifier);

        performCommonTests(symOp);
    }

    void checkCorrectOrder(const SymmetryOperation_const_sptr &symOp, size_t expected)
    {
        size_t order = symOp->order();

        TSM_ASSERT_EQUALS(symOp->identifier() + ": Order is " + boost::lexical_cast<std::string>(order) + ", expected " + boost::lexical_cast<std::string>(expected),
                          order, expected);
    }

    void checkCorrectTransformationGeneralHKL(const SymmetryOperation_const_sptr &symOp, const V3D &expected)
    {
        V3D transformed = symOp->apply(m_hkl);

        TSM_ASSERT_EQUALS(symOp->identifier() + ": Transformed hkl is " + transformed.toString() + ", expected " + expected.toString(),
                          transformed, expected);
    }

    void checkIdentifierString(const SymmetryOperation_const_sptr &symOp, const std::string &expected)
    {
        std::string identifier = symOp->identifier();

        TSM_ASSERT(identifier + ": Does not match regular expression.",
                   boost::regex_match(identifier, m_identifierRegex));
        TSM_ASSERT_EQUALS(identifier + ": Does not match expected identifier " + expected,
                    identifier, expected);
    }

    void performCommonTests(const SymmetryOperation_const_sptr &symOp)
    {
        checkGeneralReflection(symOp);
        checkCorrectOrderAll(symOp);
        checkDeterminant(symOp);
    }

    void checkGeneralReflection(const SymmetryOperation_const_sptr &symOp)
    {
        V3D transformedOrderTimes = applyOrderTimes(symOp, m_hkl);

        TSM_ASSERT_EQUALS(symOp->identifier() + ": Transforming " + m_hkl.toString() + " $order times lead to unexpected result " + transformedOrderTimes.toString(),
                          transformedOrderTimes, m_hkl);

        V3D transformedLessThanOrderTimes = applyLessThanOrderTimes(symOp, m_hkl);
        TSM_ASSERT_DIFFERS(symOp->identifier() + ": Transforming " + m_hkl.toString() + " less than $order times lead to unexpected result " + transformedLessThanOrderTimes.toString(),
                          transformedLessThanOrderTimes, m_hkl);
    }

    void checkCorrectOrderAll(const SymmetryOperation_const_sptr &symOp)
    {
        for(size_t i = 0; i < m_allHkl.size(); ++i) {
            TS_ASSERT_EQUALS(applyOrderTimes(symOp, m_allHkl[i]), m_allHkl[i]);
        }
    }

    void checkDeterminant(const SymmetryOperation_const_sptr &symOp)
    {
        IntMatrix symOpMatrix = symOp->apply(IntMatrix(3, 3, true));
        int determinant = abs(symOpMatrix.determinant());

        TSM_ASSERT_EQUALS(symOp->identifier() + ": Determinant of symmetry operation matrix is expected to be 1. Actual value: " + boost::lexical_cast<std::string>(determinant),
                          determinant, 1);
    }

    double m_h;
    double m_k;
    double m_l;

    // hkls to test
    V3D m_hkl;
    V3D m_hhl;
    V3D m_hk0;
    V3D m_h00;

    std::vector<V3D> m_allHkl;

    // regex for matching symmetry operation identifiers
    boost::regex m_identifierRegex;
};


#endif /* MANTID_GEOMETRY_SYMMETRYOPERATIONTEST_H_ */
