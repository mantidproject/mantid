#ifndef MANTID_GEOMETRY_ISOTROPICATOMSCATTERERTEST_H_
#define MANTID_GEOMETRY_ISOTROPICATOMSCATTERERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/IsotropicAtomScatterer.h"
#include "MantidGeometry/Crystal/SpaceGroupFactory.h"

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class IsotropicAtomScattererTest : public CxxTest::TestSuite
{
public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static IsotropicAtomScattererTest *createSuite() { return new IsotropicAtomScattererTest(); }
    static void destroySuite( IsotropicAtomScattererTest *suite ) { delete suite; }


    void testConstructor()
    {
        TS_ASSERT_THROWS_NOTHING(IsotropicAtomScatterer scatterer("Si", V3D(0, 0, 0), 0.0));

        // bad symbol - throws whatever PhysicalConstants::getAtom() throws
        TS_ASSERT_THROWS_ANYTHING(IsotropicAtomScatterer scatterer("Random", V3D(0, 0, 0), 0.0));
    }

    void testGetSetElement()
    {
        IsotropicAtomScatterer scatterer("H", V3D(0, 0, 0), 0.0);

        TS_ASSERT_THROWS_NOTHING(scatterer.setElement("Si"));
        TS_ASSERT_EQUALS(scatterer.getElement(), "Si");
        TS_ASSERT_EQUALS(scatterer.getNeutronAtom().z_number, 14);

        TS_ASSERT_THROWS_ANYTHING(scatterer.setElement("Random"));
    }

    void testGetSetOccupancy()
    {
        IsotropicAtomScatterer scatterer("H", V3D(0, 0, 0), 0.0);

        TS_ASSERT_THROWS_NOTHING(scatterer.setOccupancy(0.3));
        TS_ASSERT_EQUALS(scatterer.getOccupancy(), 0.3);
        TS_ASSERT_THROWS_NOTHING(scatterer.setOccupancy(0.0));
        TS_ASSERT_THROWS_NOTHING(scatterer.setOccupancy(1.0));

        TS_ASSERT_THROWS(scatterer.setOccupancy(-0.3), std::invalid_argument);
        TS_ASSERT_THROWS(scatterer.setOccupancy(1.3), std::invalid_argument);
    }

    void testGetSetU()
    {
        IsotropicAtomScatterer scatterer("H", V3D(0, 0, 0), 0.0);

        TS_ASSERT_THROWS_NOTHING(scatterer.setU(0.0));
        TS_ASSERT_THROWS_NOTHING(scatterer.setU(1.0));
        TS_ASSERT_EQUALS(scatterer.getU(), 1.0);

        TS_ASSERT_THROWS_NOTHING(scatterer.setU(1.23e12));
        TS_ASSERT_THROWS_NOTHING(scatterer.setU(1.23e-2));

        TS_ASSERT_THROWS(scatterer.setU(-0.2), std::invalid_argument);
    }

    void testClone()
    {
        UnitCell cell(5.43, 5.43, 5.43);
        SpaceGroup_const_sptr spaceGroup = SpaceGroupFactory::Instance().createSpaceGroup("P m -3 m");


        IsotropicAtomScatterer_sptr scatterer = boost::make_shared<IsotropicAtomScatterer>("H", V3D(1.0, 0, 0), 0.0);
        scatterer->setU(3.04);
        scatterer->setOccupancy(0.5);
        scatterer->setCell(cell);
        scatterer->setSpaceGroup(spaceGroup);

        IScatterer_sptr clone = scatterer->clone();
        TS_ASSERT_EQUALS(clone->getPosition(), scatterer->getPosition());
        TS_ASSERT_EQUALS(clone->getCell().getG(), scatterer->getCell().getG());
        TS_ASSERT_EQUALS(clone->getSpaceGroup(), scatterer->getSpaceGroup());

        IsotropicAtomScatterer_sptr scattererClone = boost::dynamic_pointer_cast<IsotropicAtomScatterer>(clone);
        TS_ASSERT(scattererClone);

        TS_ASSERT_EQUALS(scattererClone->getU(), scatterer->getU());
        TS_ASSERT_EQUALS(scattererClone->getOccupancy(), scatterer->getOccupancy());
    }

};


#endif /* MANTID_GEOMETRY_ISOTROPICATOMSCATTERERTEST_H_ */
