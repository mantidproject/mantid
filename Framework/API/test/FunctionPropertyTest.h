// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/ParamFunction.h"
#include <json/value.h>
#include <memory>

using namespace Mantid::Kernel;
using namespace Mantid::API;

class FunctionPropertyTest_Function : public virtual ParamFunction, public virtual IFunction {
public:
  FunctionPropertyTest_Function() {
    this->declareParameter("A", 1.0);
    this->declareParameter("B", 2.0);
  }
  std::string name() const override { return "FunctionPropertyTest_Function"; }
  void function(const FunctionDomain &, FunctionValues &) const override {}
};

DECLARE_FUNCTION(FunctionPropertyTest_Function)

class FunctionPropertyTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FunctionPropertyTest *createSuite() { return new FunctionPropertyTest(); }
  static void destroySuite(FunctionPropertyTest *suite) { delete suite; }

  void test_Constructor() { TS_ASSERT_THROWS_NOTHING(FunctionProperty prop("fun")); }

  void test_Value() {
    FunctionProperty prop("fun");
    TS_ASSERT_EQUALS("", prop.value());

    std::string error;
    TS_ASSERT_THROWS_NOTHING(error = prop.setValue(createTestFunctionString()));
    TS_ASSERT(error.empty());
    std::shared_ptr<IFunction> fun_p = prop;
    TS_ASSERT_EQUALS(fun_p->asString(), "name=FunctionPropertyTest_Function,A=3,B=2");
    TS_ASSERT_EQUALS(fun_p->getParameter("A"), 3.0);
    TS_ASSERT_EQUALS(fun_p->getParameter("B"), 2.0);
    TS_ASSERT_EQUALS(prop.value(), "name=FunctionPropertyTest_Function,A=3,B=2");
  }

  void test_ValueAsJson() {
    FunctionProperty prop("fun");
    const std::string funcString("name=FunctionPropertyTest_Function,A=3,B=2");
    prop.setValue(funcString);
    TS_ASSERT_EQUALS(funcString, prop.valueAsJson().asString());
  }

  void test_SetValueFromJson() {
    FunctionProperty prop("fun");
    const std::string helpMessage{prop.setValueFromJson(Json::Value(createTestFunctionString()))};
    TS_ASSERT(helpMessage.empty());
    TS_ASSERT_EQUALS("name=FunctionPropertyTest_Function,A=3,B=2", prop.value());
  }

  void test_Bad_Value() {
    FunctionProperty prop("fun");
    std::string error;
    TS_ASSERT_THROWS_NOTHING(error = prop.setValue("name=FunctionDoesnotExist,A=3"));
    TS_ASSERT(!error.empty());
    TS_ASSERT_THROWS_NOTHING(error = prop.setValue("ghvjhgvjhgcjh"));
    TS_ASSERT(!error.empty());
  }

  void test_Assignment_By_SharedPtr() {
    FunctionProperty prop("fun");
    auto fun_p = FunctionFactory::Instance().createInitialized(createTestFunctionString());
    TS_ASSERT(fun_p);
    prop = fun_p;
    auto fun1_p = prop();
    TS_ASSERT(fun1_p);
    TS_ASSERT_EQUALS(fun_p, fun1_p);
    TS_ASSERT_EQUALS(fun1_p->asString(), "name=FunctionPropertyTest_Function,A=3,B=2");
    TS_ASSERT_EQUALS(fun1_p->getParameter("A"), 3.0);
    TS_ASSERT_EQUALS(fun1_p->getParameter("B"), 2.0);
  }

  void test_SetValue_From_Json() {}

  void test_Shared_Pointer() {
    FunctionProperty prop("fun");
    std::shared_ptr<FunctionPropertyTest_Function> fun_p(new FunctionPropertyTest_Function);
    TS_ASSERT(fun_p);
    fun_p->setParameter("A", 3);
    prop = fun_p;
    std::shared_ptr<IFunction> fun1_p = prop;
    TS_ASSERT(fun1_p);
    TS_ASSERT_EQUALS(fun_p, fun1_p);
    TS_ASSERT_EQUALS(fun1_p->asString(), "name=FunctionPropertyTest_Function,A=3,B=2");
    TS_ASSERT_EQUALS(fun1_p->getParameter("A"), 3.0);
    TS_ASSERT_EQUALS(fun1_p->getParameter("B"), 2.0);
  }

private:
  std::string createTestFunctionString() { return "name=FunctionPropertyTest_Function,A=3"; }
};
