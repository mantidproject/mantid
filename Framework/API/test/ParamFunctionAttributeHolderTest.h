// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef PARAMFUNCTIONATTRIBUTEHOLDERTEST_H_
#define PARAMFUNCTIONATTRIBUTEHOLDERTEST_H_

#include "MantidAPI/ParamFunction.h"
#include <cxxtest/TestSuite.h>

class FakeParamFunctionAttributeHolder : public Mantid::API::ParamFunction {
public:
  std::string name() const override {
    return "FakeParamFunctionAttributeHolder";
  }

  void declareAttributes() override {
    declareAttribute("Att1", Mantid::API::IFunction::Attribute(3));
    declareAttribute("Att2", Mantid::API::IFunction::Attribute(2));
    declareAttribute("Att3", Mantid::API::IFunction::Attribute(1));
  }
  void declareParameters() override {
    declareParameter("Par1", 1.0);
    declareParameter("Par2", 9.1);
    declareParameter("Par3", 10.0);
    declareParameter("Par4", 6.5);
  }
  void function(const Mantid::API::FunctionDomain &,
                Mantid::API::FunctionValues &) const override {}
};

class ParamFunctionAttributeHolderTest : public CxxTest::TestSuite {
public:
  void test_Object_Initialization_Declares_Expected_Attributes() {
    FakeParamFunctionAttributeHolder funct;
    TS_ASSERT_EQUALS(funct.nAttributes(), 0);
    funct.initialize();
    TS_ASSERT_EQUALS(funct.nAttributes(), 3);
  }

  void test_Object_Initialization_Declares_Expected_Parameters() {
    FakeParamFunctionAttributeHolder funct;
    TS_ASSERT_EQUALS(funct.nParams(), 0);
    funct.initialize();
    TS_ASSERT_EQUALS(funct.nParams(), 4);
  }

  void test_Unknown_Attribute_Throws_Invalid_Argument() {
    using namespace Mantid::API;
    FakeParamFunctionAttributeHolder funct;

    TS_ASSERT_THROWS(funct.getAttribute("NonExistent"), const std::invalid_argument &);
    TS_ASSERT_THROWS(funct.setAttribute("NonExistent", IFunction::Attribute(1)),
                     const std::invalid_argument &);
  }

  void test_hasAttribute_Returns_True_For_Existing_Attribute() {
    FakeParamFunctionAttributeHolder funct;
    funct.initialize();
    TS_ASSERT_EQUALS(funct.hasAttribute("Att1"), true);
  }

  void test_hasAttribute_Returns_False_For_NoneExisting_Attribute() {
    FakeParamFunctionAttributeHolder funct;
    TS_ASSERT_EQUALS(funct.hasAttribute("Att1"), false);
  }

  void test_Attribute_Defaults_Are_Respected() {
    FakeParamFunctionAttributeHolder funct;
    funct.initialize();
    int att1 = funct.getAttribute("Att1").asInt();
    TS_ASSERT_EQUALS(att1, 3);
    int att2 = funct.getAttribute("Att2").asInt();
    TS_ASSERT_EQUALS(att2, 2);
    int att3 = funct.getAttribute("Att3").asInt();
    TS_ASSERT_EQUALS(att3, 1);
  }

  void test_AttributeNames_Are_The_Declared_Ones() {
    FakeParamFunctionAttributeHolder funct;
    funct.initialize();
    std::vector<std::string> attrNames = funct.getAttributeNames();
    TS_ASSERT_EQUALS(attrNames.size(), 3);
    TS_ASSERT_EQUALS(attrNames.at(0), "Att1");
    TS_ASSERT_EQUALS(attrNames.at(1), "Att2");
    TS_ASSERT_EQUALS(attrNames.at(2), "Att3");
  }
};

#endif /* PARAMFUNCTIONATTRIBUTEHOLDERTEST_H_ */
