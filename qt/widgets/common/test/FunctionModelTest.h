// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTIDWIDGETS_FUNCTIONMODELTEST_H_
#define MANTIDWIDGETS_FUNCTIONMODELTEST_H_

#include "MantidQtWidgets/Common/FunctionModel.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IFunction.h"
#include <cxxtest/TestSuite.h>

using namespace MantidQt::MantidWidgets;
using namespace Mantid::API;

class FunctionModelTest : public CxxTest::TestSuite {

public:
  static FunctionModelTest *createSuite() {
    return new FunctionModelTest;
  }
  static void destroySuite(FunctionModelTest *suite) { delete suite; }

  FunctionModelTest() {
    // To make sure API is initialized properly
    FrameworkManager::Instance();
  }

  void test_empty() {
    SingleDomainFunctionModel model;
    TS_ASSERT(!model.getFitFunction());
  }

  void test_simple() {
    SingleDomainFunctionModel model;
    model.setFunctionStr("name=LinearBackground,A0=1,A1=2");
    auto fun = model.getFitFunction();
    TS_ASSERT_EQUALS(fun->name(), "LinearBackground");
    TS_ASSERT_EQUALS(fun->getParameter("A0"), 1.0);
    TS_ASSERT_EQUALS(fun->getParameter("A1"), 2.0);
  }

  void test_simple_multidomain() {
    MultiDomainFunctionModel model;
    model.setFunctionStr("name=LinearBackground,A0=1,A1=2");
    model.setNumberDomains(2);
    TS_ASSERT_EQUALS(model.getNumberDomains(), 2);
    TS_ASSERT_EQUALS(model.currentDomainIndex(), 0);
    model.setCurrentDomainIndex(1);
    TS_ASSERT_EQUALS(model.currentDomainIndex(), 1);
    TS_ASSERT_THROWS_EQUALS(model.setCurrentDomainIndex(2), std::runtime_error &e, std::string(e.what()),
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
    TS_ASSERT_THROWS_EQUALS(model.getSingleFunction(2), std::runtime_error &e, std::string(e.what()),
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
};

#endif // MANTIDWIDGETS_FUNCTIONMODELTEST_H_
