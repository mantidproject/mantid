#ifndef MANTID_GEOMETRY_SYMMETRYOPERATIONTEST_H_
#define MANTID_GEOMETRY_SYMMETRYOPERATIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/SymmetryOperation.h"
#include "MantidGeometry/Crystal/SymmetryOperationSymbolParser.h"

#include "MantidKernel/V3D.h"

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
        SymmetryOperation()
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

    void testDefaultConstructor()
    {
        SymmetryOperation symOp;
        TS_ASSERT(symOp.isIdentity());
        TS_ASSERT(!symOp.hasTranslation())
        TS_ASSERT_EQUALS(symOp.order(), 1);
        TS_ASSERT_EQUALS(symOp.identifier(), "x,y,z");

        V3D hkl(1, 1, 1);
        TS_ASSERT_EQUALS(symOp * hkl, hkl);
    }

    void testStringConstructor()
    {
        SymmetryOperation inversion("-x,-y,-z");

        TS_ASSERT(!inversion.isIdentity());
        TS_ASSERT(!inversion.hasTranslation());
        TS_ASSERT_EQUALS(inversion.order(), 2);
        TS_ASSERT_EQUALS(inversion.identifier(), "-x,-y,-z");

        V3D hkl(1, 1, 1);
        TS_ASSERT_EQUALS(inversion * hkl, hkl * -1.0);

        // translational components are wrapped to the unit cell
        SymmetryOperation screw21z("-x,-y,z+3/2");
        TS_ASSERT_EQUALS(screw21z.identifier(), "-x,-y,z+1/2");
    }

    void testCopyConstructor()
    {
        SymmetryOperation inversion("-x,-y,-z");
        SymmetryOperation anotherInversion(inversion);

        TS_ASSERT_EQUALS(inversion, anotherInversion);
        TS_ASSERT_EQUALS(inversion.order(), anotherInversion.order());
        TS_ASSERT_EQUALS(inversion.identifier(), anotherInversion.identifier());
    }

    void testIsIdentity()
    {
        SymmetryOperation identity;
        TS_ASSERT(identity.isIdentity());

        SymmetryOperation inversion("-x,-y,-z");
        TS_ASSERT(!inversion.isIdentity());

        SymmetryOperation screw21z("-x,-y,z+1/2");
        TS_ASSERT(!screw21z.isIdentity());

        SymmetryOperation shift("x+1/2,y+1/2,z+1/2");
        TS_ASSERT(!shift.isIdentity());
    }

    void testHasTranslation()
    {
        SymmetryOperation identity;
        TS_ASSERT(!identity.hasTranslation());

        SymmetryOperation inversion("-x,-y,-z");
        TS_ASSERT(!inversion.hasTranslation());

        SymmetryOperation screw21z("-x,-y,z+1/2");
        TS_ASSERT(screw21z.hasTranslation());

        SymmetryOperation shift("x+1/2,y+1/2,z+1/2");
        TS_ASSERT(shift.hasTranslation());
    }

    void testMultiplicationOperator()
    {
        SymmetryOperation inversion("-x,-y,-z");

        V3D hklDouble(1.0, 1.0, 1.0);
        V3D hklDoubleReferenceInversion(-1.0, -1.0, -1.0);
        TS_ASSERT_EQUALS(inversion * hklDouble, hklDoubleReferenceInversion);

        V3R hklRational(1, 1, 1);
        V3R hklRationalReferenceInversion(-1, -1, -1);
        TS_ASSERT_EQUALS(inversion * hklRational, hklRationalReferenceInversion);

        SymmetryOperation screw21z("-x,-y,z+1/2");

        V3D coordinates(0.35, 0.45, 0.75);
        V3D coordinatesReference(-0.35, -0.45, 1.25);

        TS_ASSERT_EQUALS(screw21z * coordinates, coordinatesReference);
    }

    void testMultiplicationOperatorSymmetryOperation()
    {
        SymmetryOperation screw21z("-x,-y,z+1/2");
        SymmetryOperation identity;

        // should be identity, since 1/2 + 1/2 = 1 => 0
        TS_ASSERT_EQUALS(screw21z * screw21z, identity);
    }

    void testInverse()
    {
        SymmetryOperation identity("x,y,z");
        SymmetryOperation inversion = identity.inverse();
        TS_ASSERT_EQUALS(inversion.identifier(), "x,y,z");

        SymmetryOperation fourFoldZPlus("-y,x,z");
        SymmetryOperation fourFoldZMinus = fourFoldZPlus.inverse();
        TS_ASSERT_EQUALS(fourFoldZMinus.identifier(), "y,-x,z");

        SymmetryOperation fourOneScrewZPlus("-y,x,z+1/4");
        SymmetryOperation fourOneScrewZMinus = fourOneScrewZPlus.inverse();
        TS_ASSERT_EQUALS(fourOneScrewZMinus.identifier(), "y,-x,z+3/4");

        // (Op^-1)^-1 = Op
        TS_ASSERT_EQUALS(fourOneScrewZMinus.inverse(), fourOneScrewZPlus);

        // Op * Op^-1 = Identity
        TS_ASSERT_EQUALS(fourOneScrewZPlus * fourOneScrewZMinus, identity);
    }

    void testGetWrappedVectorV3R()
    {
        V3R one = V3R(1, 1, 1) / 2;
        TS_ASSERT_EQUALS(one, getWrappedVector(one));

        V3R two = one + 1;
        TS_ASSERT_EQUALS(one, getWrappedVector(two));

        V3R three = one - 1;
        TS_ASSERT_EQUALS(one, getWrappedVector(three));

        V3R four = one - 10;
        TS_ASSERT_EQUALS(one, getWrappedVector(four));

        V3R five = one + 10;
        TS_ASSERT_EQUALS(one, getWrappedVector(five));
    }

    void testGetWrappedVectorV3D()
    {
        V3D one = V3D(0.5, 0.5, 0.5);
        TS_ASSERT_EQUALS(one, getWrappedVector(one));

        V3D two = one + V3D(1.0, 1.0, 1.0);
        TS_ASSERT_EQUALS(one, getWrappedVector(two));

        V3D three = one + V3D(1.0, 1.0, 1.0);
        TS_ASSERT_EQUALS(one, getWrappedVector(three));

        V3D four = one + V3D(10.0, 10.0, 10.0);
        TS_ASSERT_EQUALS(one, getWrappedVector(four));

        V3D five = one + V3D(10.0, 10.0, 10.0);
        TS_ASSERT_EQUALS(one, getWrappedVector(five));
    }

    void testGetOrderFromComponents()
    {
        TestableSymmetryOperation symOp;

        // identity - 0
        std::pair<Mantid::Kernel::IntMatrix, V3R> param1 = SymmetryOperationSymbolParser::parseIdentifier("x, y, z");
        TS_ASSERT_EQUALS(symOp.getOrderFromMatrix(param1.first), 1);

        // inversion - 1
        std::pair<Mantid::Kernel::IntMatrix, V3R> param2 = SymmetryOperationSymbolParser::parseIdentifier("-x, -y, -z");
        TS_ASSERT_EQUALS(symOp.getOrderFromMatrix(param2.first), 2);

        // mirror perpendicular to z
        std::pair<Mantid::Kernel::IntMatrix, V3R> param3 = SymmetryOperationSymbolParser::parseIdentifier("x, y, -z");
        TS_ASSERT_EQUALS(symOp.getOrderFromMatrix(param3.first), 2);

        // 4_1 screw axis along z
        std::pair<Mantid::Kernel::IntMatrix, V3R> param4 = SymmetryOperationSymbolParser::parseIdentifier("-y, x, z+1/4");
        TS_ASSERT_EQUALS(symOp.getOrderFromMatrix(param4.first), 4);

        // check that random matrices don't work
        Mantid::Kernel::IntMatrix randMatrix(3, 3, false);

        for(int i = 1; i < 10; ++i) {
            randMatrix.setRandom(-0, -i, i);
            TS_ASSERT_THROWS(symOp.getOrderFromMatrix(randMatrix), std::runtime_error);
        }
    }

    void testComparisonOperator()
    {
        SymmetryOperation inversion1("-x, -y, -z");
        SymmetryOperation inversion2("-x, -y, -z");


        TS_ASSERT(inversion1 == inversion2);
    }

    void testSymmetryOperations()
    {
        // Inversion
        SymmetryOperation inversionOp("-x, -y, -z");
        testSymmetryOperation(inversionOp,
                              2, m_hkl * -1.0, "-x,-y,-z");

        // 2-fold rotation around x
        SymmetryOperation twoFoldXOp("x, -y, -z");
        testSymmetryOperation(twoFoldXOp,
                              2, V3D(m_h, -m_k, -m_l), "x,-y,-z");

        // 6-fold rotation around [001] in hexagonal
        SymmetryOperation sixFoldZOp("x-y , x, z");
        testSymmetryOperation(sixFoldZOp,
                              6, V3D(-m_k, (m_h+m_k), m_l), "x-y,x,z");

    }

    void testPower()
    {
        SymmetryOperation mirror("x,-y,z");
        SymmetryOperation identity;

        TS_ASSERT_EQUALS(mirror^0, identity);
        TS_ASSERT_EQUALS(mirror^1, mirror);
        TS_ASSERT_EQUALS(mirror^2, identity);

        SymmetryOperation twoFoldZ("-x,-y,z");
        SymmetryOperation fourFoldZ("-y,x,z");
        TS_ASSERT_EQUALS(fourFoldZ^0, identity);
        TS_ASSERT_EQUALS(fourFoldZ^1, fourFoldZ);
        TS_ASSERT_EQUALS(fourFoldZ^2, twoFoldZ);
        TS_ASSERT_EQUALS(fourFoldZ^4, identity);
    }

private:
    V3D applyOrderTimes(const SymmetryOperation &symOp, const V3D &vector)
    {
        return applyNTimes(symOp, vector, symOp.order());
    }

    V3D applyLessThanOrderTimes(const SymmetryOperation &symOp, const V3D &vector)
    {
        return applyNTimes(symOp, vector, symOp.order() - 1);
    }

    V3D applyNTimes(const SymmetryOperation &symOp, const V3D &vector, size_t n)
    {
        V3D vectorCopy(vector);

        for(size_t i = 0; i < n; ++i) {
            vectorCopy = symOp * vectorCopy;
        }

        return vectorCopy;
    }

    void testSymmetryOperation(SymmetryOperation &symOp, size_t expectedOrder, const V3D &expectedHKL, const std::string &expectedIdentifier)
    {
        checkCorrectOrder(symOp, expectedOrder);
        checkCorrectTransformationGeneralHKL(symOp, expectedHKL);
        checkIdentifierString(symOp, expectedIdentifier);

        performCommonTests(symOp);

    }

    void checkCorrectOrder(const SymmetryOperation &symOp, size_t expected)
    {
        size_t order = symOp.order();

        TSM_ASSERT_EQUALS(symOp.identifier() + ": Order is " + boost::lexical_cast<std::string>(order) + ", expected " + boost::lexical_cast<std::string>(expected),
                          order, expected);
    }

    void checkCorrectTransformationGeneralHKL(const SymmetryOperation &symOp, const V3D &expected)
    {
        V3D transformed = symOp.transformHKL(m_hkl);

        TSM_ASSERT_EQUALS(symOp.identifier() + ": Transformed hkl is " + transformed.toString() + ", expected " + expected.toString(),
                          transformed, expected);
    }

    void checkIdentifierString(const SymmetryOperation &symOp, const std::string &expected)
    {
        std::string identifier = symOp.identifier();

        TSM_ASSERT_EQUALS(identifier + ": Does not match expected identifier " + expected,
                    identifier, expected);
    }

    void performCommonTests(const SymmetryOperation &symOp)
    {
        checkGeneralReflection(symOp);
        checkCorrectOrderAll(symOp);
        checkDeterminant(symOp);
    }

    void checkGeneralReflection(const SymmetryOperation &symOp)
    {
        V3D transformedOrderTimes = applyOrderTimes(symOp, m_hkl);

        TSM_ASSERT_EQUALS(symOp.identifier() + ": Transforming " + m_hkl.toString() + " $order times lead to unexpected result " + transformedOrderTimes.toString(),
                          transformedOrderTimes, m_hkl);

        V3D transformedLessThanOrderTimes = applyLessThanOrderTimes(symOp, m_hkl);
        TSM_ASSERT_DIFFERS(symOp.identifier() + ": Transforming " + m_hkl.toString() + " less than $order times lead to unexpected result " + transformedLessThanOrderTimes.toString(),
                          transformedLessThanOrderTimes, m_hkl);
    }

    void checkCorrectOrderAll(const SymmetryOperation &symOp)
    {
        for(size_t i = 0; i < m_allHkl.size(); ++i) {
            TS_ASSERT_EQUALS(applyOrderTimes(symOp, m_allHkl[i]), m_allHkl[i]);
        }
    }

    void checkDeterminant(const SymmetryOperation &symOp)
    {
        SymmetryOperation identity;

        SymmetryOperation symOpMatrix = symOp * identity;
        int determinant = abs(symOpMatrix.matrix().determinant());

        TSM_ASSERT_EQUALS(symOp.identifier() + ": Determinant of symmetry operation matrix is expected to be 1. Actual value: " + boost::lexical_cast<std::string>(determinant),
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
};


#endif /* MANTID_GEOMETRY_SYMMETRYOPERATIONTEST_H_ */
