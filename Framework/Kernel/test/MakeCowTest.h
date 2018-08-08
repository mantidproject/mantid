#ifndef MAKE_COW_TEST_H_
#define MAKE_COW_TEST_H_

#include "MantidKernel/make_cow.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::Kernel;

namespace {

template <typename T> class MyType {
public:
  MyType() : args(0) {}
  MyType(const T &) : args(1) {}
  MyType(const T &, const T &) : args(2) {}
  const int args;
};
} // namespace

class MakeCowTest : public CxxTest::TestSuite {
public:
  void testDefaultConstruction() {

    auto product = make_cow<MyType<int>>();
    TSM_ASSERT_EQUALS("We expect default constructor to be called",
                      product->args, 0);
  }

  void testConstructWithOneArgument() {
    auto product = make_cow<MyType<int>>(7);
    TSM_ASSERT_EQUALS("We expect one arg constructor to be called",
                      product->args, 1);
  }

  void testConstructWithTwoArgument() {
    auto product = make_cow<MyType<int>>(7, 7);
    TSM_ASSERT_EQUALS("We expect two arg constructor to be called",
                      product->args, 2);
  }
};

#endif
