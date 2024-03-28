// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "QENSFitting/ParameterEstimation.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces::Inelastic;

namespace {
DataForParameterEstimationCollection createEstimationData(int const nDomains, int const nDataPoints) {
  std::vector<double> x(nDataPoints, 2.2);
  std::vector<double> y(nDataPoints, 3.3);

  return DataForParameterEstimationCollection(nDomains, DataForParameterEstimation{x, y});
}

Mantid::API::IFunction_sptr createIFunction(std::string const &functionString) {
  return Mantid::API::FunctionFactory::Instance().createInitialized(functionString);
}

Mantid::API::CompositeFunction_sptr toComposite(Mantid::API::IFunction_sptr function) {
  return std::dynamic_pointer_cast<Mantid::API::CompositeFunction>(function);
}

Mantid::API::IFunction_sptr createComposite(std::string const &functionString1, std::string const &functionString2) {
  auto composite = toComposite(createIFunction("name=CompositeFunction"));
  composite->addFunction(createIFunction(functionString1));
  composite->addFunction(createIFunction(functionString2));
  return composite;
}

Mantid::API::IFunction_sptr createMultiDomainFunction(std::string const &functionString1,
                                                      std::string const &functionString2) {
  auto multiDomainFunc = toComposite(createIFunction("name=MultiDomainFunction"));
  multiDomainFunc->addFunction(createComposite(functionString1, functionString2));
  multiDomainFunc->addFunction(createComposite(functionString1, functionString2));
  return multiDomainFunc;
}

} // namespace

class ParameterEstimationTest : public CxxTest::TestSuite {
public:
  ParameterEstimationTest() {
    FunctionParameterEstimation::ParameterEstimator linearBackground = [](Mantid::MantidVec const &x,
                                                                          Mantid::MantidVec const &y) {
      return std::unordered_map<std::string, double>{{"A0", x[0]}, {"A1", y[0]}};
    };
    FunctionParameterEstimation::ParameterEstimator expDecay = [](Mantid::MantidVec const &x,
                                                                  Mantid::MantidVec const &y) {
      return std::unordered_map<std::string, double>{{"Height", 2.0 * x[0]}, {"Lifetime", 2.0 * y[0]}};
    };

    auto const estimators = std::unordered_map<std::string, FunctionParameterEstimation::ParameterEstimator>{
        {"LinearBackground", linearBackground}, {"ExpDecay", expDecay}};
    m_parameterEstimators = std::make_unique<FunctionParameterEstimation>(estimators);
  }

  static ParameterEstimationTest *createSuite() { return new ParameterEstimationTest(); }

  static void destroySuite(ParameterEstimationTest *suite) { delete suite; }

  void test_estimateFunctionParameters_does_nothing_if_nDataPoints_is_too_small() {
    auto multiDomainFunction = createMultiDomainFunction("name=LinearBackground", "name=ExpDecay");

    TS_ASSERT_THROWS_NOTHING(
        m_parameterEstimators->estimateFunctionParameters(multiDomainFunction, createEstimationData(2, 1)))

    TS_ASSERT_EQUALS(0.0, multiDomainFunction->getParameter("f0.f0.A0"))
    TS_ASSERT_EQUALS(0.0, multiDomainFunction->getParameter("f0.f0.A1"))
    TS_ASSERT_EQUALS(1.0, multiDomainFunction->getParameter("f0.f1.Height"))
    TS_ASSERT_EQUALS(1.0, multiDomainFunction->getParameter("f0.f1.Lifetime"))
  }

  void test_estimateFunctionParameters_correctly_updates_function() {
    auto multiDomainFunction = createMultiDomainFunction("name=LinearBackground", "name=ExpDecay");

    m_parameterEstimators->estimateFunctionParameters(multiDomainFunction, createEstimationData(2, 2));

    TS_ASSERT_EQUALS(2.2, multiDomainFunction->getParameter("f0.f0.A0"))
    TS_ASSERT_EQUALS(3.3, multiDomainFunction->getParameter("f0.f0.A1"))
    TS_ASSERT_EQUALS(4.4, multiDomainFunction->getParameter("f0.f1.Height"))
    TS_ASSERT_EQUALS(6.6, multiDomainFunction->getParameter("f0.f1.Lifetime"))
  }

  void test_estimateFunctionParameters_does_not_throw_if_function_is_null() {
    Mantid::API::IFunction_sptr multiDomainFunction = nullptr;
    TS_ASSERT_THROWS_NOTHING(
        m_parameterEstimators->estimateFunctionParameters(multiDomainFunction, createEstimationData(2, 2)))
  }

  void test_estimateFunctionParameters_does_not_throw_if_estimate_data_has_different_size() {
    auto multiDomainFunction = createMultiDomainFunction("name=LinearBackground", "name=ExpDecay");

    TS_ASSERT_THROWS_NOTHING(
        m_parameterEstimators->estimateFunctionParameters(multiDomainFunction, createEstimationData(1, 2)))

    TS_ASSERT_EQUALS(0.0, multiDomainFunction->getParameter("f0.f0.A0"))
    TS_ASSERT_EQUALS(0.0, multiDomainFunction->getParameter("f0.f0.A1"))
    TS_ASSERT_EQUALS(1.0, multiDomainFunction->getParameter("f0.f1.Height"))
    TS_ASSERT_EQUALS(1.0, multiDomainFunction->getParameter("f0.f1.Lifetime"))
  }

private:
  std::unique_ptr<FunctionParameterEstimation> m_parameterEstimators;
};
