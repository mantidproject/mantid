#ifndef MANTID_KERNEL_OPTIONALBOOLTEST_H_
#define MANTID_KERNEL_OPTIONALBOOLTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/OptionalBool.h"

using namespace Mantid::Kernel;

class OptionalBoolTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static OptionalBoolTest *createSuite() { return new OptionalBoolTest(); }
  static void destroySuite(OptionalBoolTest *suite) { delete suite; }

  void test_construction_by_bool() {

    OptionalBool arg1(true);
    TS_ASSERT_EQUALS(OptionalBool::True, arg1.getValue());

    OptionalBool arg2(false);
    TS_ASSERT_EQUALS(OptionalBool::False, arg2.getValue());
  }

  void test_defaults_to_unset() {
    OptionalBool arg;
    TS_ASSERT_EQUALS(OptionalBool::Unset, arg.getValue());
  }

  void test_construction_by_value() {
    auto value = OptionalBool::True;
    OptionalBool arg(value);
    TS_ASSERT_EQUALS(value, arg.getValue());
  }

  void test_comparison_overload() {
    OptionalBool a(OptionalBool::True);
    OptionalBool b(OptionalBool::True);
    TS_ASSERT_EQUALS(a, b);
    OptionalBool c(OptionalBool::False);
    TS_ASSERT_DIFFERS(a, c);
    OptionalBool d(OptionalBool::Unset);
    TS_ASSERT_DIFFERS(a, d);
  }

  void test_copy_constructor() {
    OptionalBool arg(OptionalBool::False);
    OptionalBool copy(arg);
    TS_ASSERT_EQUALS(OptionalBool::False, copy.getValue());
  }

  void test_assignment() {
    OptionalBool arg(OptionalBool::False);
    arg = OptionalBool(OptionalBool::True);
    TS_ASSERT_EQUALS(OptionalBool::True, arg.getValue());
  }
};

#endif /* MANTID_KERNEL_OPTIONALBOOLTEST_H_ */
