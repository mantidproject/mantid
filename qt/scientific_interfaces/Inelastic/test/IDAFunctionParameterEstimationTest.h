// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "Analysis/IDAFunctionParameterEstimation.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces::IDA;

namespace {
DataForParameterEstimation createEstimationData(int const size) {
  std::vector<double> x(size, 1);
  std::vector<double> y(size, 0.981);

  return DataForParameterEstimation{x, y};
}

} // namespace

class IDAFunctionParameterEstimationTest : public CxxTest::TestSuite {
public:
  IDAFunctionParameterEstimationTest() {
    m_fitFunction = [](Mantid::MantidVec const &x, Mantid::MantidVec const &y) {
      (void)x;
      (void)y;
      return std::unordered_map<std::string, double>{{"A0", 2.0}, {"A1", 3.0}};
    };
  }

  void test_estimateFunctionParameters_does_nothing_if_estimate_data_is_too_small() {
    IDAFunctionParameterEstimation parameterEstimation({{"LinearBackground", m_fitFunction}});
    auto fun = FunctionFactory::Instance().createInitialized("name=LinearBackground,A0=0,A1=0");
    auto funCopy = fun->clone();

    parameterEstimation.estimateFunctionParameters(fun, createEstimationData(1));

    TS_ASSERT_EQUALS(fun->getParameter("A0"), funCopy->getParameter("A0"))
    TS_ASSERT_EQUALS(fun->getParameter("A1"), funCopy->getParameter("A1"))
  }

  void test_estimateFunctionParameters_correctly_updates_function() {
    IDAFunctionParameterEstimation parameterEstimation({{"LinearBackground", m_fitFunction}});
    auto fun = FunctionFactory::Instance().createInitialized("name=LinearBackground,A0=0,A1=0");

    parameterEstimation.estimateFunctionParameters(fun, createEstimationData(2));

    TS_ASSERT_EQUALS(fun->getParameter("A0"), 2.00)
    TS_ASSERT_EQUALS(fun->getParameter("A1"), 3.00)
  }

private:
  IDAFunctionParameterEstimation::ParameterEstimator m_fitFunction;
};
