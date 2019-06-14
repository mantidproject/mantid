// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
using Mantid::PhysicalConstants::NeutronAtom;
using Mantid::PhysicalConstants::getNeutronAtom;

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

  // common code for comparing scattering information for material made of a
  // single atom
  void checkMatching(const Material &material, const NeutronAtom &atom) {
    TS_ASSERT_DELTA(material.cohScatterXSection(), atom.coh_scatt_xs, 1.e-4);
    TS_ASSERT_DELTA(material.incohScatterXSection(), atom.inc_scatt_xs, 1.e-4);
    TS_ASSERT_DELTA(material.totalScatterXSection(), atom.tot_scatt_xs, 1.e-4);
    TS_ASSERT_DELTA(material.absorbXSection(), atom.abs_scatt_xs, 1.e-4);
    TS_ASSERT_DELTA(material.cohScatterLength(), atom.coh_scatt_length, 1.e-4);
    TS_ASSERT_DELTA(material.incohScatterLength(), atom.inc_scatt_length,
                    1.e-4);
    TS_ASSERT_DELTA(material.totalScatterLength(), atom.tot_scatt_length,
                    1.e-4);
    TS_ASSERT_DELTA(material.cohScatterLengthReal(), atom.coh_scatt_length_real,
                    1.e-4);
    TS_ASSERT_DELTA(material.cohScatterLengthImg(), atom.coh_scatt_length_img,
                    1.e-4);
    TS_ASSERT_DELTA(material.incohScatterLengthReal(),
                    atom.inc_scatt_length_real, 1.e-4);
    TS_ASSERT_DELTA(material.incohScatterLengthImg(), atom.inc_scatt_length_img,
                    1.e-4);
    const double cohReal = atom.coh_scatt_length_real;
    const double cohImag = atom.coh_scatt_length_img;
    TS_ASSERT_DELTA(material.cohScatterLengthSqrd(),
                    cohReal * cohReal + cohImag * cohImag, 1.e-4);
    const double totXS = atom.tot_scatt_xs;
    TS_ASSERT_DELTA(material.totalScatterLengthSqrd(), 25. * totXS / M_PI,
                    1.e-4);
  }

  void test_Vanadium() {
    const std::string name("Vanadium");
    NeutronAtom atom = getNeutronAtom(23);
    Material material(name, atom, 0.072);

    TS_ASSERT_EQUALS(material.name(), name);
    TS_ASSERT_EQUALS(material.numberDensity(), 0.072);
    TS_ASSERT_EQUALS(material.temperature(), 300);
    TS_ASSERT_EQUALS(material.pressure(),
                     Mantid::PhysicalConstants::StandardAtmosphere);

    // check everything with (default) reference wavelength
    checkMatching(material, atom);

    // check everything against another wavelength, only affects absorption
    const double lambda(2.1);
    TS_ASSERT_DELTA(material.cohScatterXSection(lambda),
                    material.cohScatterXSection(), 1e-04);
    TS_ASSERT_DELTA(material.incohScatterXSection(lambda),
                    material.incohScatterXSection(), 1e-04);
    TS_ASSERT_DELTA(material.absorbXSection(lambda), 5.93, 1e-02);
  }

  // highly absorbing material
  void test_Gadolinium() {
    const std::string name("Gadolinium");
    NeutronAtom atom = getNeutronAtom(64);
    Material material(name, atom, 0.0768); // mass density is 7.90 g/cm3
    TS_ASSERT_EQUALS(material.name(), name);

    // check everything with (default) reference wavelength
    checkMatching(material, atom);
    const double totLength = material.totalScatterLength();
    TS_ASSERT_DELTA(.04 * M_PI * totLength * totLength,
                    material.totalScatterXSection(), 1.e-4);
  }

  // "null scatterer" has only incoherent scattering
  void test_TiZr() {
    Material TiZr("TiZr", Material::parseChemicalFormula("Ti2.082605 Zr"),
                  0.542);

    TS_ASSERT_EQUALS(TiZr.cohScatterLengthImg(), 0.);
    TS_ASSERT_DELTA(TiZr.cohScatterLengthReal(), 0., 1.e-5);
    TS_ASSERT_EQUALS(
        TiZr.cohScatterLength(),
        TiZr.cohScatterLengthReal()); // there  is no imaginary part
    TS_ASSERT_DELTA(TiZr.cohScatterXSection(), 0., 1.e-5);

    TS_ASSERT_DELTA(TiZr.totalScatterXSection(),
                    TiZr.cohScatterXSection() + TiZr.incohScatterXSection(),
                    1.e-5);

    const double cohReal = TiZr.cohScatterLengthReal();
    const double cohImag = TiZr.cohScatterLengthImg();
    TS_ASSERT_DELTA(TiZr.cohScatterLengthSqrd(),
                    cohReal * cohReal + cohImag * cohImag, 1.e-4);

    const double totXS = TiZr.totalScatterXSection();
    TS_ASSERT_DELTA(TiZr.totalScatterLengthSqrd(), 25. * totXS / M_PI, 1.e-4);
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
                     const std::runtime_error &);
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
