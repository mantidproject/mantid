#ifndef MANTID_HISTOGRAM_VECTOROFTEST_H_
#define MANTID_HISTOGRAM_VECTOROFTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/make_cow.h"
#include "MantidHistogram/VectorOf.h"
#include "MantidHistogram/ConstIterable.h"

using namespace Mantid;
using namespace Histogram;
using Mantid::Kernel::cow_ptr;
using Mantid::Kernel::make_cow;

class VectorOfTester : public VectorOf<VectorOfTester>,
                       public ConstIterable<VectorOfTester> {
public:
  using VectorOf<VectorOfTester>::VectorOf;
  using VectorOf<VectorOfTester>::operator=;
  VectorOfTester() = default;
};

class VectorOfTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static VectorOfTest *createSuite() { return new VectorOfTest(); }
  static void destroySuite(VectorOfTest *suite) { delete suite; }

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

  void test_length_zero_value_constructor() {
    const VectorOfTester values(0, 0.1);
    TS_ASSERT_EQUALS(values.size(), 0);
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

  void test_empty_initializer_list_constructor() {
    std::initializer_list<double> empty_list;
    const VectorOfTester values(empty_list);
    // Note difference to default constructor: No elements, but allocated.
    TS_ASSERT(values);
    TS_ASSERT_EQUALS(values.size(), 0);
  }

  void test_copy_constructor() {
    const VectorOfTester src(2, 0.1);
    const VectorOfTester dest(src);
    TS_ASSERT_EQUALS(dest[0], 0.1);
    TS_ASSERT_EQUALS(dest[1], 0.1);
  }

  void test_copy_from_null_constructor() {
    const VectorOfTester src{};
    const VectorOfTester dest(src);
    TS_ASSERT(!dest);
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

  void test_move_from_null_constructor() {
    VectorOfTester src{};
    const VectorOfTester dest(std::move(src));
    TS_ASSERT(!src);
    TS_ASSERT(!dest);
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

  void test_copy_assignment_from_null() {
    const VectorOfTester src{};
    VectorOfTester dest(1);
    dest = src;
    TS_ASSERT(!dest);
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

  void test_move_assignment_from_null() {
    VectorOfTester src{};
    VectorOfTester dest(1);
    dest = std::move(src);
    TS_ASSERT(!src);
    TS_ASSERT(!dest);
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

  void test_empty_initializer_list_assignment() {
    std::initializer_list<double> empty_list;
    VectorOfTester values(1);
    values = empty_list;
    TS_ASSERT_EQUALS(values.size(), 0);
  }

  void test_cow_ptr_constructor() {
    auto cow = make_cow<std::vector<double>>(2, 0.1);
    VectorOfTester values(cow);
    TS_ASSERT(values);
    TS_ASSERT_EQUALS(values.size(), 2);
    TS_ASSERT_EQUALS(values[0], 0.1);
    TS_ASSERT_EQUALS(values[1], 0.1);
    TS_ASSERT_EQUALS(&values.constData(), cow.get());
  }

  void test_null_cow_ptr_constructor() {
    cow_ptr<std::vector<double>> cow(nullptr);
    VectorOfTester values(cow);
    TS_ASSERT(!values);
  }

  void test_shared_ptr_constructor() {
    auto shared = boost::make_shared<std::vector<double>>(2, 0.1);
    VectorOfTester values(shared);
    TS_ASSERT(values);
    TS_ASSERT_EQUALS(values.size(), 2);
    TS_ASSERT_EQUALS(values[0], 0.1);
    TS_ASSERT_EQUALS(values[1], 0.1);
    TS_ASSERT_EQUALS(&values.constData(), shared.get());
  }

  void test_null_shared_ptr_constructor() {
    boost::shared_ptr<std::vector<double>> shared;
    VectorOfTester values(shared);
    TS_ASSERT(!values);
  }

  void test_vector_constructor() {
    std::vector<double> vector(2, 0.1);
    VectorOfTester values(vector);
    TS_ASSERT(values);
    TS_ASSERT_EQUALS(values.size(), 2);
    TS_ASSERT_EQUALS(values[0], 0.1);
    TS_ASSERT_EQUALS(values[1], 0.1);
  }

  void test_cow_ptr_assignment() {
    auto cow = make_cow<std::vector<double>>(2, 0.1);
    VectorOfTester values(1);
    values = cow;
    TS_ASSERT(values);
    TS_ASSERT_EQUALS(values.size(), 2);
    TS_ASSERT_EQUALS(values[0], 0.1);
    TS_ASSERT_EQUALS(values[1], 0.1);
    TS_ASSERT_EQUALS(&values.constData(), cow.get());
  }

  void test_null_cow_ptr_assignment() {
    cow_ptr<std::vector<double>> cow(nullptr);
    VectorOfTester values(1);
    values = cow;
    TS_ASSERT(!values);
  }

  void test_cow_ptr_self_assignment() {
    VectorOfTester values(2, 0.1);
    const auto *raw_data = &values.constData();
    auto cow = values.cowData();
    values = cow;
    TS_ASSERT(values);
    TS_ASSERT_EQUALS(cow.use_count(), 2);
    TS_ASSERT_EQUALS(&values.constData(), raw_data);
    TS_ASSERT_EQUALS(values.size(), 2);
    TS_ASSERT_EQUALS(values[0], 0.1);
    TS_ASSERT_EQUALS(values[1], 0.1);
    TS_ASSERT_EQUALS(&values.constData(), cow.get());
  }

  void test_shared_ptr_assignment() {
    auto shared = boost::make_shared<std::vector<double>>(2, 0.1);
    VectorOfTester values(1);
    values = shared;
    TS_ASSERT(values);
    TS_ASSERT_EQUALS(values.size(), 2);
    TS_ASSERT_EQUALS(values[0], 0.1);
    TS_ASSERT_EQUALS(values[1], 0.1);
    TS_ASSERT_EQUALS(&values.constData(), shared.get());
  }

  void test_null_shared_ptr_assignment() {
    boost::shared_ptr<std::vector<double>> shared;
    VectorOfTester values(1);
    values = shared;
    TS_ASSERT(!values);
  }

  void test_shared_ptr_self_assignment() {
    auto shared = boost::make_shared<std::vector<double>>(2, 0.1);
    VectorOfTester values(1);
    values = shared;
    TS_ASSERT_THROWS_NOTHING(values = shared);
    TS_ASSERT(values);
    TS_ASSERT_EQUALS(shared.use_count(), 2);
    TS_ASSERT_EQUALS(values.size(), 2);
    TS_ASSERT_EQUALS(values[0], 0.1);
    TS_ASSERT_EQUALS(values[1], 0.1);
    TS_ASSERT_EQUALS(&values.constData(), shared.get());
  }

  void test_vector_assignment() {
    const std::vector<double> raw{0.1, 0.2, 0.3};
    VectorOfTester values;
    TS_ASSERT_THROWS_NOTHING(values = raw);
    TS_ASSERT(values);
    TS_ASSERT_DIFFERS(&(values.constData()), &raw);
    TS_ASSERT_EQUALS(values.size(), 3);
    TS_ASSERT_EQUALS(values[0], 0.1);
  }

  void test_vector_self_assignment() {
    VectorOfTester values(2, 0.1);
    // Reference to internal data
    auto &vector = values.constData();
    values = vector;
    TS_ASSERT(values);
    // Reference still valid after self assignment
    TS_ASSERT_EQUALS(&(values.constData()), &vector);
  }

  void test_operator_bool() {
    VectorOfTester null;
    TS_ASSERT(!null);
    VectorOfTester not_null(0);
    TS_ASSERT(not_null);
  }

  void test_size() {
    const VectorOfTester values(42);
    TS_ASSERT_EQUALS(values.size(), 42);
  }

  void test_data_const() {
    const VectorOfTester values(2, 0.1);
    auto copy(values);
    TS_ASSERT_EQUALS(&copy.constData(), &values.constData());
    const auto &data = values.data();
    TS_ASSERT_EQUALS(&copy.constData(), &values.constData());
    TS_ASSERT_EQUALS(data.size(), 2);
  }

  void test_const_data() {
    VectorOfTester values(2, 0.1);
    auto copy(values);
    TS_ASSERT_EQUALS(&copy.constData(), &values.constData());
    const auto &data = values.constData();
    TS_ASSERT_EQUALS(&copy.constData(), &values.constData());
    TS_ASSERT_EQUALS(data.size(), 2);
  }

  void test_data() {
    VectorOfTester values(2, 0.1);
    auto copy(values);
    TS_ASSERT_EQUALS(&copy.constData(), &values.constData());
    auto &data = values.data();
    TS_ASSERT_DIFFERS(&copy.constData(), &values.constData());
    TS_ASSERT_EQUALS(data.size(), 2);
  }

  void test_cow_data() {
    VectorOfTester values(2, 0.1);
    auto cow = values.cowData();
    TS_ASSERT_EQUALS(cow.get(), &values.constData());
    TS_ASSERT_EQUALS(cow.use_count(), 2);
    auto &data = values.data();
    TS_ASSERT_DIFFERS(cow.get(), &values.constData());
    TS_ASSERT_EQUALS(cow.use_count(), 1);
    TS_ASSERT_EQUALS(data.size(), 2);
  }
};

#endif /* MANTID_HISTOGRAM_VECTOROFTEST_H_ */
