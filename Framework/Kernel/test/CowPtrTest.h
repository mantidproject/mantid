#ifndef COW_PTR_TEST_H_
#define COW_PTR_TEST_H_

#include "MantidKernel/cow_ptr.h"
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <cxxtest/TestSuite.h>

using namespace Mantid::Kernel;

namespace {

struct MyType {
public:
  MyType() : value(0) {}
  MyType(int val) : value(val) {}
  int value;
};
} // namespace

class CowPtrTest : public CxxTest::TestSuite {
public:
  void testDefaultConstruct() {

    cow_ptr<MyType> cow{};
    TSM_ASSERT_EQUALS("Should give us the default of the default constructed T",
                      0, cow->value);
  }

  void testConstructorNullptr() {
    cow_ptr<MyType> cow{nullptr};
    TS_ASSERT_EQUALS(cow.operator->(), nullptr);
  }

  void testConstructorByPtr() {

    auto *resource = new MyType{2};
    cow_ptr<MyType> cow{resource};

    TSM_ASSERT_EQUALS("COW does not hold the expected value", 2, cow->value);
  }

  void testConstructorByTemporarySptr() {

    int value = 3;
    auto resource = boost::make_shared<MyType>(value);
    cow_ptr<MyType> cow{std::move(resource)};

    TS_ASSERT_EQUALS(cow->value, value);
    TSM_ASSERT("Resource should have been moved", resource.get() == nullptr);
  }

  void testConstructorByNamedSptr() {

    int value = 3;
    auto resource = boost::make_shared<MyType>(value);
    cow_ptr<MyType> cow{resource};

    TS_ASSERT_EQUALS(cow->value, value);
    TSM_ASSERT("Resource should NOT have been moved",
               resource.get() != nullptr);
    TSM_ASSERT_EQUALS("Two shared_ptr objects in scope", resource.use_count(),
                      2)
  }

  void test_move_constructor() {
    auto resource = boost::make_shared<int>(42);
    cow_ptr<int> source{resource};
    cow_ptr<int> clone(std::move(source));
    TS_ASSERT(!source);
    TS_ASSERT_EQUALS(clone.get(), resource.get());
  }

  void test_move_assignment() {
    auto resource = boost::make_shared<int>(42);
    cow_ptr<int> source{resource};
    cow_ptr<int> clone;
    clone = std::move(source);
    TS_ASSERT(!source);
    TS_ASSERT_EQUALS(clone.get(), resource.get());
  }

  void test_copy_assign_nullptr() {
    cow_ptr<MyType> cow1{nullptr};
    TS_ASSERT(!cow1);

    auto cow2 = cow1;
    TS_ASSERT(!cow2);

    auto cow3(cow1);
    TS_ASSERT(!cow3);

    cow_ptr<MyType> cow4;
    TS_ASSERT(cow4);
    cow4 = cow1;
    TS_ASSERT(!cow4);

    boost::shared_ptr<MyType> shared;
    TS_ASSERT(!shared);
    cow4 = shared;
    TS_ASSERT(!cow4);

    MyType *resource = nullptr;
    cow_ptr<MyType> cow5{resource};
    TS_ASSERT(!cow5);
  }

  void test_get() {
    auto resource = boost::make_shared<MyType>(42);
    cow_ptr<MyType> cow(resource);
    TS_ASSERT_DIFFERS(resource.get(), nullptr);
    TS_ASSERT_EQUALS(cow.get(), resource.get());
  }

  void test_operator_bool() {
    cow_ptr<MyType> cow1{nullptr};
    TS_ASSERT(!cow1);
    auto resource = boost::make_shared<MyType>(42);
    cow_ptr<MyType> cow2{resource};
    TS_ASSERT(cow2);
  }

  void test_use_count_and_unique() {
    cow_ptr<MyType> cow{nullptr};
    TS_ASSERT(!cow.unique());
    TS_ASSERT_EQUALS(cow.use_count(), 0);
    cow = boost::make_shared<MyType>(42);
    TS_ASSERT(cow.unique());
    TS_ASSERT_EQUALS(cow.use_count(), 1);
    auto copy = cow;
    TS_ASSERT(!cow.unique());
    TS_ASSERT_EQUALS(cow.use_count(), 2);
    // acessing copy makes a copy of data, so ref count in cow drops
    copy.access();
    TS_ASSERT(cow.unique());
    TS_ASSERT_EQUALS(cow.use_count(), 1);
  }

  void test_access() {

    int value = 3;
    cow_ptr<MyType> original{boost::make_shared<MyType>(value)};
    auto copy = original; // Now internal shared_ptr count should be at 2

    MyType &copyResource = copy.access(); // The resource should now be copied.

    TSM_ASSERT_EQUALS("Value should NOT have changed", original->value,
                      copyResource.value);

    copyResource.value = 4;

    TSM_ASSERT_DIFFERS("Value should now have changed", original->value,
                       copyResource.value);
  }

  void test_equals_not_equals() {
    cow_ptr<MyType> cow{nullptr};
    TS_ASSERT(cow == cow);
    const auto cow2 = boost::make_shared<MyType>(42);
    TS_ASSERT(cow2 == cow2);
    TS_ASSERT(cow != cow2);
    cow = boost::make_shared<MyType>(42);
    TS_ASSERT(cow == cow);
    TS_ASSERT(cow != cow2);
    cow = cow2;
    TS_ASSERT(cow == cow2);
  }
};

#endif /*COW_PTR_TEST_H_*/
