#ifndef MANTID_TESTMATERIAL__
#define MANTID_TESTMATERIAL__

#include <cxxtest/TestSuite.h>
#include <cmath>
#include <stdexcept>

#include "MantidKernel/Material.h"
#include "MantidKernel/NeutronAtom.h"
#include "MantidTestHelpers/NexusTestHelper.h"

using Mantid::Kernel::Material;

class MaterialTest: public CxxTest::TestSuite
{
public:

  void test_Empty_Constructor()
  {
    Material empty;
    TS_ASSERT_EQUALS(empty.name(), "");
    TS_ASSERT_EQUALS(empty.numberDensity(), 0.0);
    TS_ASSERT_EQUALS(empty.temperature(), 0.0);
    TS_ASSERT_EQUALS(empty.pressure(), 0.0);

    const double lambda(2.1);
    TS_ASSERT_EQUALS(empty.cohScatterXSection(lambda), 0.0);
    TS_ASSERT_EQUALS(empty.incohScatterXSection(lambda), 0.0);
    TS_ASSERT_EQUALS(empty.absorbXSection(lambda), 0.0);
    
  }

  void test_That_Construction_By_Known_Element_Gives_Expected_Values()
  {
    Material vanBlock("vanBlock", Mantid::PhysicalConstants::getNeutronAtom(23, 0), 0.072);

    TS_ASSERT_EQUALS(vanBlock.name(), "vanBlock");
    TS_ASSERT_EQUALS(vanBlock.numberDensity(), 0.072);
    TS_ASSERT_EQUALS(vanBlock.temperature(), 300);
    TS_ASSERT_EQUALS(vanBlock.pressure(), Mantid::PhysicalConstants::StandardAtmosphere);

    const double lambda(2.1);
    TS_ASSERT_DELTA(vanBlock.cohScatterXSection(lambda), 0.0184,  1e-02);
    TS_ASSERT_DELTA(vanBlock.incohScatterXSection(lambda), 5.08,  1e-02);
    TS_ASSERT_DELTA(vanBlock.absorbXSection(lambda), 5.93, 1e-02);
  }

  /** Save then re-load from a NXS file */
  void test_nexus()
  {
    Material testA("testMaterial", Mantid::PhysicalConstants::getNeutronAtom(23, 0), 0.072, 273, 1.234);
    NexusTestHelper th(true);
    th.createFile("MaterialTest.nxs");

    TS_ASSERT_THROWS_NOTHING( testA.saveNexus(th.file, "material"); );

    Material testB;
    th.reopenFile();
    TS_ASSERT_THROWS_NOTHING( testB.loadNexus(th.file, "material"); );

    TS_ASSERT_EQUALS(testB.name(), "testMaterial");
    TS_ASSERT_DELTA(testB.numberDensity(), 0.072, 1e-6);
    TS_ASSERT_DELTA(testB.temperature(), 273, 1e-6);
    TS_ASSERT_DELTA(testB.pressure(), 1.234, 1e-6);
    // This (indirectly) checks that the right element was found
    const double lambda(2.1);
    TS_ASSERT_DELTA(testB.cohScatterXSection(lambda), 0.0184,  1e-02);
    TS_ASSERT_DELTA(testB.incohScatterXSection(lambda), 5.08,  1e-02);
    TS_ASSERT_DELTA(testB.absorbXSection(lambda), 5.93, 1e-02);
  }

  void test_nexus_emptyMaterial()
  {
    Material testA;
    NexusTestHelper th(true);
    th.createFile("MaterialTest.nxs");
    TS_ASSERT_THROWS_NOTHING( testA.saveNexus(th.file, "material"); );
    Material testB;
    th.reopenFile();
    TS_ASSERT_THROWS_NOTHING( testB.loadNexus(th.file, "material"); );
  }

  void test_parseMaterial()
  {
    Material::ChemicalFormula cf;

    cf = Material::parseChemicalFormula("F60");
    TS_ASSERT_EQUALS(cf.atoms.size(), 1);
    TS_ASSERT_EQUALS(cf.atoms[0], "F");
    TS_ASSERT_EQUALS(cf.aNumbers[0], 0);
    TS_ASSERT_EQUALS(cf.numberAtoms[0], 60);

    cf = Material::parseChemicalFormula("(F60)");
    TS_ASSERT_EQUALS(cf.atoms.size(), 1);
    TS_ASSERT_EQUALS(cf.atoms[0], "F");
    TS_ASSERT_EQUALS(cf.aNumbers[0], 60);
    TS_ASSERT_EQUALS(cf.numberAtoms[0], 1);

    cf = Material::parseChemicalFormula("C60");
    TS_ASSERT_EQUALS(cf.atoms.size(), 1);
    TS_ASSERT_EQUALS(cf.atoms[0], "C");
    TS_ASSERT_EQUALS(cf.aNumbers[0], 0);
    TS_ASSERT_EQUALS(cf.numberAtoms[0], 60);

    cf = Material::parseChemicalFormula("(C60)");
    TS_ASSERT_EQUALS(cf.atoms.size(), 1);
    TS_ASSERT_EQUALS(cf.atoms[0], "C");
    TS_ASSERT_EQUALS(cf.aNumbers[0], 60);
    TS_ASSERT_EQUALS(cf.numberAtoms[0], 1);

    cf = Material::parseChemicalFormula("H2O");
    TS_ASSERT_EQUALS(cf.atoms.size(), 2);
    TS_ASSERT_EQUALS(cf.atoms[0], "H");
    TS_ASSERT_EQUALS(cf.aNumbers[0], 0);
    TS_ASSERT_EQUALS(cf.numberAtoms[0], 2);
    TS_ASSERT_EQUALS(cf.atoms[1], "O");
    TS_ASSERT_EQUALS(cf.aNumbers[1], 0);
    TS_ASSERT_EQUALS(cf.numberAtoms[1], 1);

    cf = Material::parseChemicalFormula("(H1)2O");
    TS_ASSERT_EQUALS(cf.atoms.size(), 2);
    TS_ASSERT_EQUALS(cf.atoms[0], "H");
    TS_ASSERT_EQUALS(cf.aNumbers[0], 1);
    TS_ASSERT_EQUALS(cf.numberAtoms[0], 2);
    TS_ASSERT_EQUALS(cf.atoms[1], "O");
    TS_ASSERT_EQUALS(cf.aNumbers[1], 0);
    TS_ASSERT_EQUALS(cf.numberAtoms[1], 1);

    cf = Material::parseChemicalFormula("D2O");
    TS_ASSERT_EQUALS(cf.atoms.size(), 2);
    TS_ASSERT_EQUALS(cf.atoms[0], "H");
    TS_ASSERT_EQUALS(cf.aNumbers[0], 2);
    TS_ASSERT_EQUALS(cf.numberAtoms[0], 2);
    TS_ASSERT_EQUALS(cf.atoms[1], "O");
    TS_ASSERT_EQUALS(cf.aNumbers[1], 0);
    TS_ASSERT_EQUALS(cf.numberAtoms[1], 1);

    cf = Material::parseChemicalFormula("H2 O");
    TS_ASSERT_EQUALS(cf.atoms.size(), 2);
    TS_ASSERT_EQUALS(cf.atoms[0], "H");
    TS_ASSERT_EQUALS(cf.aNumbers[0], 0);
    TS_ASSERT_EQUALS(cf.numberAtoms[0], 2);
    TS_ASSERT_EQUALS(cf.atoms[1], "O");
    TS_ASSERT_EQUALS(cf.aNumbers[1], 0);
    TS_ASSERT_EQUALS(cf.numberAtoms[1], 1);

    cf = Material::parseChemicalFormula("H2-O");
    TS_ASSERT_EQUALS(cf.atoms.size(), 2);
    TS_ASSERT_EQUALS(cf.atoms[0], "H");
    TS_ASSERT_EQUALS(cf.aNumbers[0], 0);
    TS_ASSERT_EQUALS(cf.numberAtoms[0], 2);
    TS_ASSERT_EQUALS(cf.atoms[1], "O");
    TS_ASSERT_EQUALS(cf.aNumbers[1], 0);
    TS_ASSERT_EQUALS(cf.numberAtoms[1], 1);

    TS_ASSERT_THROWS(cf = Material::parseChemicalFormula("H2*O"), std::runtime_error);
    TS_ASSERT_EQUALS(cf.atoms.size(), 2);
    TS_ASSERT_EQUALS(cf.atoms[0], "H");
    TS_ASSERT_EQUALS(cf.aNumbers[0], 0);
    TS_ASSERT_EQUALS(cf.numberAtoms[0], 2);
    TS_ASSERT_EQUALS(cf.atoms[1], "O");
    TS_ASSERT_EQUALS(cf.aNumbers[1], 0);
    TS_ASSERT_EQUALS(cf.numberAtoms[1], 1);
  }

};
#endif
