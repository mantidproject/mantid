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
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/StringContainsValidator.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;

namespace detail {
class FAVT_Funct : public ParamFunction, public IFunction1D {
public:
  FAVT_Funct() {
    declareAttribute("DAttr", Attribute(0.0), Mantid::Kernel::BoundedValidator<double>(0.0, 100.0));

    declareAttribute("IAttr", Attribute(5), Mantid::Kernel::BoundedValidator<int>(0, 10));

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
        const IFunction::validationException &);

    TS_ASSERT_THROWS(declareAttribute("IAttr_invalid", Attribute(11), Mantid::Kernel::BoundedValidator<int>(0, 10)),
                     const IFunction::validationException &);

    TS_ASSERT_THROWS(declareAttribute("SAttr_invalid", Attribute("Invalid"),
                                      Mantid::Kernel::StringListValidator(std::vector<std::string>{"K", "meV"})),
                     const IFunction::validationException &);

    std::vector<std::string> sV(3);
    sV[0] = "a";
    sV[1] = "b";
    sV[2] = "c";
    TS_ASSERT_THROWS(
        declareAttribute("SQAttr_invalid", Attribute("ab", true), Mantid::Kernel::StringContainsValidator(sV)),
        const IFunction::validationException &);

    std::vector<double> v(3);
    v[0] = 1.0;
    v[1] = 2.0;
    v[2] = 50.0;
    TS_ASSERT_THROWS(
        declareAttribute("VAttr_invalid", Attribute(v), Mantid::Kernel::ArrayBoundedValidator<double>(1, 5)),
        const IFunction::validationException &);
  }
};

// Simple Attribute Visitor for Test
class SetAttribute : public Mantid::API::IFunction::AttributeVisitor<> {
public:
  SetAttribute(Mantid::Kernel::IValidator_sptr validator) : m_dbl(0) { m_validator = validator; }
  double m_dbl;

protected:
  /// Create string property
  void apply(std::string &str) const override { return; }
  /// Create double property
  void apply(double &d) const override {
    evaluateValidator(m_dbl);
    d = m_dbl;
  }
  /// Create int property
  void apply(int &i) const override { return; }
  /// Create bool property
  void apply(bool &b) const override { return; }
  /// Create vector property
  void apply(std::vector<double> &v) const override { return; }
};

DECLARE_FUNCTION(FAVT_Funct)
} // namespace detail

class FunctionAttributeValidatorTest : public CxxTest::TestSuite {
public:
  void test_double_attribute_validator() {
    detail::FAVT_Funct f;
    IFunction::Attribute att = f.getAttribute("DAttr");

    TS_ASSERT_THROWS(att.setDouble(-1), const IFunction::validationException &);

    att.setDouble(50.0);
    TS_ASSERT(att.asDouble() == 50.0);
  }

  void test_int_attribute_validator() {
    detail::FAVT_Funct f;
    IFunction::Attribute att = f.getAttribute("IAttr");

    TS_ASSERT_THROWS(att.setInt(11), const IFunction::validationException &);

    att.setInt(3);
    TS_ASSERT(att.asInt() == 3);
  }

  void test_string_attribute_validator() {
    detail::FAVT_Funct f;
    IFunction::Attribute att = f.getAttribute("SAttr");

    TS_ASSERT_THROWS(att.setString("Invalid"), IFunction::validationException &);

    att.setString("meV");
    TS_ASSERT(att.asString() == "meV");
  }

  void test_quoted_string_attribute_validator() {
    detail::FAVT_Funct f;
    IFunction::Attribute att = f.getAttribute("SQAttr");

    TS_ASSERT_THROWS(att.setString("ab"), IFunction::validationException &);

    att.setString("abcd");
    TS_ASSERT(att.asString() == "\"abcd\"");
  }

  void test_vector_attribute_validator() {
    detail::FAVT_Funct f;
    IFunction::Attribute att = f.getAttribute("VAttr");

    std::vector<double> v(3);
    v[0] = 1.0;
    v[1] = 2.0;
    v[2] = 5.0;

    att.setVector(v);
    TS_ASSERT(att.asVector() == v);

    v[2] = 50;
    TS_ASSERT_THROWS(att.setVector(v), const IFunction::validationException &);
  }

  void test_double_attribute_visitor() {
    detail::FAVT_Funct f;
    IFunction::Attribute att = f.getAttribute("DAttr");
    detail::SetAttribute att_visitor(att.getValidator());

    // Test visitor change within validator restrictions
    double dbl = 75;

    att_visitor.m_dbl = dbl;

    att.apply(att_visitor);
    TS_ASSERT(att.asDouble() == dbl);

    // Test visitor change outside of validator restrictions
    dbl = 150;
    att_visitor.m_dbl = dbl;

    TS_ASSERT_THROWS(att.apply(att_visitor), IFunction::validationException &);
  }

  void test_double_attribute_from_string() {
    detail::FAVT_Funct f;
    IFunction::Attribute att = f.getAttribute("DAttr");

    // Test visitor change within validator restrictions
    att.fromString("65.0");
    TS_ASSERT(att.asDouble() == 65.0);

    // Test visitor change outside of validator restrictions
    TS_ASSERT_THROWS(att.fromString("150.0"), IFunction::validationException &);
  }

  // void test_factory_creation() {
  // Need to test factory creation?
  //}
};
