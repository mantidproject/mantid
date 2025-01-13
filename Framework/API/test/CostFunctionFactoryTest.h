// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/ICostFunction.h"

#include <sstream>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::API;

class CostFunctionFactoryTest_A : public ICostFunction {
public:
  CostFunctionFactoryTest_A() {}

  std::string name() const override { return "fido"; }
  double getParameter(size_t) const override { return 0; }
  void setParameter(size_t, const double &) override {}
  size_t nParams() const override { return 0; }

  double val() const override { return 0.0; }
  void deriv(std::vector<double> &) const override {}
  double valAndDeriv(std::vector<double> &) const override { return 0.0; }
};

DECLARE_COSTFUNCTION(CostFunctionFactoryTest_A, nedtur)

class CostFunctionFactoryTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CostFunctionFactoryTest *createSuite() { return new CostFunctionFactoryTest(); }
  static void destroySuite(CostFunctionFactoryTest *suite) { delete suite; }

  CostFunctionFactoryTest() { Mantid::API::FrameworkManager::Instance(); }

  void testCreateFunction() {
    ICostFunction *cfA = CostFunctionFactory::Instance().createUnwrapped("nedtur");
    TS_ASSERT(cfA);
    TS_ASSERT(cfA->name().compare("fido") == 0);
    TS_ASSERT(cfA->shortName().compare("Quality") == 0);

    delete cfA;
  }
};
