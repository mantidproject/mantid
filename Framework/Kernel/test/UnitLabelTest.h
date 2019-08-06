// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_UNITLABELTEST_H_
#define MANTID_KERNEL_UNITLABELTEST_H_

#include "MantidKernel/UnitLabel.h"

#include <cxxtest/TestSuite.h>

using Mantid::Kernel::UnitLabel;

class UnitLabelTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static UnitLabelTest *createSuite() { return new UnitLabelTest(); }
  static void destroySuite(UnitLabelTest *suite) { delete suite; }

  void test_simple_string() {
    UnitLabel label("TextLabel", L"TextLabel", "TextLabelLatex");

    TS_ASSERT_EQUALS("TextLabel", label.ascii());
    TS_ASSERT_EQUALS("TextLabelLatex", label.latex());
  }

  void test_utf8_string_can_hold_unicode_data() {
    UnitLabel label("TextLabel", L"\u212b", "\\AA");

    TS_ASSERT_EQUALS(L"\u212b", label.utf8());
    TS_ASSERT_EQUALS("\\AA", label.latex());
  }

  void test_construction_from_single_string_sets_all_label_types_to_equal() {
    UnitLabel label("LabelText");

    TS_ASSERT_EQUALS("LabelText", label.ascii());
    TS_ASSERT_EQUALS(L"LabelText", label.utf8());
    TS_ASSERT_EQUALS("LabelText", label.latex());
  }

  void
  test_implicit_construction_from_std_string_sets_both_label_types_to_equal() {
    doImplicitConversionTest("LabelText", "LabelText");
  }

  void test_implicit_string_converter_returns_ascii_method() {
    UnitLabel label("TextLabel", L"\u212b", "\\AA");
    std::string asciiText = label;

    TS_ASSERT_EQUALS("TextLabel", asciiText);
  }

  void test_comparision_operators() {
    UnitLabel label("TextLabel", L"\u212b", "\\AA");
    UnitLabel labelDiffAscii("TextLabe", L"\u212b", "not used");
    UnitLabel labelDiffUtf8("TextLabel", L"\u207b", "not used");

    TS_ASSERT(label == label);
    TS_ASSERT(label == "TextLabel");
    TS_ASSERT(label == L"\u212b");

    TS_ASSERT(label != labelDiffAscii);
    TS_ASSERT(label != labelDiffUtf8);
    TS_ASSERT(label != "TextLabe");
    TS_ASSERT(label != L"\u207b");
  }

private:
  void doImplicitConversionTest(UnitLabel lbl, std::string expected) {
    TS_ASSERT_EQUALS(expected, lbl.ascii());
    const auto &utf8 = lbl.utf8();
    TS_ASSERT_EQUALS(std::wstring(expected.begin(), expected.end()), utf8);
  }
};

#endif /* MANTID_KERNEL_UNITLABELTEST_H_ */
