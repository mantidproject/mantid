#ifndef MANTID_GEOMETRY_ISOTROPICATOMBRAGGSCATTERERTEST_H_
#define MANTID_GEOMETRY_ISOTROPICATOMBRAGGSCATTERERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/IsotropicAtomBraggScatterer.h"
#include "MantidGeometry/Crystal/SpaceGroupFactory.h"

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class IsotropicAtomBraggScattererTest : public CxxTest::TestSuite
{
public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static IsotropicAtomBraggScattererTest *createSuite() { return new IsotropicAtomBraggScattererTest(); }
    static void destroySuite( IsotropicAtomBraggScattererTest *suite ) { delete suite; }


    void testConstructor()
    {
        TS_ASSERT_THROWS_NOTHING(IsotropicAtomBraggScatterer scatterer);
    }

    void testProperties()
    {
        IsotropicAtomBraggScatterer_sptr scatterer = boost::make_shared<IsotropicAtomBraggScatterer>();

        TS_ASSERT_THROWS_NOTHING(scatterer->initialize());

        TS_ASSERT(scatterer->existsProperty("Position"));
        TS_ASSERT(scatterer->existsProperty("SpaceGroup"));
        TS_ASSERT(scatterer->existsProperty("UnitCell"));
        TS_ASSERT(scatterer->existsProperty("U"));
        TS_ASSERT(scatterer->existsProperty("Element"));
        TS_ASSERT(scatterer->existsProperty("Occupancy"));

    }

    void testGetSetElement()
    {
        IsotropicAtomBraggScatterer_sptr scatterer = getInitializedScatterer();

        TS_ASSERT_THROWS_NOTHING(scatterer->setProperty("Element", "Si"));
        TS_ASSERT_EQUALS(scatterer->getElement(), "Si");
        TS_ASSERT_EQUALS(scatterer->getNeutronAtom().z_number, 14);

        TS_ASSERT_THROWS_ANYTHING(scatterer->setProperty("Element","Random"));
    }

    void testGetSetOccupancy()
    {
        IsotropicAtomBraggScatterer_sptr scatterer = getInitializedScatterer();

        TS_ASSERT_THROWS_NOTHING(scatterer->setProperty("Occupancy", 0.3));
        TS_ASSERT_EQUALS(scatterer->getOccupancy(), 0.3);
        TS_ASSERT_THROWS_NOTHING(scatterer->setProperty("Occupancy", 0.0));
        TS_ASSERT_THROWS_NOTHING(scatterer->setProperty("Occupancy", 1.0));

        TS_ASSERT_THROWS(scatterer->setProperty("Occupancy", -0.3), std::invalid_argument);
        TS_ASSERT_THROWS(scatterer->setProperty("Occupancy", 1.3), std::invalid_argument);
    }

    void testGetSetU()
    {
        IsotropicAtomBraggScatterer_sptr scatterer = getInitializedScatterer();

        TS_ASSERT_THROWS_NOTHING(scatterer->setProperty("U", 0.0));
        TS_ASSERT_THROWS_NOTHING(scatterer->setProperty("U", 1.0));
        TS_ASSERT_EQUALS(scatterer->getU(), 1.0);

        TS_ASSERT_THROWS_NOTHING(scatterer->setProperty("U", 1.23e12));
        TS_ASSERT_THROWS_NOTHING(scatterer->setProperty("U", 1.23e-2));

        TS_ASSERT_THROWS(scatterer->setProperty("U", -0.2), std::invalid_argument);
    }

    void testCreate()
    {
        IsotropicAtomBraggScatterer_sptr isotropic = getInitializedScatterer("Si", V3D(0.3, 0.1, 0.12), 1.0, 0.5);

        TS_ASSERT(isotropic);
        TS_ASSERT_EQUALS(isotropic->getElement(), "Si");
        TS_ASSERT_EQUALS(isotropic->getOccupancy(), 0.5);
        TS_ASSERT_EQUALS(isotropic->getU(), 1.0);
        TS_ASSERT_EQUALS(isotropic->getPosition(), V3D(0.3, 0.1, 0.12));
    }

    void testClone()
    {
        UnitCell cell(5.43, 5.43, 5.43);
        SpaceGroup_const_sptr spaceGroup = SpaceGroupFactory::Instance().createSpaceGroup("P m -3 m");

        IsotropicAtomBraggScatterer_sptr scatterer = getInitializedScatterer("H", V3D(1.0, 0, 0), 0.0);
        scatterer->setProperty("U", 3.04);
        scatterer->setProperty("Occupancy", 0.5);
        scatterer->setProperty("UnitCell", unitCellToStr(cell));
        scatterer->setProperty("SpaceGroup", spaceGroup->hmSymbol());

        BraggScatterer_sptr baseclone = scatterer->clone();
        BraggScattererInCrystalStructure_sptr clone = boost::dynamic_pointer_cast<BraggScattererInCrystalStructure>(baseclone);

        TS_ASSERT(clone)

        TS_ASSERT_EQUALS(clone->getPosition(), scatterer->getPosition());
        TS_ASSERT_EQUALS(clone->getCell().getG(), scatterer->getCell().getG());
        TS_ASSERT_EQUALS(clone->getSpaceGroup()->hmSymbol(), scatterer->getSpaceGroup()->hmSymbol());

        IsotropicAtomBraggScatterer_sptr scattererClone = boost::dynamic_pointer_cast<IsotropicAtomBraggScatterer>(clone);
        TS_ASSERT(scattererClone);

        TS_ASSERT_EQUALS(scattererClone->getU(), scatterer->getU());
        TS_ASSERT_EQUALS(scattererClone->getOccupancy(), scatterer->getOccupancy());
    }

    void testCalculateStructureFactor()
    {
        IsotropicAtomBraggScatterer_sptr scatterer = getInitializedScatterer("Si", V3D(0.0, 0.0, 0.0), 0.0);

        double bSi = scatterer->getNeutronAtom().coh_scatt_length_real;

        V3D hkl(1, 0, 0);

        // There's only one atom in (0,0,0) and U is 0 - rigid scatterer
        StructureFactor structureFactor = scatterer->calculateStructureFactor(hkl);

        /* Phase is (1,0,0) * (0,0,0) = (1*0 + 0*0 + 0*0) = 0
         * cos(phase) = 1.0
         * sin(phase) = 0.0
         */
        TS_ASSERT_EQUALS(structureFactor.real(), bSi);
        TS_ASSERT_EQUALS(structureFactor.imag(), 0.0);

        // For using U, the cell needs to be set, because 1/d is required
        UnitCell cell(5.43, 5.43, 5.43);
        scatterer->setProperty("UnitCell", unitCellToStr(cell));
        scatterer->setProperty("U", 0.05);

        structureFactor = scatterer->calculateStructureFactor(hkl);
        /* Real part is reduced by exp(-U * 2*pi^2 * 1/d^2)
         * d = 5.43, d^2 = 29.4849, 1/d^2 = 0.033916...
         * exp(-0.05 * 2 * pi^2 * 1/29.4849) = 0.96708...
         */
        TS_ASSERT_EQUALS(structureFactor.real(), bSi * 0.96708061593352515459);

        // Occupancy goes in directly
        scatterer->setProperty("Occupancy", 0.5);
        structureFactor = scatterer->calculateStructureFactor(hkl);
        TS_ASSERT_EQUALS(structureFactor.real(), bSi * 0.5 * 0.96708061593352515459);

        // Set a space group with F-centering
        SpaceGroup_const_sptr spaceGroup = SpaceGroupFactory::Instance().createSpaceGroup("F m -3 m");
        scatterer->setProperty("SpaceGroup", spaceGroup->hmSymbol());

        /* Now there are 4 equivalent positions, the contributions cancel out for (1, 0, 0)
         * scalar products are:
         *   (1,0,0) * (0,0,0) = (1*0 + 1*0 + 1*0)         = 0    cos(0) = 1, sin(0) = 0
         *   (1,0,0) * (0,0.5,0.5) = (1*0 + 0*0.5 + 0*0.5) = 0    cos(0) = 1, sin(0) = 0
         *   (1,0,0) * (0.5,0,0.5) = (1*0.5 + 0*0 + 0*0.5) = 0.5  cos(pi) = -1, sin(pi) = 0
         *   (1,0,0) * (0.5,0.5,0) = (1*0.5 + 0*0.5 + 0*0) = 0.5  cos(pi) = -1, sin(pi) = 0
         *
         *   That means 1 * real + 1 * real + (-1 * real) + (-1 * real) = 0
         */
        structureFactor = scatterer->calculateStructureFactor(hkl);

        // It's not always exactly 0 (floating point math), but should not be less than 0
        TS_ASSERT_LESS_THAN(structureFactor.real(), 1e-9);
        TS_ASSERT_LESS_THAN_EQUALS(0, structureFactor.real());

        // For (1, 1, 1), the value is defined
        hkl = V3D(1, 1, 1);
        structureFactor = scatterer->calculateStructureFactor(hkl);

        /* scalar products are:
         *   (1,1,1) * (0,0,0) = (1*0 + 1*0 + 1*0)         = 0  cos(0) = 1, sin(0) = 0
         *   (1,1,1) * (0,0.5,0.5) = (1*0 + 1*0.5 + 1*0.5) = 1  cos(2pi) = 1, sin(2pi) = 0
         *   (1,1,1) * (0.5,0,0.5) = (1*0.5 + 1*0 + 1*0.5) = 1  cos(2pi) = 1, sin(2pi) = 0
         *   (1,1,1) * (0.5,0.5,0) = (1*0.5 + 1*0.5 + 1*0) = 1  cos(2pi) = 1, sin(2pi) = 0
         *
         * That means 4 * real * debye waller * occupation. d = 3.13...
         */
        TS_ASSERT_DELTA(structureFactor.real(), 4.0 * bSi * 0.90445723107190849637 * 0.5, 5e-16)
    }

private:
    IsotropicAtomBraggScatterer_sptr getInitializedScatterer()
    {
        IsotropicAtomBraggScatterer_sptr scatterer = boost::make_shared<IsotropicAtomBraggScatterer>();
        scatterer->initialize();

        return scatterer;
    }

    IsotropicAtomBraggScatterer_sptr getInitializedScatterer(const std::string &element, const V3D &position, double U = 0.0, double occ = 1.0)
    {
        IsotropicAtomBraggScatterer_sptr scatterer = getInitializedScatterer();

        scatterer->setProperty("Element", element);
        scatterer->setProperty("Position", position);
        scatterer->setProperty("U", U);
        scatterer->setProperty("Occupancy", occ);

        return scatterer;
    }

};


#endif /* MANTID_GEOMETRY_ISOTROPICATOMBRAGGSCATTERERTEST_H_ */
