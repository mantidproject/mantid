// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_OPTIONALBOOLTEST_H_
#define MANTID_KERNEL_OPTIONALBOOLTEST_H_

#include "MantidKernel/Exception.h"
#include "MantidKernel/OptionalBool.h"

#include <json/value.h>

#include <cxxtest/TestSuite.h>
#include <sstream>

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

  void test_ostream_false() {
    std::stringstream buffer;

    OptionalBool notTrue(OptionalBool::False);
    buffer << notTrue;
    TS_ASSERT_EQUALS("False", buffer.str());
  }

  void test_ostream_true() {
    std::stringstream buffer;

    OptionalBool isTrue(OptionalBool::True);
    buffer << isTrue;
    TS_ASSERT_EQUALS("True", buffer.str());
  }

  void test_ostream_unset() {
    std::stringstream buffer;

    OptionalBool notTrue(OptionalBool::Unset);
    buffer << notTrue;
    TS_ASSERT_EQUALS("Unset", buffer.str());
  }

  void test_istream_to_false() {
    OptionalBool target;
    std::stringstream buffer;
    buffer << "False";
    buffer >> target;
    TS_ASSERT_EQUALS(target, OptionalBool(OptionalBool::False));
  }

  void test_istream_to_true() {
    OptionalBool target;
    std::stringstream buffer;
    buffer << "True";
    buffer >> target;
    TS_ASSERT_EQUALS(target, OptionalBool(OptionalBool::True));
  }

  void test_istream_to_unset() {
    OptionalBool target;
    std::stringstream buffer;
    buffer << "Unset";
    buffer >> target;
    TS_ASSERT_EQUALS(target, OptionalBool(OptionalBool::Unset));
  }

  void test_str_map() {
    auto map = OptionalBool::strToEmumMap();
    TS_ASSERT_EQUALS(3, map.size());
    TS_ASSERT_EQUALS(map[OptionalBool::StrUnset], OptionalBool::Unset);
    TS_ASSERT_EQUALS(map[OptionalBool::StrFalse], OptionalBool::False);
    TS_ASSERT_EQUALS(map[OptionalBool::StrTrue], OptionalBool::True);
  }

  void test_value_map() {
    auto map = OptionalBool::enumToStrMap();
    TS_ASSERT_EQUALS(3, map.size());
    TS_ASSERT_EQUALS(OptionalBool::StrUnset, map[OptionalBool::Unset]);
    TS_ASSERT_EQUALS(OptionalBool::StrFalse, map[OptionalBool::False]);
    TS_ASSERT_EQUALS(OptionalBool::StrTrue, map[OptionalBool::True]);
  }

  void testEncodeOptionalBoolPropertyThrows() {
    using Mantid::Kernel::OptionalBool;
    using Mantid::Kernel::Exception::NotImplementedError;
    OptionalBool truth{OptionalBool::True};
    TS_ASSERT_THROWS(encodeAsJson(truth), const NotImplementedError &);
  }
};

#endif /* MANTID_KERNEL_OPTIONALBOOLTEST_H_ */
