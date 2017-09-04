#ifndef NDARRAY1DTEST_H
#define NDARRAY1DTEST_H

#include "cxxtest/TestSuite.h"

#include "MantidQtWidgets/MplCpp/NDArray1D.h"

class NDArray1DTest : public CxxTest::TestSuite {
public:
  static NDArray1DTest *createSuite() { return new NDArray1DTest; }
  static void destroySuite(NDArray1DTest *suite) { delete suite; }

  //---------------------------------------------------------------------------
  // Success tests
  //---------------------------------------------------------------------------
  void test_create_from_vector() {
    using MantidQt::Widgets::MplCpp::NDArray1D;
    const std::vector<double> values = {1, 2, 3, 4, 5};
    NDArray1D<double> ndarray(values);
    TSM_ASSERT("Object from vector should not be None", !ndarray.isNone());
    auto shape = ndarray.shape();
    TSM_ASSERT_EQUALS("Shape array should be 1D", 1, shape.size());
    TSM_ASSERT_EQUALS("Array length should match vector input", values.size(),
                      shape[0]);
  }

  void test_index_access_in_bounds_gives_expected_result() {
    using MantidQt::Widgets::MplCpp::NDArray1D;
    const std::vector<double> values = {1, 2, 3, 4, 5};
    NDArray1D<double> ndarray(values);
    TS_ASSERT_EQUALS(3.0, ndarray[2]);
  }

  //---------------------------------------------------------------------------
  // Failure tests
  //---------------------------------------------------------------------------
};
#endif // NDARRAY1DTEST_H
