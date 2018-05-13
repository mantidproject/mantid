#ifndef MANTID_PYTHONINTERFACE_IFUNCTION1DADAPTERTEST_H
#define MANTID_PYTHONINTERFACE_IFUNCTION1DADAPTERTEST_H

#include "MantidAPI/FunctionFactory.h"
#include "MantidPythonInterface/api/FitFunctions/IFunction1DAdapter.h"
#include "MantidPythonInterface/kernel/Environment/ErrorHandling.h"
#include <cxxtest/TestSuite.h>

class IFunction1DAdapterTest : public CxxTest::TestSuite {
public:
  static IFunction1DAdapterTest *createSuite() {
    return new IFunction1DAdapterTest();
  }
  static void destroySuite(IFunction1DAdapterTest *suite) { delete suite; }

  void testfunction1D_Returning_Non_Numpy_Array_Throws() {
    using Mantid::API::FunctionFactory;
    using Mantid::API::IFunction1D;
    using Mantid::API::IFunction_sptr;
    auto code = "from mantid.api import IFunction1D, FunctionFactory\n"
                "class BadReturnFunction(IFunction1D):\n"
                "    def init(self):\n"
                "        pass\n"
                "    def function1D(self, x):\n"
                "        return 1\n\n"
                "FunctionFactory.Instance().subscribe(BadReturnFunction)\n";
    PyRun_SimpleString(code);
    IFunction_sptr badReturnFunc;
    TS_ASSERT_THROWS_NOTHING(
        badReturnFunc =
            FunctionFactory::Instance().createFunction("BadReturnFunction"));
    auto badReturnFunc1D =
        boost::dynamic_pointer_cast<IFunction1D>(badReturnFunc);
    TS_ASSERT(badReturnFunc1D);

    std::array<double, 1> xvalues{10}, retvalue{0};
    TS_ASSERT_THROWS(
        badReturnFunc1D->function1D(retvalue.data(), xvalues.data(), 1),
        std::runtime_error);
  }

  void testfunction1D_Returning_Numpy_Array_With_NonFloat_Type_Throws() {
    using Mantid::API::FunctionFactory;
    using Mantid::API::IFunction1D;
    using Mantid::API::IFunction_sptr;
    auto code = "from mantid.api import IFunction1D, FunctionFactory\n"
                "import numpy as np\n"
                "class BadNDArrayFunction(IFunction1D):\n"
                "    def init(self):\n"
                "        pass\n"
                "    def function1D(self, x):\n"
                "        return np.array([1])\n\n"
                "FunctionFactory.Instance().subscribe(BadNDArrayFunction)\n";
    PyRun_SimpleString(code);
    IFunction_sptr badNdArrayFunc;
    TS_ASSERT_THROWS_NOTHING(
        badNdArrayFunc =
            FunctionFactory::Instance().createFunction("BadNDArrayFunction"));
    auto badNdArrayFunc1D =
        boost::dynamic_pointer_cast<IFunction1D>(badNdArrayFunc);
    TS_ASSERT(badNdArrayFunc1D);

    std::array<double, 1> xvalues{10}, retvalue{0};
    TS_ASSERT_THROWS(
        badNdArrayFunc1D->function1D(retvalue.data(), xvalues.data(), 1),
        std::runtime_error);
  }
};

#endif // MANTID_PYTHONINTERFACE_IFUNCTION1DADAPTERTEST_H
