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

class cow_ptr_test : public CxxTest::TestSuite {
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

  void testConstructorBySptr() {

    int value = 3;
    auto resource = boost::make_shared<MyType>(value);
    cow_ptr<MyType> cow{std::move(resource)}; // via lhr

    TS_ASSERT_EQUALS(cow->value, value);
    TSM_ASSERT("Resource should have been moved", resource.get() == nullptr);
  }
};

#endif /*COW_PTR_TEST_H_*/
