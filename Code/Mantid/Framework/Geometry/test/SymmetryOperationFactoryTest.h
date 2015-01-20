#ifndef MANTID_GEOMETRY_SYMMETRYOPERATIONFACTORYTEST_H_
#define MANTID_GEOMETRY_SYMMETRYOPERATIONFACTORYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/Exception.h"

#include <boost/lexical_cast.hpp>

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;


class SymmetryOperationFactoryTest : public CxxTest::TestSuite
{
public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static SymmetryOperationFactoryTest *createSuite() { return new SymmetryOperationFactoryTest(); }
    static void destroySuite( SymmetryOperationFactoryTest *suite ) { delete suite; }

    SymmetryOperationFactoryTest()
    {
        SymmetryOperationFactory::Instance().subscribeSymOp("x,y,z");
    }

    ~SymmetryOperationFactoryTest()
    {
        SymmetryOperationFactory::Instance().unsubscribeSymOp("x,y,z");
    }


    void testCreateSymOp()
    {
        TS_ASSERT_THROWS_NOTHING(SymmetryOperationFactory::Instance().createSymOp("x,y,z"));
        TS_ASSERT_THROWS(SymmetryOperationFactory::Instance().createSymOp("fake2"), Mantid::Kernel::Exception::ParseError);

        // createSymOp also works when an operation is not subscribed
        TS_ASSERT_THROWS_NOTHING(SymmetryOperationFactory::Instance().unsubscribeSymOp("x,y,z"));
        TS_ASSERT_EQUALS(SymmetryOperationFactory::Instance().isSubscribed("x,y,z"), false);

        TS_ASSERT_THROWS_NOTHING(SymmetryOperationFactory::Instance().createSymOp("x,y,z"));

        // it's automatically registered
        TS_ASSERT_EQUALS(SymmetryOperationFactory::Instance().isSubscribed("x,y,z"), true);
    }

    void testCreateSymOpsVector()
    {
        std::vector<std::string> opStrings;
        opStrings.push_back("x,y,z");

        std::vector<SymmetryOperation> symOps = SymmetryOperationFactory::Instance().createSymOps(opStrings);
        TS_ASSERT_EQUALS(symOps.size(), 1);
        TS_ASSERT_EQUALS(symOps.front().identifier(), "x,y,z");

        // Add another one
        opStrings.push_back("-x,-y,-z");

        TS_ASSERT_THROWS_NOTHING(symOps = SymmetryOperationFactory::Instance().createSymOps(opStrings));
        TS_ASSERT_EQUALS(symOps.size(), 2);
        TS_ASSERT_EQUALS(symOps.front().identifier(), "x,y,z");
        TS_ASSERT_EQUALS(symOps.back().identifier(), "-x,-y,-z");

        opStrings.push_back("doesNotWork");
        TS_ASSERT_THROWS(symOps = SymmetryOperationFactory::Instance().createSymOps(opStrings), Mantid::Kernel::Exception::ParseError);
    }

    void testCreateSymOpsString()
    {
        std::string validOne("-x,-y,-z");
        std::string validTwo("-x,-y,-z; x+1/2,y+1/2,z+1/2");
        std::string validThree("-x,-y,-z; x+1/2,y+1/2,z+1/2; x,-y,z");

        TS_ASSERT_THROWS_NOTHING(SymmetryOperationFactory::Instance().createSymOps(validOne));
        TS_ASSERT_THROWS_NOTHING(SymmetryOperationFactory::Instance().createSymOps(validTwo));
        TS_ASSERT_THROWS_NOTHING(SymmetryOperationFactory::Instance().createSymOps(validThree));

        std::string invalidSep("-x,-y,-z | x+1/2,y+1/2,z+1/2");
        std::string invalidOne("-x,-y,-z; invalid");

        TS_ASSERT_THROWS(SymmetryOperationFactory::Instance().createSymOps(invalidSep), Mantid::Kernel::Exception::ParseError);
        TS_ASSERT_THROWS(SymmetryOperationFactory::Instance().createSymOps(invalidOne), Mantid::Kernel::Exception::ParseError);
    }

    void testUnsubscribe()
    {
        TS_ASSERT_EQUALS(SymmetryOperationFactory::Instance().isSubscribed("x,y,z"), true);

        TS_ASSERT_THROWS_NOTHING(SymmetryOperationFactory::Instance().unsubscribeSymOp("x,y,z"));
        TS_ASSERT_EQUALS(SymmetryOperationFactory::Instance().isSubscribed("x,y,z"), false);

        TS_ASSERT_THROWS_NOTHING(SymmetryOperationFactory::Instance().subscribeSymOp("x,y,z"));
    }

    void testIsSubscribed()
    {
        TS_ASSERT_THROWS_NOTHING(SymmetryOperationFactory::Instance().unsubscribeSymOp("x,y,z"));
        TS_ASSERT_EQUALS(SymmetryOperationFactory::Instance().isSubscribed("x,y,z"), false);
        TS_ASSERT_THROWS_NOTHING(SymmetryOperationFactory::Instance().subscribeSymOp("x,y,z"));
        TS_ASSERT_EQUALS(SymmetryOperationFactory::Instance().isSubscribed("x,y,z"), true);
    }

    void testSubscribedSymbols()
    {
        // Clear factory
        std::vector<std::string> allSymbols = SymmetryOperationFactory::Instance().subscribedSymbols();
        for(auto it = allSymbols.begin(); it != allSymbols.end(); ++it) {
            SymmetryOperationFactory::Instance().unsubscribeSymOp(*it);
        }

        // Subscribe two symmetry operations
        SymmetryOperationFactory::Instance().subscribeSymOp("x,y,z");
        SymmetryOperationFactory::Instance().subscribeSymOp("-x,-y,-z");

        std::vector<std::string> symbols = SymmetryOperationFactory::Instance().subscribedSymbols();

        TS_ASSERT_EQUALS(symbols.size(), 2);
        TS_ASSERT_DIFFERS(std::find(symbols.begin(), symbols.end(), "x,y,z"), symbols.end());
        TS_ASSERT_DIFFERS(std::find(symbols.begin(), symbols.end(), "-x,-y,-z"), symbols.end());

        SymmetryOperationFactory::Instance().unsubscribeSymOp("x,y,z");
        SymmetryOperationFactory::Instance().unsubscribeSymOp("-x,-y,-z");

        // Restore factory
        for(auto it = allSymbols.begin(); it != allSymbols.end(); ++it) {
            SymmetryOperationFactory::Instance().subscribeSymOp(*it);
        }
    }

    void testSymmetryElement()
    {
        SymmetryOperation twoFoldY = SymmetryOperationFactory::Instance().createSymOp("x-y,x,z+1/6");

        size_t k = twoFoldY.order();
        IntMatrix sumMatrix(3, 3, true);
        for(size_t i = (k - 1); i > 0; --i) {
            std::cout << i << std::endl;
            sumMatrix += (twoFoldY^i).matrix();
        }

        V3R vector = twoFoldY.vector();

        V3R screw = (sumMatrix * vector) / k;

        std::cout << screw << std::endl;

        IntMatrix matrix = twoFoldY.matrix();
        std::vector<int> vect = matrix;
        std::vector<double> dvec(vect.size());
        for(size_t i = 0; i < vect.size(); ++i) {
            dvec[i] = static_cast<double>(vect[i]);
        }

        DblMatrix dblMatrix(dvec);

        DblMatrix eigenValues;
        DblMatrix eigenVectors;

        dblMatrix.averSymmetric();
        dblMatrix.Diagonalise(eigenVectors, eigenValues);
        std::cout << eigenValues << std::endl;
        std::cout << eigenVectors << std::endl;

    }
};


#endif /* MANTID_GEOMETRY_SYMMETRYOPERATIONFACTORYTEST_H_ */
