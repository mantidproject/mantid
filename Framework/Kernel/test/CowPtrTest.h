#ifndef COW_PTR_TEST_H_
#define COW_PTR_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/cow_ptr.h"
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

using namespace Mantid::Kernel;

namespace {

struct MyType {
public:
  MyType() : value(0) {}
  MyType(int val) : value(val) {}
  int value;
};
}

class CowPtrTest : public CxxTest::TestSuite {
public:
  void testDefaultConstruct() {

    cow_ptr<MyType> cow{};
    TSM_ASSERT_EQUALS("Should give us the default of the default constructed T",
                      0, cow->value);
  }

  void testConstructorByPtr() {

    MyType *nullMyType = nullptr;
    TSM_ASSERT_THROWS("Source cannot be null", cow_ptr<MyType>{nullMyType},
                      std::invalid_argument &);

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
};

#endif /*COW_PTR_TEST_H_*/
