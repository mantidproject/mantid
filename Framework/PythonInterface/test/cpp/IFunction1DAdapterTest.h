// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINTERFACE_IFUNCTION1DADAPTERTEST_H
#define MANTID_PYTHONINTERFACE_IFUNCTION1DADAPTERTEST_H

#include "FunctionAdapterTestCommon.h"
#include "MantidPythonInterface/api/FitFunctions/IFunction1DAdapter.h"
#include <algorithm>
#include <array>
#include <cxxtest/TestSuite.h>
#include <numeric>

class IFunction1DAdapterTest : public CxxTest::TestSuite {
public:
  static IFunction1DAdapterTest *createSuite() {
    return new IFunction1DAdapterTest();
  }
  static void destroySuite(IFunction1DAdapterTest *suite) { delete suite; }

  // ---------------- Failure tests -------------------------
  void testfunction1D_Returning_Non_Numpy_Array_Throws() {
    using Mantid::API::IFunction1D_sptr;
    using Mantid::PythonInterface::createTestFunction;
    const auto func1DImpl = "        return 1";
    IFunction1D_sptr badReturnFunc1D;
    TS_ASSERT_THROWS_NOTHING(
        badReturnFunc1D = createTestFunction<IFunction1D_sptr::element_type>(
            "BadReturnFunction", func1DImpl));
    TS_ASSERT(badReturnFunc1D);

    std::array<double, 1> xvalues{{10}}, retvalue{{0}};
    TS_ASSERT_THROWS(
        badReturnFunc1D->function1D(retvalue.data(), xvalues.data(), 1),
        const std::runtime_error &);
  }

  void testfunction1D_Returning_Numpy_Array_With_NonFloat_Type_Throws() {
    using Mantid::API::IFunction1D_sptr;
    using Mantid::PythonInterface::createTestFunction;
    const auto funcImpl = "        import numpy as np\n"
                          "        return np.array([1])";
    IFunction1D_sptr badNdArrayFunc1D;
    TS_ASSERT_THROWS_NOTHING(
        badNdArrayFunc1D = createTestFunction<IFunction1D_sptr::element_type>(
            "BadReturnFunction", funcImpl));
    TS_ASSERT(badNdArrayFunc1D);

    std::array<double, 1> xvalues{{10}}, retvalue{{0}};
    TS_ASSERT_THROWS(
        badNdArrayFunc1D->function1D(retvalue.data(), xvalues.data(), 1),
        const std::runtime_error &);
  }

  // -------------- Success tests -------------------------
  void testfunction1D_Return_Numpy_Array_Copies_Values_To_Output_Array() {
    using Mantid::API::IFunction1D_sptr;
    using Mantid::PythonInterface::createTestFunction;
    IFunction1D_sptr timesTwo;
    TS_ASSERT_THROWS_NOTHING(
        timesTwo = createTestFunction<IFunction1D_sptr::element_type>(
            "IFunction1DAdapterTimesTwo", "        return 2*x"));
    TS_ASSERT(timesTwo);

    std::array<double, 10> xvalues, result;
    std::iota(std::begin(xvalues), std::end(xvalues), 0.0);
    timesTwo->function1D(result.data(), xvalues.data(), xvalues.size());

    std::array<double, 10> expected;
    std::transform(std::begin(xvalues), std::end(xvalues), std::begin(expected),
                   [](double x) { return 2. * x; });
    TS_ASSERT(
        std::equal(std::begin(result), std::end(result), std::begin(expected)));
  }

  void testfunction_Uses_Numerical_Deriv_When_Deriv_NotSupplied() {
    using Mantid::API::IFunction1D_sptr;
    using Mantid::PythonInterface::FunctionAdapterTestJacobian;
    using Mantid::PythonInterface::createTestFunction;
    IFunction1D_sptr noDerivFunc1D;
    TS_ASSERT_THROWS_NOTHING(
        noDerivFunc1D = createTestFunction<IFunction1D_sptr::element_type>(
            "IFunction1DAdapterWithDeriv",
            "        return self.getParameterValue(0)*x"));
    TS_ASSERT(noDerivFunc1D);

    std::array<double, 10> xvalues;
    std::iota(std::begin(xvalues), std::end(xvalues), 10.0);
    FunctionAdapterTestJacobian jacobian(xvalues.size(), 1);
    noDerivFunc1D->functionDeriv1D(&jacobian, xvalues.data(), xvalues.size());

    TS_ASSERT_DELTA(9.99999, jacobian.get(0, 0), 1e-05);
  }

  void testfunction_Uses_Supplied_Deriv() {
    using Mantid::API::IFunction1D_sptr;
    using Mantid::PythonInterface::FunctionAdapterTestJacobian;
    using Mantid::PythonInterface::createTestFunction;
    IFunction1D_sptr func1DWithDeriv;
    TS_ASSERT_THROWS_NOTHING(
        func1DWithDeriv = createTestFunction<IFunction1D_sptr::element_type>(
            "IFunction1DAdapterWithDeriv",
            "        return self.getParameterValue(0)*x",
            "        jacobian.set(0, 0, 1000)"));
    TS_ASSERT(func1DWithDeriv);

    std::array<double, 10> xvalues;
    std::iota(std::begin(xvalues), std::end(xvalues), 10.0);
    FunctionAdapterTestJacobian jacobian(xvalues.size(), 1);
    func1DWithDeriv->functionDeriv1D(&jacobian, xvalues.data(), xvalues.size());

    TS_ASSERT_DELTA(1000, jacobian.get(0, 0), 1e-05);
  }
};

// Performance test

class IFunction1DAdapterTestPerformance : public CxxTest::TestSuite {
public:
  static IFunction1DAdapterTestPerformance *createSuite() {
    return new IFunction1DAdapterTestPerformance();
  }
  static void destroySuite(IFunction1DAdapterTestPerformance *suite) {
    delete suite;
  }

public:
  IFunction1DAdapterTestPerformance() {
    using Mantid::API::IFunction1D;
    using Mantid::PythonInterface::createTestFunction;
    m_testFunction = createTestFunction<IFunction1D>(
        "IFunction1DPerfTest", "        return 1.0 + 2.0*x");
    // test data
    std::iota(std::begin(m_xdata), std::end(m_xdata), 0.0);
  }

  void test_IFunction1D_override_performance() {
    for (int i = 0; i < 50000; ++i) {
      m_testFunction->function1D(m_result.data(), m_xdata.data(),
                                 m_result.size());
    }
  }

private:
  Mantid::API::IFunction1D_sptr m_testFunction;
  using TestDataType = std::array<double, 1000>;
  TestDataType m_xdata;
  TestDataType m_result;
};

#endif // MANTID_PYTHONINTERFACE_IFUNCTION1DADAPTERTEST_H
