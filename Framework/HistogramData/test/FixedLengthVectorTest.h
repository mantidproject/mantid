// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_HISTOGRAMDATA_FIXEDLENGTHVECTORTEST_H_
#define MANTID_HISTOGRAMDATA_FIXEDLENGTHVECTORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/FixedLengthVector.h"
#include "MantidKernel/make_cow.h"

using Mantid::HistogramData::detail::FixedLengthVector;
using Mantid::Kernel::make_cow;

struct FixedLengthVectorTester
    : public FixedLengthVector<FixedLengthVectorTester> {
  FixedLengthVectorTester() = default;
  using FixedLengthVector<FixedLengthVectorTester>::FixedLengthVector;
  using FixedLengthVector<FixedLengthVectorTester>::operator=;
};

class FixedLengthVectorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FixedLengthVectorTest *createSuite() {
    return new FixedLengthVectorTest();
  }
  static void destroySuite(FixedLengthVectorTest *suite) { delete suite; }

  void test_empty_constructor() {
    const FixedLengthVectorTester values{};
    TS_ASSERT_EQUALS(values.size(), 0);
  }

  void test_length_zero_constructor() {
    const FixedLengthVectorTester values(0);
    TS_ASSERT_EQUALS(values.size(), 0);
  }

  void test_count_value_constructor() {
    const FixedLengthVectorTester values(2, 0.1);
    TS_ASSERT_EQUALS(values.size(), 2);
    TS_ASSERT_EQUALS(values[0], 0.1);
    TS_ASSERT_EQUALS(values[1], 0.1);
  }

  void test_length_zero_value_constructor() {
    const FixedLengthVectorTester values(0, 0.1);
    TS_ASSERT_EQUALS(values.size(), 0);
  }

  void test_count_constructor() {
    const FixedLengthVectorTester values(2);
    TS_ASSERT_EQUALS(values.size(), 2);
    TS_ASSERT_EQUALS(values[0], 0.0);
    TS_ASSERT_EQUALS(values[1], 0.0);
  }

  void test_initializer_list_constructor() {
    const FixedLengthVectorTester values{0.1, 0.2, 0.3};
    TS_ASSERT_EQUALS(values.size(), 3);
    TS_ASSERT_EQUALS(values[0], 0.1);
    TS_ASSERT_EQUALS(values[1], 0.2);
    TS_ASSERT_EQUALS(values[2], 0.3);
  }

  void test_empty_initializer_list_constructor() {
    std::initializer_list<double> empty_list;
    const FixedLengthVectorTester values(empty_list);
    TS_ASSERT_EQUALS(values.size(), 0);
  }

  void test_copy_constructor() {
    const FixedLengthVectorTester src(2, 0.1);
    const FixedLengthVectorTester dest(src);
    TS_ASSERT_EQUALS(dest[0], 0.1);
    TS_ASSERT_EQUALS(dest[1], 0.1);
  }

  void test_move_constructor() {
    FixedLengthVectorTester src(2, 0.1);
    TS_ASSERT_EQUALS(src.size(), 2);
    const FixedLengthVectorTester dest(std::move(src));
    TS_ASSERT_EQUALS(src.size(), 0);
    TS_ASSERT_EQUALS(dest[0], 0.1);
    TS_ASSERT_EQUALS(dest[1], 0.1);
  }

  void test_iterator_constructor() {
    std::vector<double> data{0.1, 0.2, 0.3, 0.4};
    FixedLengthVectorTester testee(data.begin() + 1, data.end() - 1);
    TS_ASSERT_EQUALS(testee.size(), 2);
    TS_ASSERT_EQUALS(testee[0], 0.2);
    TS_ASSERT_EQUALS(testee[1], 0.3);
  }

  void test_generator_constructor() {
    FixedLengthVectorTester testee(2, []() { return 0.1; });
    TS_ASSERT_EQUALS(testee.size(), 2);
    TS_ASSERT_EQUALS(testee[0], 0.1);
    TS_ASSERT_EQUALS(testee[1], 0.1);
  }

  void test_iterator_constructor_special_case() {
    // Used like this, we might think that the (count, value) constructor is
    // called. However, that would require converting the second int to a
    // double, so actually the templated iterator constructor matches. We could
    // prevent that with some complicated SFINAE (as done by std::vector),
    // however it actually does not seem to matter: We just forward to the
    // std::vector constructor, where the templated constructor does not work,
    // so we are back to the std::vector(count, value) constructor, after the
    // detour to the templated FixedLengthVector constructor.
    FixedLengthVectorTester testee(3, 1);
    TS_ASSERT_EQUALS(testee.size(), 3);
    TS_ASSERT_EQUALS(testee[0], 1.0);
    TS_ASSERT_EQUALS(testee[1], 1.0);
    TS_ASSERT_EQUALS(testee[2], 1.0);
  }

  void test_range_assignment() {
    std::vector<double> src{3.6, 9.7, 8.5};
    FixedLengthVectorTester dest(3);

    dest.assign(src.cbegin(), src.cend());

    TS_ASSERT_EQUALS(dest.size(), 3);
    TS_ASSERT_EQUALS(dest[0], src[0]);
    TS_ASSERT_EQUALS(dest[1], src[1]);
    TS_ASSERT_EQUALS(dest[2], src[2]);
  }

  void test_range_assignment_fail() {
    std::vector<double> src(10, 0);
    FixedLengthVectorTester dest(5);

    TS_ASSERT_THROWS(dest.assign(src.cbegin(), src.cend()),
                     const std::logic_error &);
  }

  void test_length_value_assignment() {
    FixedLengthVectorTester dest(4);

    dest.assign(4, 3.9);

    TS_ASSERT_EQUALS(dest.size(), 4);
    TS_ASSERT_EQUALS(dest[0], 3.9);
    TS_ASSERT_EQUALS(dest[1], 3.9);
    TS_ASSERT_EQUALS(dest[2], 3.9);
    TS_ASSERT_EQUALS(dest[3], 3.9);
  }

  void test_value_assignment_fail() {
    FixedLengthVectorTester dest(3);

    TS_ASSERT_THROWS(dest.assign(20, 4.5), const std::logic_error &);
  }

  void test_copy_assignment() {
    const FixedLengthVectorTester src(2, 0.1);
    FixedLengthVectorTester dest(2);
    dest = src;
    TS_ASSERT_EQUALS(dest.size(), 2);
    TS_ASSERT_EQUALS(dest[0], 0.1);
    TS_ASSERT_EQUALS(dest[1], 0.1);
  }

  void test_copy_assignment_fail() {
    const FixedLengthVectorTester src(2, 0.1);
    FixedLengthVectorTester dest(1);
    TS_ASSERT_THROWS(dest = src, const std::logic_error &);
  }

  void test_move_assignment() {
    FixedLengthVectorTester src(2, 0.1);
    FixedLengthVectorTester dest(2);
    dest = std::move(src);
    TS_ASSERT_EQUALS(src.size(), 0);
    TS_ASSERT_EQUALS(dest[0], 0.1);
    TS_ASSERT_EQUALS(dest[1], 0.1);
  }

  void test_move_assignment_fail() {
    FixedLengthVectorTester src(2, 0.1);
    FixedLengthVectorTester dest(1);
    TS_ASSERT_THROWS(dest = std::move(src), const std::logic_error &);
  }

  void test_initializer_list_assignment() {
    FixedLengthVectorTester values(3);
    values = {0.1, 0.2, 0.3};
    TS_ASSERT_EQUALS(values.size(), 3);
    TS_ASSERT_EQUALS(values[0], 0.1);
    TS_ASSERT_EQUALS(values[1], 0.2);
    TS_ASSERT_EQUALS(values[2], 0.3);
  }

  void test_empty_initializer_list_assignment() {
    std::initializer_list<double> empty_list;
    FixedLengthVectorTester values(0);
    values = empty_list;
    TS_ASSERT_EQUALS(values.size(), 0);
  }

  void test_initializer_list_assignment_fail() {
    FixedLengthVectorTester values(2);
    TS_ASSERT_THROWS((values = {0.1, 0.2, 0.3}), const std::logic_error &);
  }

  void test_vector_constructor() {
    const std::vector<double> vector(2, 0.1);
    FixedLengthVectorTester values(vector);
    TS_ASSERT_EQUALS(values.size(), 2);
    TS_ASSERT_EQUALS(values[0], 0.1);
    TS_ASSERT_EQUALS(values[1], 0.1);
  }

  void test_vector_move_constructor() {
    std::vector<double> vector(2, 0.1);
    FixedLengthVectorTester values(std::move(vector));
    TS_ASSERT_EQUALS(vector.size(), 0);
    TS_ASSERT_EQUALS(values.size(), 2);
    TS_ASSERT_EQUALS(values[0], 0.1);
    TS_ASSERT_EQUALS(values[1], 0.1);
  }

  void test_vector_assignment() {
    const std::vector<double> vector{0.1, 0.2};
    FixedLengthVectorTester values(2);
    TS_ASSERT_THROWS_NOTHING(values = vector);
    TS_ASSERT_EQUALS(values.size(), 2);
    TS_ASSERT_EQUALS(values[0], 0.1);
    TS_ASSERT_EQUALS(values[1], 0.2);
  }

  void test_vector_move_assignment() {
    std::vector<double> vector{0.1, 0.2};
    FixedLengthVectorTester values(2);
    TS_ASSERT_THROWS_NOTHING(values = std::move(vector));
    TS_ASSERT_EQUALS(vector.size(), 0);
    TS_ASSERT_EQUALS(values.size(), 2);
    TS_ASSERT_EQUALS(values[0], 0.1);
    TS_ASSERT_EQUALS(values[1], 0.2);
  }

  void test_value_assignment() {
    FixedLengthVectorTester dest(4);

    dest = 3.9;

    TS_ASSERT_EQUALS(dest.size(), 4);
    TS_ASSERT_EQUALS(dest[0], 3.9);
    TS_ASSERT_EQUALS(dest[1], 3.9);
    TS_ASSERT_EQUALS(dest[2], 3.9);
    TS_ASSERT_EQUALS(dest[3], 3.9);
  }

  void test_empty() {
    TS_ASSERT_EQUALS(FixedLengthVectorTester(0).empty(), true);
    TS_ASSERT_EQUALS(FixedLengthVectorTester(1).empty(), false);
  }

  void test_size() {
    const FixedLengthVectorTester values(42);
    TS_ASSERT_EQUALS(values.size(), 42);
  }

  void test_const_index_operator() {
    const FixedLengthVectorTester data{0.1, 0.2};
    TS_ASSERT_EQUALS(data[0], 0.1);
    TS_ASSERT_EQUALS(data[1], 0.2);
  }

  void test_index_operator() {
    FixedLengthVectorTester data{0.1, 0.2};
    TS_ASSERT_EQUALS(data[0], 0.1);
    TS_ASSERT_EQUALS(data[1], 0.2);
  }

  void test_front_back() {
    FixedLengthVectorTester data{0.1, 0.2, 0.4};
    data.front() += 1.0;
    data.back() += 1.0;
    TS_ASSERT_EQUALS(data.front(), 1.1);
    TS_ASSERT_EQUALS(data.back(), 1.4);
  }

  void test_const_front_back() {
    const FixedLengthVectorTester data{0.1, 0.2, 0.4};
    TS_ASSERT_EQUALS(data.front(), 0.1);
    TS_ASSERT_EQUALS(data.back(), 0.4);
  }

  void test_sum_vector() {
    const FixedLengthVectorTester data{0.1, 0.2, 0.4};
    TS_ASSERT_DELTA(data.sum(), 0.7, 1e-6);
    TS_ASSERT_DELTA(data.sum(1), 0.6, 1e-6);
    TS_ASSERT_DELTA(data.sum(0, 2), 0.3, 1e-6);
    TS_ASSERT_DELTA(data.sum(0, 2, 10.0), 10.3, 1e-6);
  }
};

#endif /* MANTID_HISTOGRAMDATA_FIXEDLENGTHVECTORTEST_H_ */
