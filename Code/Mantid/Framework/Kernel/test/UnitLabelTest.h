#ifndef MANTID_KERNEL_UNITLABELTEST_H_
#define MANTID_KERNEL_UNITLABELTEST_H_

#include "MantidKernel/UnitLabel.h"

#include <cxxtest/TestSuite.h>

using Mantid::Kernel::UnitLabel;

class UnitLabelTest : public CxxTest::TestSuite
{

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static UnitLabelTest *createSuite() { return new UnitLabelTest(); }
  static void destroySuite( UnitLabelTest *suite ) { delete suite; }

  void test_simple_string()
  {
    UnitLabel label("TextLabel", L"TextLabel");

    TS_ASSERT_EQUALS("TextLabel", label.ascii());
  }

  void test_utf8_string_can_hold_unicode_data()
  {
    UnitLabel label("TextLabel", L"\u212b");

    TS_ASSERT_EQUALS(L"\u212b", label.utf8());
  }

  void test_implicit_string_converter_returns_ascii_method()
  {
    UnitLabel label("TextLabel", L"\u212b");
    std::string asciiText = label;

    TS_ASSERT_EQUALS("TextLabel", asciiText);
  }

};


#endif /* MANTID_KERNEL_UNITLABELTEST_H_ */
