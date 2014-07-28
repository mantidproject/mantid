#ifndef MANTID_GEOMETRY_SYMMETRYOPERATIONTEST_H_
#define MANTID_GEOMETRY_SYMMETRYOPERATIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/SymmetryOperation.h"
#include "MantidKernel/V3D.h"

#include <boost/make_shared.hpp>

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class SymmetryOperationTest : public CxxTest::TestSuite
{
public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static SymmetryOperationTest *createSuite() { return new SymmetryOperationTest(); }
    static void destroySuite( SymmetryOperationTest *suite ) { delete suite; }

    SymmetryOperationTest() :
        m_hkl(3.0, 2.0, 4.0),
        m_hhl(3.0, 3.0, 4.0),
        m_hk0(3.0, 2.0, 0.0),
        m_h00(3.0, 0.0, 0.0),
        m_allHkl()
    {
        m_allHkl.push_back(m_hkl);
        m_allHkl.push_back(m_hhl);
        m_allHkl.push_back(m_hk0);
        m_allHkl.push_back(m_h00);
    }


    void testIdentity()
    {
        auto identity = boost::make_shared<const SymOpIdentity>();
        TS_ASSERT_EQUALS(identity->order(), 1);

        TS_ASSERT_EQUALS(checkCorrectOrder(identity, m_hkl), m_hkl);

        checkCorrectOrderAll(identity);
    }

    void testInversion()
    {
        auto inversion = boost::make_shared<const SymOpInversion>();
        TS_ASSERT_EQUALS(inversion->order(), 2);

        TS_ASSERT_EQUALS(checkCorrectOrder(inversion, m_hkl), m_hkl);

        checkCorrectOrderAll(inversion);
    }

private:
    V3D checkCorrectOrder(const SymmetryOperation_const_sptr &symOp, const V3D &vector)
    {
        V3D vectorCopy(vector);

        for(size_t i = 0; i < symOp->order(); ++i) {
            vectorCopy = symOp->apply(vectorCopy);
        }

        return vectorCopy;
    }

    void checkCorrectOrderAll(const SymmetryOperation_const_sptr &symOp)
    {
        for(size_t i = 0; i < m_allHkl.size(); ++i) {
            TS_ASSERT_EQUALS(checkCorrectOrder(symOp, m_allHkl[i]), m_allHkl[i]);
        }
    }


    // hkls to test
    V3D m_hkl;
    V3D m_hhl;
    V3D m_hk0;
    V3D m_h00;

    std::vector<V3D> m_allHkl;
};


#endif /* MANTID_GEOMETRY_SYMMETRYOPERATIONTEST_H_ */
