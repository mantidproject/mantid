#ifndef MANTID_GEOMETRY_SCATTERERCOLLECTIONTEST_H_
#define MANTID_GEOMETRY_SCATTERERCOLLECTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/ScattererCollection.h"
#include "MantidGeometry/Crystal/UnitCell.h"
#include "MantidKernel/V3D.h"
#include "MantidGeometry/Crystal/SpaceGroupFactory.h"

#include "MantidGeometry/Crystal/IsotropicAtomScatterer.h"
#include "MantidKernel/Timer.h"

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;


class ScattererCollectionTest : public CxxTest::TestSuite
{
public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static ScattererCollectionTest *createSuite() { return new ScattererCollectionTest(); }
    static void destroySuite( ScattererCollectionTest *suite ) { delete suite; }


    void test_Something()
    {
        UnitCell cell(5.43, 6.43, 7.43, 90.0, 103.0, 90.0);

        SpaceGroup_const_sptr spaceGroup = SpaceGroupFactory::Instance().createSpaceGroup("P 1 2/m 1");

        ScattererCollection coll;

        std::vector<V3D> equiPos = spaceGroup->getEquivalentPositions(V3D(0.2, 0.3, 0.4));
        for(auto it = equiPos.begin(); it != equiPos.end(); ++it) {
            coll.addScatterer(boost::make_shared<IsotropicAtomScatterer>("Si", *it, cell, 0.01267));
        }

        std::cout << equiPos.size() << std::endl;

        std::vector<V3D> hkls;
        for(int h = 1; h <= 5; ++h) {
            for(int k = 0; k <= h; ++k) {
                for(int l = 0; l <= k; ++l) {
                    hkls.push_back(V3D(h, k, l));
                }
            }
        }

        for(auto it = hkls.begin(); it != hkls.end(); ++it) {
            double ampl = std::abs(coll.calculateStructureFactor(*it));
            double sqAmpl = ampl * ampl;

            if(sqAmpl > 1e-9) {
                std::cout << *it << " " << sqAmpl << std::endl;
            }
        }
    }

    void testMonoclinicCell()
    {
        UnitCell cell(5.43, 6.43, 7.43, 90.0, 103.0, 90.0);

        std::cout << cell.a() << " " << cell.astar() << std::endl;
        std::cout << cell.b() << " " << cell.bstar() << std::endl;
        std::cout << cell.c() << " " << cell.cstar() << std::endl;
    }


};


#endif /* MANTID_GEOMETRY_SCATTERERCOLLECTIONTEST_H_ */
