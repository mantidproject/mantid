// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidKernel/ArrayBoundedValidator.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/StringContainsValidator.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;

namespace detail {
class IFT_Funct : public ParamFunction, public IFunction1D {
public:
  IFT_Funct() {
    declareAttribute("DAttr", Attribute(0.0), Mantid::Kernel::BoundedValidator<double>(0.0, 100.0));

    int i[5] = {1, 2, 3, 4, 5};
    declareAttribute("IAttr", Attribute(i), Mantid::Kernel::ArrayLengthValidator<int>(3, 10));

    declareAttribute("SAttr", Attribute("K"),
                     Mantid::Kernel::StringListValidator(std::vector<std::string>{"K", "meV"}));

    std::vector<std::string> sV(3);
    sV[0] = "a";
    sV[1] = "b";
    sV[2] = "c";
    declareAttribute("SQAttr", Attribute("abc", true), Mantid::Kernel::StringContainsValidator(sV));

    std::vector<double> v(3);
    v[0] = 1;
    v[1] = 2;
    v[2] = 3;
    declareAttribute("VAttr", Attribute(v), Mantid::Kernel::ArrayBoundedValidator<double>(1, 5));

    testInvalidDeclaration();
  }

  std::string name() const override { return "IFT_Funct"; }
  void function1D(double *, const double *, const size_t) const override {}
  void functionDeriv1D(Jacobian *, const double *, const size_t) override {}

private:
  void testInvalidDeclaration() {
    TS_ASSERT_THROWS(
        declareAttribute("DAttr_invalid", Attribute(-1), Mantid::Kernel::BoundedValidator<double>(0.0, 100.0)),
        const std::runtime_error &);

    int i[2] = {1, 2};
    TS_ASSERT_THROWS(declareAttribute("IAttr_invalid", Attribute(i), Mantid::Kernel::ArrayLengthValidator<int>(3, 10)),
                     const std::runtime_error &);

    TS_ASSERT_THROWS(declareAttribute("SAttr_invalid", Attribute("Invalid"),
                                      Mantid::Kernel::StringListValidator(std::vector<std::string>{"K", "meV"})),
                     const std::runtime_error &);

    std::vector<std::string> sV;
    sV[0] = "a";
    sV[1] = "b";
    sV[2] = "c";
    TS_ASSERT_THROWS(
        declareAttribute("SQAttr_invalid", Attribute("ab", true), Mantid::Kernel::StringContainsValidator(sV)),
        const std::runtime_error &);

    std::vector<double> v(3);
    v[0] = 1.0;
    v[1] = 2.0;
    v[2] = 50.0;
    TS_ASSERT_THROWS(
        declareAttribute("VAttr_invalid", Attribute(v), Mantid::Kernel::ArrayBoundedValidator<double>(1, 5)),
        const std::runtime_error &);
  }
};

DECLARE_FUNCTION(IFT_Funct)
} // namespace detail

class FunctionAttributeTest : public CxxTest::TestSuite {
public:
  void test_double_attribute_validator() {
    detail::IFT_Funct f;
    IFunction::Attribute att = f.getAttribute("DAttr");

    TS_ASSERT_THROWS(att.setDouble(-1), const std::runtime_error &);

    att.setDouble(50.0);
    TS_ASSERT(f.getAttribute("DAttr").asDouble() == 50.0);
  }

  void test_int_attribute_validator() {
    detail::IFT_Funct f;
    IFunction::Attribute att = f.getAttribute("IAttr");

    int i[2] = {1, 2};
    TS_ASSERT_THROWS(att.setInt(i), const std::runtime_error &);

    int i[5] = {1, 2, 3, 4, 5};
    att.setInt(i);
    TS_ASSERT(f.getAttribute("IAttr").asInt() == i);
  }

  void test_string_attribute_validator() {
    detail::IFT_Funct f;
    IFunction::Attribute att = f.getAttribute("SAttr");

    TS_ASSERT_THROWS(att.setString("Invalid"), const std::runtime_error &);

    att.setString("meV");
    TS_ASSERT(f.getAttribute("SAttr").asString() == "meV");
  }

  void test_quoted_string_attribute_validator() {
    detail::IFT_Funct f;
    IFunction::Attribute att = f.getAttribute("SQAttr");

    TS_ASSERT_THROWS(att.setString("ab"), const std::runtime_error &);

    att.setString("abcd");
    TS_ASSERT(f.getAttribute("SQAttr").asString() == "abcd");
  }

  void test_vector_attribute_validator() {
    detail::IFT_Funct f;
    IFunction::Attribute att = f.getAttribute("VAttr");

    std::vector<double> v(3);
    v[0] = 1.0;
    v[1] = 2.0;
    v[2] = 5.0;

    att.setVector(v);
    TS_ASSERT(f.getAttribute("VAttr").asVector() == v);

    v[2] = 50;
    TS_ASSERT_THROWS(att.setVector(v), const std::runtime_error &);
  }

  // void test_factory_creation() {
  // Will need to test factory creation.
  //}
};
