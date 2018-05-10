#ifndef MANTID_TESTMATERIAL__
#define MANTID_TESTMATERIAL__

#include <cmath>
#include <cxxtest/TestSuite.h>
#include <stdexcept>

#include "MantidKernel/Atom.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/NeutronAtom.h"
#include "MantidTestHelpers/NexusTestHelper.h"

using Mantid::Kernel::Material;

class MaterialTest : public CxxTest::TestSuite {
public:
  void test_Empty_Constructor() {
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

  void test_That_Construction_By_Known_Element_Gives_Expected_Values() {
    Material vanBlock("vanBlock",
                      Mantid::PhysicalConstants::getNeutronAtom(23, 0), 0.072);

    TS_ASSERT_EQUALS(vanBlock.name(), "vanBlock");
    TS_ASSERT_EQUALS(vanBlock.numberDensity(), 0.072);
    TS_ASSERT_EQUALS(vanBlock.temperature(), 300);
    TS_ASSERT_EQUALS(vanBlock.pressure(),
                     Mantid::PhysicalConstants::StandardAtmosphere);

    const double lambda(2.1);
    TS_ASSERT_DELTA(vanBlock.cohScatterXSection(lambda), 0.0184, 1e-02);
    TS_ASSERT_DELTA(vanBlock.incohScatterXSection(lambda), 5.08, 1e-02);
    TS_ASSERT_DELTA(vanBlock.absorbXSection(lambda), 5.93, 1e-02);
  }

  /** Save then re-load from a NXS file */
  void test_nexus() {
    Material testA("testMaterial",
                   Mantid::PhysicalConstants::getNeutronAtom(23, 0), 0.072, 273,
                   1.234);
    NexusTestHelper th(true);
    th.createFile("MaterialTest.nxs");

    TS_ASSERT_THROWS_NOTHING(testA.saveNexus(th.file, "material"););

    Material testB;
    th.reopenFile();
    TS_ASSERT_THROWS_NOTHING(testB.loadNexus(th.file, "material"););

    TS_ASSERT_EQUALS(testB.name(), "testMaterial");
    TS_ASSERT_DELTA(testB.numberDensity(), 0.072, 1e-6);
    TS_ASSERT_DELTA(testB.temperature(), 273, 1e-6);
    TS_ASSERT_DELTA(testB.pressure(), 1.234, 1e-6);
    // This (indirectly) checks that the right element was found
    const double lambda(2.1);
    TS_ASSERT_DELTA(testB.cohScatterXSection(lambda), 0.0184, 1e-02);
    TS_ASSERT_DELTA(testB.incohScatterXSection(lambda), 5.08, 1e-02);
    TS_ASSERT_DELTA(testB.absorbXSection(lambda), 5.93, 1e-02);
  }

  void test_nexus_emptyMaterial() {
    Material testA;
    NexusTestHelper th(true);
    th.createFile("MaterialTest.nxs");
    TS_ASSERT_THROWS_NOTHING(testA.saveNexus(th.file, "material"););
    Material testB;
    th.reopenFile();
    TS_ASSERT_THROWS_NOTHING(testB.loadNexus(th.file, "material"););
  }

  void test_parseMaterial() {
    Material::ChemicalFormula cf;

    cf = Material::parseChemicalFormula("F14");
    TS_ASSERT_EQUALS(cf.size(), 1);
    TS_ASSERT_EQUALS(cf[0].atom->symbol, "F");
    TS_ASSERT_EQUALS(cf[0].atom->a_number, 0);
    TS_ASSERT_EQUALS(cf[0].multiplicity, 14);

    cf = Material::parseChemicalFormula("(F14)");
    TS_ASSERT_EQUALS(cf.size(), 1);
    TS_ASSERT_EQUALS(cf[0].atom->symbol, "F");
    TS_ASSERT_EQUALS(cf[0].atom->a_number, 14);
    TS_ASSERT_EQUALS(cf[0].multiplicity, 1);

    cf = Material::parseChemicalFormula("C15");
    TS_ASSERT_EQUALS(cf.size(), 1);
    TS_ASSERT_EQUALS(cf[0].atom->symbol, "C");
    TS_ASSERT_EQUALS(cf[0].atom->a_number, 0);
    TS_ASSERT_EQUALS(cf[0].multiplicity, 15);

    cf = Material::parseChemicalFormula("(C15)");
    TS_ASSERT_EQUALS(cf.size(), 1);
    TS_ASSERT_EQUALS(cf[0].atom->symbol, "C");
    TS_ASSERT_EQUALS(cf[0].atom->a_number, 15);
    TS_ASSERT_EQUALS(cf[0].multiplicity, 1);

    cf = Material::parseChemicalFormula("H2 O");
    TS_ASSERT_EQUALS(cf.size(), 2);
    TS_ASSERT_EQUALS(cf[0].atom->symbol, "H");
    TS_ASSERT_EQUALS(cf[0].atom->a_number, 0);
    TS_ASSERT_EQUALS(cf[0].multiplicity, 2);
    TS_ASSERT_EQUALS(cf[1].atom->symbol, "O");
    TS_ASSERT_EQUALS(cf[1].atom->a_number, 0);
    TS_ASSERT_EQUALS(cf[1].multiplicity, 1);

    cf = Material::parseChemicalFormula("(H1)2 O");
    TS_ASSERT_EQUALS(cf.size(), 2);
    TS_ASSERT_EQUALS(cf[0].atom->symbol, "H");
    TS_ASSERT_EQUALS(cf[0].atom->a_number, 1);
    TS_ASSERT_EQUALS(cf[0].multiplicity, 2);
    TS_ASSERT_EQUALS(cf[1].atom->symbol, "O");
    TS_ASSERT_EQUALS(cf[1].atom->a_number, 0);
    TS_ASSERT_EQUALS(cf[1].multiplicity, 1);

    cf = Material::parseChemicalFormula("D2 O");
    TS_ASSERT_EQUALS(cf.size(), 2);
    TS_ASSERT_EQUALS(cf[0].atom->symbol, "H");
    TS_ASSERT_EQUALS(cf[0].atom->a_number, 2);
    TS_ASSERT_EQUALS(cf[0].multiplicity, 2);
    TS_ASSERT_EQUALS(cf[1].atom->symbol, "O");
    TS_ASSERT_EQUALS(cf[1].atom->a_number, 0);
    TS_ASSERT_EQUALS(cf[1].multiplicity, 1);

    cf = Material::parseChemicalFormula("H2 O");
    TS_ASSERT_EQUALS(cf.size(), 2);
    TS_ASSERT_EQUALS(cf[0].atom->symbol, "H");
    TS_ASSERT_EQUALS(cf[0].atom->a_number, 0);
    TS_ASSERT_EQUALS(cf[0].multiplicity, 2);
    TS_ASSERT_EQUALS(cf[1].atom->symbol, "O");
    TS_ASSERT_EQUALS(cf[1].atom->a_number, 0);
    TS_ASSERT_EQUALS(cf[1].multiplicity, 1);

    cf = Material::parseChemicalFormula("H2-O");
    TS_ASSERT_EQUALS(cf.size(), 2);
    TS_ASSERT_EQUALS(cf[0].atom->symbol, "H");
    TS_ASSERT_EQUALS(cf[0].atom->a_number, 0);
    TS_ASSERT_EQUALS(cf[0].multiplicity, 2);
    TS_ASSERT_EQUALS(cf[1].atom->symbol, "O");
    TS_ASSERT_EQUALS(cf[1].atom->a_number, 0);
    TS_ASSERT_EQUALS(cf[1].multiplicity, 1);

    TS_ASSERT_THROWS(cf = Material::parseChemicalFormula("H2*O"),
                     std::runtime_error);
    TS_ASSERT_EQUALS(cf.size(), 2);
    TS_ASSERT_EQUALS(cf[0].atom->symbol, "H");
    TS_ASSERT_EQUALS(cf[0].atom->a_number, 0);
    TS_ASSERT_EQUALS(cf[0].multiplicity, 2);
    TS_ASSERT_EQUALS(cf[1].atom->symbol, "O");
    TS_ASSERT_EQUALS(cf[1].atom->a_number, 0);
    TS_ASSERT_EQUALS(cf[1].multiplicity, 1);

    cf = Material::parseChemicalFormula("(Li7)2");
    TS_ASSERT_EQUALS(cf.size(), 1);
    TS_ASSERT_EQUALS(cf[0].atom->symbol, "Li");
    TS_ASSERT_EQUALS(cf[0].atom->a_number, 7);
    TS_ASSERT_EQUALS(cf[0].multiplicity, 2);

    cf = Material::parseChemicalFormula("Y-Ba2-Cu3-O6.56");
    TS_ASSERT_EQUALS(cf.size(), 4);
    for (const auto &formulaUnit : cf) {
      TS_ASSERT_EQUALS(formulaUnit.atom->a_number, 0);
    }
    TS_ASSERT_EQUALS(cf[0].atom->symbol, "Y");
    TS_ASSERT_EQUALS(cf[0].multiplicity, 1);
    TS_ASSERT_EQUALS(cf[1].atom->symbol, "Ba");
    TS_ASSERT_EQUALS(cf[1].multiplicity, 2);
    TS_ASSERT_EQUALS(cf[2].atom->symbol, "Cu");
    TS_ASSERT_EQUALS(cf[2].multiplicity, 3);
    TS_ASSERT_EQUALS(cf[3].atom->symbol, "O");
    TS_ASSERT_DELTA(cf[3].multiplicity, 6.56, .01);
  }
};
#endif
