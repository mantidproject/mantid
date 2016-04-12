#ifndef MANTID_KERNEL_HISTOGRAM_VECTOROFTEST_H_
#define MANTID_KERNEL_HISTOGRAM_VECTOROFTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/Histogram/VectorOf.h"
#include "MantidKernel/Histogram/ConstIterable.h"

using Mantid::Kernel::VectorOf;
using Mantid::Kernel::ConstIterable;

class VectorOfTester : public VectorOf<VectorOfTester>,
                       public ConstIterable<VectorOfTester> {
public:
  using VectorOf<VectorOfTester>::VectorOf;
  using VectorOf<VectorOfTester>::operator=;
  VectorOfTester() = default;
};

class HistogramVectorOfTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HistogramVectorOfTest *createSuite() {
    return new HistogramVectorOfTest();
  }
  static void destroySuite(HistogramVectorOfTest *suite) { delete suite; }

  void test_empty_constructor() {
    const VectorOfTester values{};
    TS_ASSERT(!values);
  }

  void test_length_zero_constructor() {
    const VectorOfTester values(0);
    TS_ASSERT_EQUALS(values.size(), 0);
  }

  void test_count_value_constructor() {
    const VectorOfTester values(2, 0.1);
    TS_ASSERT_EQUALS(values.size(), 2);
    TS_ASSERT_EQUALS(values[0], 0.1);
    TS_ASSERT_EQUALS(values[1], 0.1);
  }

  void test_count_constructor() {
    const VectorOfTester values(2);
    TS_ASSERT_EQUALS(values.size(), 2);
    TS_ASSERT_EQUALS(values[0], 0.0);
    TS_ASSERT_EQUALS(values[1], 0.0);
  }

  void test_initializer_list_constructor() {
    const VectorOfTester values{0.1, 0.2, 0.3};
    TS_ASSERT_EQUALS(values.size(), 3);
    TS_ASSERT_EQUALS(values[0], 0.1);
    TS_ASSERT_EQUALS(values[1], 0.2);
    TS_ASSERT_EQUALS(values[2], 0.3);
  }

  void test_copy_constructor() {
    const VectorOfTester src(2, 0.1);
    const VectorOfTester dest(src);
    TS_ASSERT_EQUALS(dest[0], 0.1);
    TS_ASSERT_EQUALS(dest[1], 0.1);
  }

  void test_move_constructor() {
    VectorOfTester src(2, 0.1);
    TS_ASSERT_EQUALS(src.size(), 2);
    TS_ASSERT(src);
    const VectorOfTester dest(std::move(src));
    TS_ASSERT(!src);
    TS_ASSERT_EQUALS(dest[0], 0.1);
    TS_ASSERT_EQUALS(dest[1], 0.1);
  }

  void test_copy_assignment() {
    const VectorOfTester src(2, 0.1);
    VectorOfTester dest(1);
    TS_ASSERT_EQUALS(dest.size(), 1);
    TS_ASSERT_EQUALS(dest[0], 0.0);
    dest = src;
    TS_ASSERT_EQUALS(dest.size(), 2);
    TS_ASSERT_EQUALS(dest[0], 0.1);
    TS_ASSERT_EQUALS(dest[1], 0.1);
  }

  void test_move_assignment() {
    VectorOfTester src(2, 0.1);
    VectorOfTester dest(1);
    TS_ASSERT_EQUALS(dest.size(), 1);
    TS_ASSERT_EQUALS(dest[0], 0.0);
    TS_ASSERT(src);
    dest = std::move(src);
    TS_ASSERT(!src);
    TS_ASSERT_EQUALS(dest[0], 0.1);
    TS_ASSERT_EQUALS(dest[1], 0.1);
  }

  void test_initializer_list_assignment() {
    VectorOfTester values(2, 0.1);
    TS_ASSERT_EQUALS(values.size(), 2);
    TS_ASSERT_EQUALS(values[0], 0.1);
    TS_ASSERT_EQUALS(values[1], 0.1);
    values = {0.1, 0.2, 0.3};
    TS_ASSERT_EQUALS(values.size(), 3);
    TS_ASSERT_EQUALS(values[0], 0.1);
    TS_ASSERT_EQUALS(values[1], 0.2);
    TS_ASSERT_EQUALS(values[2], 0.3);
  }

  void test_vector_assignment() {
    std::vector<double> raw{0.1, 0.2, 0.3};
    VectorOfTester values;
    TS_ASSERT(!values);
    TS_ASSERT_THROWS_NOTHING(values = raw);
    TS_ASSERT(values);
    TS_ASSERT_DIFFERS(&(values.constData()), &raw);
    TS_ASSERT_EQUALS(raw.size(), 3);
    TS_ASSERT_EQUALS(raw[0], 0.1);
    TS_ASSERT_EQUALS(values.size(), 3);
    TS_ASSERT_EQUALS(values[0], 0.1);
  }

  void test_size() {
    const VectorOfTester values(42);
    TS_ASSERT_EQUALS(values.size(), 42);
  }

  void test_data() {
    const VectorOfTester values(2, 0.1);
    const auto &data = values.data();
    TS_ASSERT_EQUALS(data.size(), 2);
    TS_ASSERT_EQUALS(data[0], 0.1);
    TS_ASSERT_EQUALS(data[1], 0.1);
  }
};

#endif /* MANTID_KERNEL_HISTOGRAM_VECTOROFTEST_H_ */
