// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/ThermalNeutronBk2BkExpAlpha.h"

using namespace Mantid;
using Mantid::CurveFitting::Functions::ThermalNeutronBk2BkExpAlpha;

class ThermalNeutronBk2BkExpAlphaTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ThermalNeutronBk2BkExpAlphaTest *createSuite() { return new ThermalNeutronBk2BkExpAlphaTest(); }
  static void destroySuite(ThermalNeutronBk2BkExpAlphaTest *suite) { delete suite; }

  void test_Calculation() {
    // 1. Input data for test
    std::vector<double> vec_d = {2.72452, 2.84566, 3.33684, 4.719, 5.44903};

    // 2. Initialize the method
    ThermalNeutronBk2BkExpAlpha function;
    function.initialize();

    function.setParameter("Alph0", 4.026);
    function.setParameter("Alph1", 7.362);
    function.setParameter("Alph0t", 60.683);
    function.setParameter("Alph1t", 39.730);

    function.setParameter("Width", 1.0055);
    function.setParameter("Tcross", 0.4700);

    // 3. Set up domain
    API::FunctionDomain1DVector domain(vec_d);
    API::FunctionValues values(domain);

    function.function(domain, values);

    // 4. Check result
    for (size_t i = 0; i < domain.size(); ++i) {
      TS_ASSERT(values[i] > 0.0);
    }

    return;
  }
};
