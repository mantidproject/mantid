#ifndef IPEAKFUNCTIONADAPTERTEST_H
#define IPEAKFUNCTIONADAPTERTEST_H

#include "FunctionAdapterTestCommon.h"
#include "MantidPythonInterface/api/FitFunctions/IPeakFunctionAdapter.h"
#include "MantidPythonInterface/kernel/Environment/ErrorHandling.h"
#include <cxxtest/TestSuite.h>

class IPeakFunctionAdapterTest : public CxxTest::TestSuite {
public:
  static IPeakFunctionAdapterTest *createSuite() {
    return new IPeakFunctionAdapterTest();
  }
  static void destroySuite(IPeakFunctionAdapterTest *suite) { delete suite; }

  void testfunctionLocal_Returning_Non_Numpy_Array_Throws() {
    using Mantid::API::IPeakFunction_sptr;
    using Mantid::PythonInterface::createTestFunction;
    const auto funcImpl = "        return 1";
    IPeakFunction_sptr badReturnFunc;
    TS_ASSERT_THROWS_NOTHING(
        badReturnFunc = createTestFunction<IPeakFunction_sptr::element_type>(
            "BadReturnFunction", funcImpl));
    TS_ASSERT(badReturnFunc);

    std::array<double, 1> xvalues{10}, retvalue{0};
    TS_ASSERT_THROWS(
        badReturnFunc->function1D(retvalue.data(), xvalues.data(), 1),
        std::runtime_error);
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

    std::array<double, 1> xvalues{10}, retvalue{0};
    TS_ASSERT_THROWS(
        badNdArrayFunc->function1D(retvalue.data(), xvalues.data(), 1),
        std::runtime_error);
  }
};

#endif // IPEAKFUNCTIONADAPTERTEST_H
