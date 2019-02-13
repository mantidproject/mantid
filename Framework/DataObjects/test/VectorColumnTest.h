// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAOBJECTS_VECTORCOLUMNTEST_H_
#define MANTID_DATAOBJECTS_VECTORCOLUMNTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/VectorColumn.h"

using Mantid::DataObjects::VectorColumn;

template <class Type> class VectorColumnTestHelper : public VectorColumn<Type> {
public:
  using VectorColumn<Type>::resize;
  using VectorColumn<Type>::insert;
  using VectorColumn<Type>::remove;
};

class VectorColumnTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static VectorColumnTest *createSuite() { return new VectorColumnTest(); }
  static void destroySuite(VectorColumnTest *suite) { delete suite; }

  void test_construction() {
    VectorColumn<int> col;
    TS_ASSERT_EQUALS(col.type(), "vector_int");

    VectorColumn<double> col2;
    TS_ASSERT_EQUALS(col2.type(), "vector_double");
  }

  void test_read() {
    VectorColumnTestHelper<int> col;

    col.resize(5);

    // Simple case
    TS_ASSERT_THROWS_NOTHING(col.read(0, "1,2,3"));
    std::vector<int> v1;
    v1.push_back(1);
    v1.push_back(2);
    v1.push_back(3);
    TS_ASSERT_EQUALS(col.cell<std::vector<int>>(0), v1);

    // Check if trimming works
    TS_ASSERT_THROWS_NOTHING(col.read(1, "  4, 5,  6"));
    std::vector<int> v2;
    v2.push_back(4);
    v2.push_back(5);
    v2.push_back(6);
    TS_ASSERT_EQUALS(col.cell<std::vector<int>>(1), v2);

    // Single element
    TS_ASSERT_THROWS_NOTHING(col.read(2, "7"));
    std::vector<int> v3;
    v3.push_back(7);
    TS_ASSERT_EQUALS(col.cell<std::vector<int>>(2), v3);

    // Empty string
    TS_ASSERT_THROWS_NOTHING(col.read(3, ""));
    std::vector<int> v4;
    TS_ASSERT_EQUALS(col.cell<std::vector<int>>(3), v4);
    TS_ASSERT_EQUALS(col.cell<std::vector<int>>(4), v4);

    // Non-convertable characters
    TS_ASSERT_THROWS(col.read(4, "1,2,a,3"), std::invalid_argument);
  }

  void test_print() {
    VectorColumnTestHelper<int> col;

    col.resize(3);

    // Simple case
    std::vector<int> v1;
    v1.push_back(11);
    v1.push_back(22);
    v1.push_back(33);
    v1.push_back(44);
    v1.push_back(55);
    col.cell<std::vector<int>>(0) = v1;
    std::ostringstream s1;
    TS_ASSERT_THROWS_NOTHING(col.print(0, s1));
    TS_ASSERT_EQUALS(s1.str(), "11,22,33,44,55");

    // Single element
    std::vector<int> v2;
    v2.push_back(9876);
    col.cell<std::vector<int>>(1) = v2;
    std::ostringstream s2;
    TS_ASSERT_THROWS_NOTHING(col.print(1, s2));
    TS_ASSERT_EQUALS(s2.str(), "9876");

    // Unset element
    std::ostringstream s3;
    TS_ASSERT_THROWS_NOTHING(col.print(2, s3));
    TS_ASSERT(s3.str().empty());
  }

  void test_sizeOfData() {
    VectorColumnTestHelper<int> col;

    col.resize(3);

    col.read(0, "1,2,3");
    col.read(1, "3,4,5");
    col.read(2, "7,8,9,10");

    TS_ASSERT_EQUALS(col.sizeOfData(), 10 * sizeof(int));
  }

  void test_cannotBeConvertedToDouble() {
    VectorColumnTestHelper<int> col;
    TS_ASSERT(!col.isNumber());
  }

  void test_equals() {
    VectorColumnTestHelper<int> col;

    col.resize(3);

    col.read(0, "1,2,3");
    col.read(1, "3,4,5");
    col.read(2, "7,8,9,10");
    auto compare = std::unique_ptr<Mantid::API::Column>(col.clone());

    TS_ASSERT(col.equals(*compare, 0));
  }

  void test_equals_failure() {
    VectorColumnTestHelper<int> col;
    VectorColumnTestHelper<int> col2;

    col.resize(3);
    col2.resize(3);

    col.read(0, "1,2,3");
    col.read(1, "3,4,5");
    col.read(2, "7,8,9,10");
    auto compare = std::unique_ptr<Mantid::API::Column>(col.clone());
    col2.read(0, "1,2,3");
    col2.read(1, "3,4,5");
    col2.read(2, "7,8,9,11");
    TS_ASSERT(!col2.equals(*compare, 0));
  }

  void test_equals_tolerance() {
    VectorColumnTestHelper<int> col;
    VectorColumnTestHelper<int> col2;

    col.resize(3);
    col2.resize(3);

    col.read(0, "1,2,3");
    col.read(1, "3,4,5");
    col.read(2, "7,8,9,10");
    auto compare = std::unique_ptr<Mantid::API::Column>(col.clone());
    col2.read(0, "1,2,2");
    col2.read(1, "3,4,5");
    col2.read(2, "7,8,9,11");
    TS_ASSERT(col2.equals(*compare, 1));
  }

  void test_equals_tolerance_fail() {
    VectorColumnTestHelper<int> col;
    VectorColumnTestHelper<int> col2;

    col.resize(3);
    col2.resize(3);

    col.read(0, "1,2,3");
    col.read(1, "3,4,5");
    col.read(2, "7,8,9,10");
    auto compare = std::unique_ptr<Mantid::API::Column>(col.clone());
    col2.read(0, "1,2,2");
    col2.read(1, "3,4,5");
    col2.read(2, "7,8,9,12");
    TS_ASSERT(!col2.equals(*compare, 1));
  }

  void test_equalsRelErr() {
    VectorColumnTestHelper<int> col;
    VectorColumnTestHelper<int> col2;

    col.resize(3);
    col2.resize(3);

    col.read(0, "100,2,3");
    col.read(1, "3,4,5");
    col.read(2, "7,8,9,10");
    auto compare = std::unique_ptr<Mantid::API::Column>(col.clone());
    col2.read(0, "90,2,2");
    col2.read(1, "3,4,5");
    col2.read(2, "7,8,9,11");
    TS_ASSERT(col2.equalsRelErr(*compare, 1));
  }
};

#endif /* MANTID_DATAOBJECTS_VECTORCOLUMNTEST_H_ */
