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
#include "MantidKernel/LambdaValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/StringContainsValidator.h"

#include <cxxtest/TestSuite.h>
#include <functional>

using namespace Mantid;
using namespace Mantid::API;

namespace detail {
class FAVT_Funct : public ParamFunction, public IFunction1D {
public:
  std::string name() const override { return "FAVT_Funct"; }
  void function1D(double *, const double *, const size_t) const override {}

  void declareDblBoundedAttr(std::string attrName, double inputVal, double minVal, double maxVal) {
    declareAttribute(attrName, Attribute(inputVal), Mantid::Kernel::BoundedValidator<double>(minVal, maxVal));
  }
  void declareIntBoundedAttr(std::string attrName, int inputVal, int minVal, int maxVal) {
    declareAttribute(attrName, Attribute(inputVal), Mantid::Kernel::BoundedValidator<int>(minVal, maxVal));
  }
  void declareStrListAttr(std::string attrName, std::string inputVal, std::vector<std::string> allowedVals) {
    declareAttribute(attrName, Attribute(inputVal), Mantid::Kernel::StringListValidator(allowedVals));
  }
  void declareStrContainsAttr(std::string attrName, std::string inputVal, std::vector<std::string> allowedVals) {
    declareAttribute(attrName, Attribute(inputVal, true), Mantid::Kernel::StringContainsValidator(allowedVals));
  }
  void declareVecArrayBoundedAttr(std::string attrName, std::vector<double> inputVec, double minVal, double maxVal) {
    declareAttribute(attrName, Attribute(inputVec), Mantid::Kernel::ArrayBoundedValidator<double>(minVal, maxVal));
  }
  template <typename T> void declareLambdaAttr(std::string attrName, T inputVal, std::function<std::string(T)> lambda) {
    declareAttribute(attrName, Attribute(inputVal), Mantid::Kernel::LambdaValidator<T>(lambda));
  }
};

// Simple Attribute Visitor for Test
class SetAttribute : public Mantid::API::IFunction::AttributeVisitor<> {
public:
  SetAttribute(Mantid::Kernel::IValidator_sptr validator) { m_validator = validator; }
  double m_dbl = 0;
  std::string m_str;
  int m_i = 0;
  bool m_b = false;
  std::vector<double> m_v;

protected:
  /// Create string property
  void apply(std::string &str) const override {
    evaluateValidator(m_str);
    str = m_str;
  }
  /// Create double property
  void apply(double &d) const override {
    evaluateValidator(m_dbl);
    d = m_dbl;
  }
  /// Create int property
  void apply(int &i) const override {
    evaluateValidator(m_i);
    i = m_i;
  }
  /// Create bool property
  void apply(bool &b) const override {
    evaluateValidator(m_b);
    b = m_b;
  }
  /// Create vector property
  void apply(std::vector<double> &v) const override {
    evaluateValidator(m_v);
    v = m_v;
  }
};

DECLARE_FUNCTION(FAVT_Funct)
} // namespace detail

class FunctionAttributeValidatorTest : public CxxTest::TestSuite {
public:
  void test_double_attribute_validator() {
    detail::FAVT_Funct f;
    f.declareDblBoundedAttr("DAttr", 0.0, 0.0, 100.0);
    IFunction::Attribute att = f.getAttribute("DAttr");

    TS_ASSERT_THROWS(att.setDouble(-1), const IFunction::ValidationException &);

    att.setDouble(50.0);
    TS_ASSERT(att.asDouble() == 50.0);
  }

  void test_int_attribute_validator() {
    detail::FAVT_Funct f;
    f.declareIntBoundedAttr("IAttr", 5, 0, 10);
    IFunction::Attribute att = f.getAttribute("IAttr");

    TS_ASSERT_THROWS(att.setInt(11), const IFunction::ValidationException &);

    att.setInt(3);
    TS_ASSERT(att.asInt() == 3);
  }

  void test_string_attribute_validator() {
    detail::FAVT_Funct f;
    f.declareStrListAttr("SAttr", "K", std::vector<std::string>{"K", "meV"});
    IFunction::Attribute att = f.getAttribute("SAttr");

    TS_ASSERT_THROWS(att.setString("Invalid"), IFunction::ValidationException &);

    att.setString("meV");
    TS_ASSERT(att.asString() == "meV");
  }

  void test_quoted_string_attribute_validator() {
    detail::FAVT_Funct f;

    std::vector<std::string> sV(3);
    sV[0] = "a";
    sV[1] = "b";
    sV[2] = "c";
    f.declareStrContainsAttr("SCAttr", "abc", sV);
    IFunction::Attribute att = f.getAttribute("SCAttr");

    TS_ASSERT_THROWS(att.setString("ab"), IFunction::ValidationException &);

    att.setString("abcd");
    TS_ASSERT(att.asString() == "\"abcd\"");
  }

  void test_vector_attribute_validator() {
    detail::FAVT_Funct f;
    std::vector<double> v(3);
    v[0] = 1;
    v[1] = 2;
    v[2] = 3;
    f.declareVecArrayBoundedAttr("VAttr", v, 1, 5);
    IFunction::Attribute att = f.getAttribute("VAttr");

    std::vector<double> v2(3);
    v2[0] = 1.0;
    v2[1] = 2.0;
    v2[2] = 5.0;

    att.setVector(v2);
    TS_ASSERT(att.asVector() == v2);

    v2[2] = 50;
    TS_ASSERT_THROWS(att.setVector(v2), const IFunction::ValidationException &);
  }

  void test_lambda_attribute_validator() {
    detail::FAVT_Funct f;
    int a = 4;
    std::function<std::string(int)> l = [](int a) { return a % 2 == 0 ? "" : "Value should be even"; };

    f.declareLambdaAttr("LAttr", a, l);
    IFunction::Attribute att = f.getAttribute("LAttr");

    a = 4;
    att.setInt(a);
    TS_ASSERT(att.asInt() == a);

    a = 5;
    TS_ASSERT_THROWS(att.setInt(a), const IFunction::ValidationException &);
  }

  void test_double_attribute_visitor() {
    detail::FAVT_Funct f;
    f.declareDblBoundedAttr("DAttr", 0.0, 0.0, 100.0);
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

    TS_ASSERT_THROWS(att.apply(att_visitor), IFunction::ValidationException &);
  }

  void test_double_attribute_from_string() {
    detail::FAVT_Funct f;
    f.declareDblBoundedAttr("DAttr", 0.0, 0.0, 100.0);
    IFunction::Attribute att = f.getAttribute("DAttr");

    // Test visitor change within validator restrictions
    att.fromString("65.0");
    TS_ASSERT(att.asDouble() == 65.0);

    // Test visitor change outside of validator restrictions
    TS_ASSERT_THROWS(att.fromString("150.0"), IFunction::ValidationException &);
  }

  void test_invalid_declarations() {
    // Test invalid declarations

    detail::FAVT_Funct f;
    TS_ASSERT_THROWS(f.declareDblBoundedAttr("DAttr_invalid", -1.0, 0.0, 100.0), IFunction::ValidationException &);
    TS_ASSERT_THROWS(f.declareDblBoundedAttr("IAttr_invalid", -1, 0, 100), IFunction::ValidationException &);
    TS_ASSERT_THROWS(f.declareStrListAttr("SAttr_invalid", "Invalid", std::vector<std::string>{"K", "meV"}),
                     IFunction::ValidationException &);

    std::vector<std::string> sV(3);
    sV[0] = "a";
    sV[1] = "b";
    sV[2] = "c";
    TS_ASSERT_THROWS(f.declareStrContainsAttr("SCAttr_invalid", "Invalid", sV), IFunction::ValidationException &);

    std::vector<double> v(3);
    v[0] = 1;
    v[1] = 2;
    v[2] = 10;
    TS_ASSERT_THROWS(f.declareVecArrayBoundedAttr("VAttr_invalid", v, 1.0, 5.0), IFunction::ValidationException &);
  }
};
