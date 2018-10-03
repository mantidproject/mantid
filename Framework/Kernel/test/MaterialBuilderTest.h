// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_MATERIALBUILDERTEST_H_
#define MANTID_KERNEL_MATERIALBUILDERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/MaterialBuilder.h"
#include "MantidKernel/NeutronAtom.h"

using Mantid::Kernel::Material;
using Mantid::Kernel::MaterialBuilder;
using Mantid::PhysicalConstants::NeutronAtom;

class MaterialBuilderTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MaterialBuilderTest *createSuite() {
    return new MaterialBuilderTest();
  }
  static void destroySuite(MaterialBuilderTest *suite) { delete suite; }

  //----------------------------------------------------------------------------
  // Success tests
  //----------------------------------------------------------------------------
  void test_Build_From_Name_And_Chemical_Formula_Single_Atom() {
    MaterialBuilder builder;
    Material mat = builder.setName("Nickel").setFormula("Ni").build();

    TS_ASSERT_EQUALS(mat.name(), "Nickel");
    TS_ASSERT_DELTA(mat.numberDensity(), 0.0913375, 0.0001);
    TS_ASSERT_DELTA(mat.totalScatterXSection(), 18.5, 0.0001);
    TS_ASSERT_DELTA(mat.absorbXSection(), 4.49, 0.0001);

    // Overrides by provided values
    const double rho(0.12);
    mat = builder.setNumberDensity(rho).build();
    TS_ASSERT_DELTA(mat.numberDensity(), rho, 0.0001);

    double totScatterXSec(18.1);
    mat = builder.setTotalScatterXSection(totScatterXSec).build();
    TS_ASSERT_DELTA(mat.totalScatterXSection(), totScatterXSec, 0.0001);
    double absXSec(4.6);
    mat = builder.setAbsorptionXSection(absXSec).build();
    TS_ASSERT_DELTA(mat.absorbXSection(), absXSec, 0.0001);
    double cohXSec(4.6);
    mat = builder.setCoherentXSection(cohXSec).build();
    TS_ASSERT_DELTA(mat.cohScatterXSection(), cohXSec, 0.0001);
    double incohXSec(4.6);
    mat = builder.setIncoherentXSection(incohXSec).build();
    TS_ASSERT_DELTA(mat.incohScatterXSection(), incohXSec, 0.0001);
  }

  void test_Build_From_Name_And_Chemical_Formula_MultiAtom() {
    MaterialBuilder builder;
    Material mat = builder.setName("Nickel")
                       .setFormula("Al2-O3")
                       .setNumberDensity(0.1)
                       .build();
    TS_ASSERT_DELTA(mat.numberDensity(), 0.1, 0.0001);
    TS_ASSERT_DELTA(mat.totalScatterXSection(), 3.1404, 0.0001);
    TS_ASSERT_DELTA(mat.absorbXSection(), 0.092514, 0.0001);
  }

  void test_Build_From_Atomic_Number() {
    MaterialBuilder builder;
    Material mat = builder.setName("Nickel")
                       .setAtomicNumber(28)
                       .setNumberDensity(0.1)
                       .build();
    // Default isotope
    TS_ASSERT_DELTA(mat.totalScatterXSection(), 18.5, 0.0001);
    TS_ASSERT_DELTA(mat.absorbXSection(), 4.49, 0.0001);

    mat = builder.setName("Ni").setAtomicNumber(28).setMassNumber(58).build();
    // Other isotop
    TS_ASSERT_DELTA(mat.totalScatterXSection(), 26.1, 0.0001);
    TS_ASSERT_DELTA(mat.absorbXSection(), 4.6, 0.0001);
  }

  void test_Number_Density_Set_By_Formula_ZParameter_And_Cell_Volume() {
    MaterialBuilder builder;
    auto mat = builder.setName("Nickel")
                   .setFormula("Al2-O3")
                   .setZParameter(6)
                   .setUnitCellVolume(253.54)
                   .build();

    TS_ASSERT_DELTA(mat.numberDensity(), 0.1183245, 0.001);
  }

  void test_Number_Density_Set_By_Formula_MassDensity() {
    MaterialBuilder builder;
    auto mat = builder.setName("Nickel")
                   .setFormula("Al2-O3")
                   .setMassDensity(4)
                   .build();

    TS_ASSERT_DELTA(mat.numberDensity(), 0.0236252 * 5, 0.001);
  }

  void test_Number_Density_Set_By_AtomicNumber_MassDensity() {
    MaterialBuilder builder;
    auto mat =
        builder.setName("Nickel").setAtomicNumber(28).setMassDensity(4).build();

    TS_ASSERT_DELTA(mat.numberDensity(), 0.0410414, 0.001);
  }

  void test_Number_Set_By_AtomicNumber_ZParameter_And_Cell_Volume() {
    MaterialBuilder builder;
    auto mat = builder.setName("Nickel")
                   .setAtomicNumber(28)
                   .setZParameter(6)
                   .setUnitCellVolume(253)
                   .build();

    TS_ASSERT_DELTA(mat.numberDensity(), 0.0237154, 0.001);
  }

  //----------------------------------------------------------------------------
  // Failure tests
  //----------------------------------------------------------------------------
  void test_Empty_Name_Throws_Error_When_Set() {
    MaterialBuilder builder;
    TS_ASSERT_THROWS(builder.setName(""), std::invalid_argument);
  }

  void test_Invalid_Formula_Throws_Error_When_Set() {
    MaterialBuilder builder;
    TS_ASSERT_THROWS(builder.setFormula(""), std::invalid_argument);
    TS_ASSERT_THROWS(builder.setFormula("Al-2"), std::invalid_argument);
  }

  void test_Setting_Neither_ChemicalFormula_And_AtomicNumber_Throws_Error() {
    MaterialBuilder builder;
    TS_ASSERT_THROWS(builder.setName("Nickel").build(), std::runtime_error);
  }

  void test_Setting_Both_ChemicalFormula_And_AtomicNumber_Throws_Error() {
    MaterialBuilder builder;
    TS_ASSERT_THROWS(builder.setFormula("Al2-O3").setAtomicNumber(28),
                     std::runtime_error);
    TS_ASSERT_THROWS(builder.setAtomicNumber(28).setFormula("Al2-O3"),
                     std::runtime_error);
  }

  void test_Setting_ZParameter_UnitCell_And_MassDensity_Throws_Error() {
    MaterialBuilder builder;
    TS_ASSERT_THROWS(builder.setMassDensity(4).setZParameter(6),
                     std::runtime_error);
    TS_ASSERT_THROWS(builder.setMassDensity(4).setUnitCellVolume(250.),
                     std::runtime_error);

    TS_ASSERT_THROWS(builder.setZParameter(6).setMassDensity(4),
                     std::runtime_error);
    TS_ASSERT_THROWS(builder.setUnitCellVolume(6).setMassDensity(4),
                     std::runtime_error);
  }

  void test_MultiAtom_with_no_number_density_throws() {
    MaterialBuilder builder;
    builder.setName("Nickel").setFormula("Al2-O3");
    TS_ASSERT_THROWS_EQUALS(
        builder.build(), const std::runtime_error &e, std::string(e.what()),
        "The number density could not be determined. Please "
        "provide the number density, ZParameter and unit "
        "cell volume or mass density.")
  }
};

#endif /* MANTID_KERNEL_MATERIALBUILDERTEST_H_ */
