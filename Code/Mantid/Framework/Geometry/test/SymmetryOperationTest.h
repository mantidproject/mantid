#ifndef MANTID_GEOMETRY_SYMMETRYOPERATIONTEST_H_
#define MANTID_GEOMETRY_SYMMETRYOPERATIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/SymmetryOperation.h"
#include "MantidKernel/V3D.h"

#include <boost/make_shared.hpp>

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class SymmetryOperationTest;

class TestableSymmetryOperation : SymmetryOperation
{
    friend class SymmetryOperationTest;
public:
    TestableSymmetryOperation() :
        SymmetryOperation(0, IntMatrix(3, 3, false))
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
        m_allHkl()
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
                              2, m_hkl * -1.0);
    }

    // Rotations
    // 2-fold
    void testRotationTwoFoldX()
    {
        testSymmetryOperation(boost::make_shared<const SymOpRotationTwoFoldX>(),
                              2, V3D(m_h, -m_k, -m_l));
    }

    void testRotationTwoFoldY()
    {
        testSymmetryOperation(boost::make_shared<const SymOpRotationTwoFoldY>(),
                              2, V3D(-m_h, m_k, -m_l));
    }

    void testRotationTwoFoldZ()
    {
        testSymmetryOperation(boost::make_shared<const SymOpRotationTwoFoldZ>(),
                              2, V3D(-m_h, -m_k, m_l));
    }

    void testRotationTwoFoldXHexagonal()
    {
        testSymmetryOperation(boost::make_shared<const SymOpRotationTwoFoldXHexagonal>(),
                              2, V3D(m_h-m_k, -m_k, -m_l));
    }

    void testRotationTwoFold210Hexagonal()
    {
        testSymmetryOperation(boost::make_shared<const SymOpRotationTwoFold210Hexagonal>(),
                              2, V3D(m_h, m_h-m_k, -m_l));
    }

    // 4-fold
    void testRotation4FoldZ()
    {
        testSymmetryOperation(boost::make_shared<const SymOpRotationFourFoldZ>(),
                              4, V3D(-m_k, m_h, m_l));
    }

    // 3-fold
    void testRotationThreeFoldZHexagonal()
    {
        testSymmetryOperation(boost::make_shared<const SymOpRotationThreeFoldZHexagonal>(),
                              3, V3D(-m_k, m_h-m_k, m_l));
    }

    void testRotationThreeFold111()
    {
        testSymmetryOperation(boost::make_shared<const SymOpRotationThreeFold111>(),
                              3, V3D(m_l, m_h, m_k));
    }

    // 6-fold
    void testRotationSixFoldZHexagonal()
    {
        testSymmetryOperation(boost::make_shared<const SymOpRotationSixFoldZHexagonal>(),
                              6, V3D(m_h-m_k, m_h, m_l));
    }

    // Mirror planes
    void testMirrorPlaneY()
    {
        testSymmetryOperation(boost::make_shared<const SymOpMirrorPlaneY>(),
                              2, V3D(m_h, -m_k, m_l));
    }

    void testMirrorPlaneZ()
    {
        testSymmetryOperation(boost::make_shared<const SymOpMirrorPlaneZ>(),
                              2, V3D(m_h, m_k, -m_l));
    }

    void testMirrorPlane210Hexagonal()
    {
        testSymmetryOperation(boost::make_shared<const SymOpMirrorPlane210Hexagonal>(),
                              2, V3D(-m_h, m_k-m_h, m_l));
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

    void testSymmetryOperation(const SymmetryOperation_const_sptr &symOp, size_t expectedOrder, const V3D &expectedHKL)
    {
        checkCorrectOrder(symOp, expectedOrder);
        checkCorrectTransformationGeneralHKL(symOp, expectedHKL);

        performCommonTests(symOp);
    }

    void checkCorrectOrder(const SymmetryOperation_const_sptr &symOp, size_t expected)
    {
        TS_ASSERT_EQUALS(symOp->order(), expected);
    }

    void checkCorrectTransformationGeneralHKL(const SymmetryOperation_const_sptr &symOp, const V3D &expected)
    {
        TS_ASSERT_EQUALS(symOp->apply(m_hkl), expected);
    }

    void performCommonTests(const SymmetryOperation_const_sptr &symOp)
    {
        checkGeneralReflection(symOp);
        checkCorrectOrderAll(symOp);
        checkDeterminant(symOp);
    }

    void checkGeneralReflection(const SymmetryOperation_const_sptr &symOp)
    {
        TS_ASSERT_EQUALS(applyOrderTimes(symOp, m_hkl), m_hkl);
        TS_ASSERT_DIFFERS(applyLessThanOrderTimes(symOp, m_hkl), m_hkl);
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

        TS_ASSERT_EQUALS(abs(symOpMatrix.determinant()), 1);
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
};


#endif /* MANTID_GEOMETRY_SYMMETRYOPERATIONTEST_H_ */
