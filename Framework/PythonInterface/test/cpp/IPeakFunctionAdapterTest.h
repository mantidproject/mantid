// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef IPEAKFUNCTIONADAPTERTEST_H
#define IPEAKFUNCTIONADAPTERTEST_H

#include "FunctionAdapterTestCommon.h"
#include "MantidPythonInterface/api/FitFunctions/IPeakFunctionAdapter.h"
#include <algorithm>
#include <array>
#include <cxxtest/TestSuite.h>
#include <numeric>

class IPeakFunctionAdapterTest : public CxxTest::TestSuite {
public:
  static IPeakFunctionAdapterTest *createSuite() {
    return new IPeakFunctionAdapterTest();
  }
  static void destroySuite(IPeakFunctionAdapterTest *suite) { delete suite; }

  // -------------- Failure tests -------------------------

  void testfunctionLocal_Returning_Non_Numpy_Array_Throws() {
    using Mantid::API::IPeakFunction_sptr;
    using Mantid::PythonInterface::createTestFunction;
    const auto funcImpl = "        return 1";
    IPeakFunction_sptr badReturnFunc;
    TS_ASSERT_THROWS_NOTHING(
        badReturnFunc = createTestFunction<IPeakFunction_sptr::element_type>(
            "BadReturnFunction", funcImpl));
    TS_ASSERT(badReturnFunc);

    std::array<double, 1> xvalues{{10}}, retvalue{{0}};
    TS_ASSERT_THROWS(
        badReturnFunc->function1D(retvalue.data(), xvalues.data(), 1),
        const std::runtime_error &);
  }

  void testfunctionLocal_Returning_Numpy_Array_With_NonFloat_Type_Throws() {
    using Mantid::API::IPeakFunction_sptr;
    using Mantid::PythonInterface::createTestFunction;
    const auto funcImpl = "        import numpy as np\n"
                          "        return np.array([1])";
    IPeakFunction_sptr badNdArrayFunc;
    TS_ASSERT_THROWS_NOTHING(
        badNdArrayFunc = createTestFunction<IPeakFunction_sptr::element_type>(
            "BadNDArrayFunction", funcImpl));
    TS_ASSERT(badNdArrayFunc);

    std::array<double, 1> xvalues{{10}}, retvalue{{0}};
    TS_ASSERT_THROWS(
        badNdArrayFunc->function1D(retvalue.data(), xvalues.data(), 1),
        const std::runtime_error &);
  }

  // -------------- Success tests -------------------------

  void testfunction_Uses_Numerical_Deriv_When_Deriv_NotSupplied() {
    using Mantid::API::IPeakFunction_sptr;
    using Mantid::PythonInterface::FunctionAdapterTestJacobian;
    using Mantid::PythonInterface::createTestFunction;
    IPeakFunction_sptr noDerivPeakFunction;
    TS_ASSERT_THROWS_NOTHING(
        noDerivPeakFunction =
            createTestFunction<IPeakFunction_sptr::element_type>(
                "IFunction1DAdapterWithDeriv",
                "        return self.getParameterValue(0)*x"));
    TS_ASSERT(noDerivPeakFunction);

    std::array<double, 10> xvalues;
    std::iota(std::begin(xvalues), std::end(xvalues), 10.0);
    FunctionAdapterTestJacobian jacobian(xvalues.size(), 1);
    noDerivPeakFunction->functionDeriv1D(&jacobian, xvalues.data(),
                                         xvalues.size());

    TS_ASSERT_DELTA(9.99999, jacobian.get(0, 0), 1e-05);
  }

  void testfunction_Uses_Supplied_Deriv() {
    using Mantid::API::IPeakFunction_sptr;
    using Mantid::PythonInterface::FunctionAdapterTestJacobian;
    using Mantid::PythonInterface::createTestFunction;
    IPeakFunction_sptr peakFuncWithDeriv;
    TS_ASSERT_THROWS_NOTHING(
        peakFuncWithDeriv =
            createTestFunction<IPeakFunction_sptr::element_type>(
                "IPeakFunctionAdapterWithDeriv",
                "        return self.getParameterValue(0)*x",
                "        jacobian.set(0, 0, 2000)"));
    TS_ASSERT(peakFuncWithDeriv);

    std::array<double, 10> xvalues;
    std::iota(std::begin(xvalues), std::end(xvalues), 10.0);
    FunctionAdapterTestJacobian jacobian(xvalues.size(), 1);
    peakFuncWithDeriv->functionDeriv1D(&jacobian, xvalues.data(),
                                       xvalues.size());

    TS_ASSERT_DELTA(2000, jacobian.get(0, 0), 1e-05);
  }
};

#endif // IPEAKFUNCTIONADAPTERTEST_H
