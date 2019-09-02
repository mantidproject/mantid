// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTIDWIDGETS_FUNCTIONMODELTEST_H_
#define MANTIDWIDGETS_FUNCTIONMODELTEST_H_

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidQtWidgets/Common/FunctionModel.h"
#include <cxxtest/TestSuite.h>

using namespace MantidQt::MantidWidgets;
using namespace Mantid::API;

class FunctionModelTest : public CxxTest::TestSuite {

public:
  static FunctionModelTest *createSuite() { return new FunctionModelTest; }
  static void destroySuite(FunctionModelTest *suite) { delete suite; }

  FunctionModelTest() {
    // To make sure API is initialized properly
    FrameworkManager::Instance();
  }

  void test_empty() {
    MultiDomainFunctionModel model;
    TS_ASSERT(!model.getFitFunction());
  }

  void test_simple() {
    MultiDomainFunctionModel model;
    model.setFunctionString("name=LinearBackground,A0=1,A1=2");
    auto fun = model.getFitFunction();
    TS_ASSERT_EQUALS(fun->name(), "LinearBackground");
    TS_ASSERT_EQUALS(fun->getParameter("A0"), 1.0);
    TS_ASSERT_EQUALS(fun->getParameter("A1"), 2.0);
  }

  void test_simple_multidomain() {
    MultiDomainFunctionModel model;
    model.setFunctionString("name=LinearBackground,A0=1,A1=2");
    model.setNumberDomains(2);
    TS_ASSERT_EQUALS(model.getNumberDomains(), 2);
    TS_ASSERT_EQUALS(model.currentDomainIndex(), 0);
    model.setCurrentDomainIndex(1);
    TS_ASSERT_EQUALS(model.currentDomainIndex(), 1);
    TS_ASSERT_THROWS_EQUALS(model.setCurrentDomainIndex(2),
                            std::runtime_error & e, std::string(e.what()),
                            "Domain index is out of range: 2 out of 2");
    {
      auto fun = model.getCurrentFunction();
      TS_ASSERT_EQUALS(fun->name(), "LinearBackground");
      TS_ASSERT_EQUALS(fun->getParameter("A0"), 1.0);
      TS_ASSERT_EQUALS(fun->getParameter("A1"), 2.0);
    }
    {
      auto fun = model.getSingleFunction(0);
      TS_ASSERT_EQUALS(fun->name(), "LinearBackground");
      TS_ASSERT_EQUALS(fun->getParameter("A0"), 1.0);
      TS_ASSERT_EQUALS(fun->getParameter("A1"), 2.0);
    }
    {
      auto fun = model.getSingleFunction(1);
      TS_ASSERT_EQUALS(fun->name(), "LinearBackground");
      TS_ASSERT_EQUALS(fun->getParameter("A0"), 1.0);
      TS_ASSERT_EQUALS(fun->getParameter("A1"), 2.0);
    }
    TS_ASSERT_THROWS_EQUALS(model.getSingleFunction(2), std::runtime_error & e,
                            std::string(e.what()),
                            "Domain index is out of range: 2 out of 2");
    {
      auto fun = model.getFitFunction();
      TS_ASSERT_EQUALS(fun->name(), "MultiDomainFunction");
      TS_ASSERT_EQUALS(fun->getParameter("f0.A0"), 1.0);
      TS_ASSERT_EQUALS(fun->getParameter("f0.A1"), 2.0);
      TS_ASSERT_EQUALS(fun->getParameter("f1.A0"), 1.0);
      TS_ASSERT_EQUALS(fun->getParameter("f1.A1"), 2.0);
    }
  }

  void test_globals() {
    MultiDomainFunctionModel model;
    model.setFunctionString("name=LinearBackground,A0=1,A1=2");
    model.setNumberDomains(3);
    QStringList globals("A1");
    model.setGlobalParameters(globals);
    auto fun = model.getFitFunction();
    TS_ASSERT(!fun->getTie(1));
    TS_ASSERT_EQUALS(fun->getTie(3)->asString(), "f1.A1=f0.A1");
    TS_ASSERT_EQUALS(fun->getTie(5)->asString(), "f2.A1=f0.A1");
    auto locals = model.getLocalParameters();
    TS_ASSERT_EQUALS(locals[0], "A0");
    globals.clear();
    globals << "A0";
    model.setGlobalParameters(globals);
    fun = model.getFitFunction();
    TS_ASSERT(!fun->getTie(0));
    TS_ASSERT(!fun->getTie(1));
    TS_ASSERT(!fun->getTie(3));
    TS_ASSERT(!fun->getTie(5));
    TS_ASSERT_EQUALS(fun->getTie(2)->asString(), "f1.A0=f0.A0");
    TS_ASSERT_EQUALS(fun->getTie(4)->asString(), "f2.A0=f0.A0");
    locals = model.getLocalParameters();
    TS_ASSERT_EQUALS(locals[0], "A1");
  }

  void test_set_number_domains_after_clear() {
    MultiDomainFunctionModel model;
    model.clear();
    model.setNumberDomains(1);
    TS_ASSERT_EQUALS(model.getNumberDomains(), 1);
  }

  void test_add_function_top_level() {
    MultiDomainFunctionModel model;
    {
      model.addFunction("", "name=LinearBackground,A0=1,A1=2");
      auto testFun = FunctionFactory::Instance().createInitialized(
          "name=LinearBackground,A0=3,A1=4");
      model.updateMultiDatasetParameters(*testFun);
      auto fun = model.getFitFunction();
      TS_ASSERT_EQUALS(fun->nParams(), 2);
      TS_ASSERT_EQUALS(fun->getParameter(0), 3.0);
      TS_ASSERT_EQUALS(fun->getParameter(1), 4.0);
    }
    {
      model.addFunction("", "name=LinearBackground,A0=1,A1=2");
      auto testFun = FunctionFactory::Instance().createInitialized(
          "name=LinearBackground,A0=3,A1=4;name=LinearBackground,A0=5,A1=6");
      model.updateMultiDatasetParameters(*testFun);
      auto fun = model.getFitFunction();
      TS_ASSERT_EQUALS(fun->nParams(), 4);
      TS_ASSERT_EQUALS(fun->getParameter(0), 3.0);
      TS_ASSERT_EQUALS(fun->getParameter(1), 4.0);
      TS_ASSERT_EQUALS(fun->getParameter(2), 5.0);
      TS_ASSERT_EQUALS(fun->getParameter(3), 6.0);
    }
    {
      model.addFunction("", "name=LinearBackground,A0=1,A1=2");
      auto testFun = FunctionFactory::Instance().createInitialized(
          "name=LinearBackground,A0=3,A1=4;name=LinearBackground,A0=5,A1=6;"
          "name=LinearBackground,A0=7,A1=8");
      model.updateMultiDatasetParameters(*testFun);
      auto fun = model.getFitFunction();
      TS_ASSERT_EQUALS(fun->nParams(), 6);
      TS_ASSERT_EQUALS(fun->getParameter(0), 3.0);
      TS_ASSERT_EQUALS(fun->getParameter(1), 4.0);
      TS_ASSERT_EQUALS(fun->getParameter(2), 5.0);
      TS_ASSERT_EQUALS(fun->getParameter(3), 6.0);
      TS_ASSERT_EQUALS(fun->getParameter(4), 7.0);
      TS_ASSERT_EQUALS(fun->getParameter(5), 8.0);
    }
  }

  void test_add_function_nested() {
    MultiDomainFunctionModel model;
    model.addFunction(
        "", "name=LinearBackground,A0=1,A1=2;(composite=CompositeFunction)");
    {
      model.addFunction("f1.", "name=LinearBackground,A0=1,A1=2");
      auto testFun = FunctionFactory::Instance().createInitialized(
          "name=LinearBackground,A0=3,A1=4;name=LinearBackground,A0=5,A1=6");
      model.updateMultiDatasetParameters(*testFun);
      auto fun = model.getFitFunction();
      TS_ASSERT_EQUALS(fun->nParams(), 4);
      TS_ASSERT_EQUALS(fun->getParameter(0), 3.0);
      TS_ASSERT_EQUALS(fun->getParameter(1), 4.0);
      TS_ASSERT_EQUALS(fun->getParameter(2), 5.0);
      TS_ASSERT_EQUALS(fun->getParameter(3), 6.0);
    }
    {
      model.addFunction("f1.", "name=LinearBackground,A0=1,A1=2");
      auto testFun = FunctionFactory::Instance().createInitialized(
          "name=LinearBackground,A0=3,A1=4;"
          "(name=LinearBackground,A0=5,A1=6;name=LinearBackground,A0=7,A1=8)");
      model.updateMultiDatasetParameters(*testFun);
      auto fun = model.getFitFunction();
      TS_ASSERT_EQUALS(fun->nParams(), 6);
      TS_ASSERT_EQUALS(fun->getParameter(0), 3.0);
      TS_ASSERT_EQUALS(fun->getParameter(1), 4.0);
      TS_ASSERT_EQUALS(fun->getParameter(2), 5.0);
      TS_ASSERT_EQUALS(fun->getParameter(3), 6.0);
      TS_ASSERT_EQUALS(fun->getParameter(4), 7.0);
      TS_ASSERT_EQUALS(fun->getParameter(5), 8.0);
    }
    {
      model.addFunction("f1.", "name=LinearBackground,A0=1,A1=2");
      auto testFun = FunctionFactory::Instance().createInitialized(
          "name=LinearBackground,A0=3,A1=4;"
          "(name=LinearBackground,A0=5,A1=6;name=LinearBackground,A0=7,A1=8;"
          "name=LinearBackground,A0=9,A1=10)");
      model.updateMultiDatasetParameters(*testFun);
      auto fun = model.getFitFunction();
      TS_ASSERT_EQUALS(fun->nParams(), 8);
      TS_ASSERT_EQUALS(fun->getParameter(0), 3.0);
      TS_ASSERT_EQUALS(fun->getParameter(1), 4.0);
      TS_ASSERT_EQUALS(fun->getParameter(2), 5.0);
      TS_ASSERT_EQUALS(fun->getParameter(3), 6.0);
      TS_ASSERT_EQUALS(fun->getParameter(4), 7.0);
      TS_ASSERT_EQUALS(fun->getParameter(5), 8.0);
      TS_ASSERT_EQUALS(fun->getParameter(6), 9.0);
      TS_ASSERT_EQUALS(fun->getParameter(7), 10.0);
    }
  }

  void test_remove_function() {
    MultiDomainFunctionModel model;
    model.addFunction("", "name=LinearBackground,A0=1,A1=2;name="
                          "LinearBackground,A0=1,A1=2;name=LinearBackground,A0="
                          "1,A1=2");
    {
      auto testFun = FunctionFactory::Instance().createInitialized(
          "name=LinearBackground,A0=3,A1=4;name=LinearBackground,A0=5,A1=6;"
          "name=LinearBackground,A0=7,A1=8");
      model.updateMultiDatasetParameters(*testFun);
      auto fun = model.getFitFunction();
      TS_ASSERT_EQUALS(fun->nParams(), 6);
      TS_ASSERT_EQUALS(fun->getParameter(0), 3.0);
      TS_ASSERT_EQUALS(fun->getParameter(1), 4.0);
      TS_ASSERT_EQUALS(fun->getParameter(2), 5.0);
      TS_ASSERT_EQUALS(fun->getParameter(3), 6.0);
      TS_ASSERT_EQUALS(fun->getParameter(4), 7.0);
      TS_ASSERT_EQUALS(fun->getParameter(5), 8.0);
    }
    {
      model.removeFunction("f1.");
      auto testFun = FunctionFactory::Instance().createInitialized(
          "name=LinearBackground,A0=3,A1=4;name=LinearBackground,A0=5,A1=6");
      model.updateMultiDatasetParameters(*testFun);
      auto fun = model.getFitFunction();
      TS_ASSERT_EQUALS(fun->nParams(), 4);
      TS_ASSERT_EQUALS(fun->getParameter(0), 3.0);
      TS_ASSERT_EQUALS(fun->getParameter(1), 4.0);
      TS_ASSERT_EQUALS(fun->getParameter(2), 5.0);
      TS_ASSERT_EQUALS(fun->getParameter(3), 6.0);
    }
    {
      model.removeFunction("f1.");
      auto testFun = FunctionFactory::Instance().createInitialized(
          "name=LinearBackground,A0=3,A1=4");
      model.updateMultiDatasetParameters(*testFun);
      auto fun = model.getFitFunction();
      TS_ASSERT_EQUALS(fun->nParams(), 2);
      TS_ASSERT_EQUALS(fun->getParameter(0), 3.0);
      TS_ASSERT_EQUALS(fun->getParameter(1), 4.0);
    }
  }
};

#endif // MANTIDWIDGETS_FUNCTIONMODELTEST_H_
