#ifndef MANTID_PYTHONINTERFACE_IFUNCTION1DADAPTERTEST_H
#define MANTID_PYTHONINTERFACE_IFUNCTION1DADAPTERTEST_H

#include "MantidPythonInterface/api/FitFunctions/IFunction1DAdapter.h"
#include "MantidPythonInterface/kernel/Environment/ErrorHandling.h"
#include "FunctionAdapterTestCommon.h"
#include <cxxtest/TestSuite.h>

class IFunction1DAdapterTest : public CxxTest::TestSuite {
public:
  static IFunction1DAdapterTest *createSuite() {
    return new IFunction1DAdapterTest();
  }
  static void destroySuite(IFunction1DAdapterTest *suite) { delete suite; }

  void testfunction1D_Returning_Non_Numpy_Array_Throws() {
    using Mantid::API::IFunction1D_sptr;
    using Mantid::PythonInterface::createTestFunction;
    const auto func1DImpl = "        return 1";
    IFunction1D_sptr badReturnFunc1D;
    TS_ASSERT_THROWS_NOTHING(
        badReturnFunc1D = createTestFunction<IFunction1D_sptr::element_type>(
            "BadReturnFunction", func1DImpl));
    TS_ASSERT(badReturnFunc1D);

    std::array<double, 1> xvalues{10}, retvalue{0};
    TS_ASSERT_THROWS(
        badReturnFunc1D->function1D(retvalue.data(), xvalues.data(), 1),
        std::runtime_error);
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

    std::array<double, 1> xvalues{10}, retvalue{0};
    TS_ASSERT_THROWS(
        badNdArrayFunc1D->function1D(retvalue.data(), xvalues.data(), 1),
        std::runtime_error);
  }
};

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
