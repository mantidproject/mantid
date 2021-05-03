// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "IDAFunctionParameterEstimation.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces::IDA;

namespace {
DataForParameterEstimation createEstimationData() {
  std::vector<double> x(10, 1);
  std::vector<double> y(10, 0.981);

  return DataForParameterEstimation{x, y};
}

void estimationFunctionForLinearBackground(IFunction_sptr &function, const DataForParameterEstimation &) {

  function->setParameter("A0", 2.00);
  function->setParameter("A1", 3.00);
}
} // namespace

class IDAFunctionParameterEstimationTest : public CxxTest::TestSuite {
public:
  IDAFunctionParameterEstimationTest() { m_estimationData = createEstimationData(); }

  void test_estimateFunctionParameters_does_nothing_if_fit_esimate_does_not_exist() {
    IDAFunctionParameterEstimation parameterEstimation;
    auto fun = FunctionFactory::Instance().createInitialized("name=LinearBackground,A0=0,A1=0");
    auto funCopy = fun->clone();

    parameterEstimation.estimateFunctionParameters(fun, m_estimationData);

    TS_ASSERT_EQUALS(fun->getParameter("A0"), funCopy->getParameter("A0"))
    TS_ASSERT_EQUALS(fun->getParameter("A1"), funCopy->getParameter("A1"))
  }

  void test_estimateFunctionParameters_correctly_updates_function() {
    IDAFunctionParameterEstimation parameterEstimation;
    auto fun = FunctionFactory::Instance().createInitialized("name=LinearBackground,A0=0,A1=0");
    parameterEstimation.addParameterEstimationFunction("LinearBackground", estimationFunctionForLinearBackground);

    parameterEstimation.estimateFunctionParameters(fun, m_estimationData);

    TS_ASSERT_EQUALS(fun->getParameter("A0"), 2.00)
    TS_ASSERT_EQUALS(fun->getParameter("A1"), 3.00)
  }

private:
  DataForParameterEstimation m_estimationData;
};
