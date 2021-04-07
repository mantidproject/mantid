// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidKernel/MDUnit.h"
#include "MantidKernel/UnitLabelTypes.h"

using namespace Mantid::Kernel;

class MDUnitTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MDUnitTest *createSuite() { return new MDUnitTest(); }
  static void destroySuite(MDUnitTest *suite) { delete suite; }

  void test_RLU_Constructor_with_valid_special_unit_label_accepts_the_label() {
    auto unitLabel = UnitLabel("in 1.992 A^-1");
    ReciprocalLatticeUnit unit(unitLabel);
    TS_ASSERT_EQUALS(unitLabel.ascii(), unit.getUnitLabel().ascii());
  }

  void test_RLU_Constructor_with_invalid_special_unit_label_does_not_accept_the_label() {
    auto unitLabel = UnitLabel("in invalidLabel A-1");
    ReciprocalLatticeUnit unit(unitLabel);
    TS_ASSERT_EQUALS(Units::Symbol::RLU, unit.getUnitLabel());
  }

  void test_RLU_getUnitLabel() {
    ReciprocalLatticeUnit unit;
    TS_ASSERT_EQUALS(Units::Symbol::RLU, unit.getUnitLabel());
  }

  void test_RLU_canConvertTo_does_not_convert_to_just_anything() {
    ReciprocalLatticeUnit unit;
    LabelUnit other("MeV");
    TSM_ASSERT("Conversion forbidden", !unit.canConvertTo(other));
    TSM_ASSERT_DIFFERS("Different types", unit, other);
  }

  void test_RLU_canConvertTo_InverseAngstroms() {
    ReciprocalLatticeUnit unit;
    InverseAngstromsUnit other;
    TSM_ASSERT("Simple conversion possible", unit.canConvertTo(other));
    TSM_ASSERT_DIFFERS("Convertable, but not the same", unit, other);
  }

  void test_InverseAngstroms_getUnitLabel() {
    InverseAngstromsUnit unit;
    TS_ASSERT_EQUALS(Units::Symbol::InverseAngstrom, unit.getUnitLabel());
  }

  void test_InverseAngstroms_canConvertTo_does_not_convert_to_just_anything() {
    InverseAngstromsUnit unit;
    LabelUnit other("MeV");
    TSM_ASSERT("Conversion forbidden", !unit.canConvertTo(other));
    TSM_ASSERT_DIFFERS("Different types", unit, other);
  }

  void test_InverseAnstroms_canConvertTo_RLU() {
    ReciprocalLatticeUnit unit;
    InverseAngstromsUnit other;
    TSM_ASSERT("Simple conversion possible", unit.canConvertTo(other));
    TSM_ASSERT_DIFFERS("Convertable, but not the same", unit, other);
  }

  void test_labelUnit_getUnitLabel() {
    // Negative test
    LabelUnit tUnit("DegC");
    TSM_ASSERT_DIFFERS("Not same unit label", UnitLabel("SomethingElse"), tUnit.getUnitLabel());

    // Positive test
    TSM_ASSERT_EQUALS("Same unit label", UnitLabel("DegC"), tUnit.getUnitLabel());
  }

  void test_LabelUnit_canConvert_to_same() {
    LabelUnit a("Bar");
    LabelUnit b("Bar");
    TS_ASSERT(a.canConvertTo(b));
    TSM_ASSERT_EQUALS("Convertable, and same type", a, b);
  }

  void test_LabelUnit_canConvert_to_other() {
    LabelUnit a("DegC");
    LabelUnit b("Bar");
    TS_ASSERT(!a.canConvertTo(b));
    TS_ASSERT_DIFFERS(a, b);
  }

  void test_cloneLabelUnit() {
    LabelUnit a("CustomUnit");
    LabelUnit *b = a.clone();
    TS_ASSERT_EQUALS(a, *b);
    delete b;
  }
};
