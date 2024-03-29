// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidSINQ/PoldiUtilities/PoldiSpectrumConstantBackground.h"

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::Poldi;

class PoldiSpectrumConstantBackgroundTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PoldiSpectrumConstantBackgroundTest *createSuite() { return new PoldiSpectrumConstantBackgroundTest(); }
  static void destroySuite(PoldiSpectrumConstantBackgroundTest *suite) { delete suite; }

  PoldiSpectrumConstantBackgroundTest() { FrameworkManager::Instance(); }

  void testParameterCount() {
    PoldiSpectrumConstantBackground function;
    function.initialize();

    TS_ASSERT_EQUALS(function.nParams(), 1);
  }

  void testFunction() {
    IFunction_sptr function = FunctionFactory::Instance().createFunction("PoldiSpectrumConstantBackground");
    TS_ASSERT(function);
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspaceWhereYIsWorkspaceIndex(20, 2);

    TS_ASSERT_THROWS_NOTHING(function->setWorkspace(ws));
    function->setParameter(0, 10.0);

    FunctionDomain1DVector domain(ws->x(0).rawData());
    FunctionValues values(domain);

    function->function(domain, values);

    TS_ASSERT_EQUALS(values[0], 10.0);
    TS_ASSERT_EQUALS(values[1], 10.0);
  }

  void testPoldiFunction1D() {
    IFunction_sptr function = FunctionFactory::Instance().createFunction("PoldiSpectrumConstantBackground");
    TS_ASSERT(function);
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspace123(20, 2);

    TS_ASSERT_THROWS_NOTHING(function->setWorkspace(ws));
    function->setParameter(0, 10.0);

    FunctionDomain1DVector domain(0.0, 10.0, 100);
    FunctionValues values(domain);

    // workspace has 20 spectra, value does not matter for function
    std::vector<int> indices(20, 1);

    std::shared_ptr<IPoldiFunction1D> poldiFunction = std::dynamic_pointer_cast<IPoldiFunction1D>(function);
    TS_ASSERT(poldiFunction);
    poldiFunction->poldiFunction1D(indices, domain, values);

    for (size_t i = 0; i < values.size(); ++i) {
      TS_ASSERT_EQUALS(values[i], 4.0)
    }
  }
};
