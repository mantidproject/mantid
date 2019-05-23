// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MRULISTTEST_H_
#define MRULISTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/MRUList.h"
#include "MantidKernel/MultiThreaded.h"

using namespace Mantid::Kernel;

/** Dummy class needed by MRUList */
class MyTestClass {
public:
  size_t hash;
  int value;

  /// Constructor
  MyTestClass(size_t hash, int value) : hash(hash), value(value) {}

  // Function returning the has index value
  size_t hashIndexFunction() const { return hash; }
};

class MRUListTest : public CxxTest::TestSuite {
public:
  void testMRU_Everything() {
    // MRUList with 3 spots
    MRUList<MyTestClass> m(3);
    TS_ASSERT_EQUALS(m.size(), 0);
    m.insert(std::make_shared<MyTestClass>(10, 20));
    TS_ASSERT_EQUALS(m.size(), 1);

    // Retrieve an element
    TS_ASSERT(m.find(10));
    TS_ASSERT_EQUALS(m.find(10)->value, 20);

    auto twenty = std::make_shared<MyTestClass>(20, 40);
    m.insert(twenty);
    TS_ASSERT_EQUALS(m.size(), 2);
    m.insert(std::make_shared<MyTestClass>(30, 60));
    TS_ASSERT_EQUALS(m.size(), 3);

    // This will drop one from the list
    std::shared_ptr<MyTestClass> being_dropped = m.insert(std::make_shared<MyTestClass>(40, 80));
    TS_ASSERT_EQUALS(m.size(), 3);
    // # 10 was dropped off; the calling class takes care of whatever this means
    TS_ASSERT_EQUALS(being_dropped->hash, 10);

    // Can't find the one that dropped off
    TS_ASSERT(!m.find(10));

    // But we can find the one that was last in line
    TS_ASSERT_EQUALS(m.find(20), twenty.get());
    // We can add it again, pushing it to the top
    being_dropped = m.insert(twenty);
    TS_ASSERT_EQUALS(m.size(), 3);
    // Nothing needs to be dropped.
    TS_ASSERT(!being_dropped);

    // And we can add 2 new ones
    m.insert(std::make_shared<MyTestClass>(50, 100));
    TS_ASSERT_EQUALS(m.size(), 3);
    m.insert(std::make_shared<MyTestClass>(60, 120));
    TS_ASSERT_EQUALS(m.size(), 3);

    // And now the ones left are 20 (since it was moved to the top of the MRU
    // list) and the 2 new ones.
    TS_ASSERT(m.find(20));
    TS_ASSERT(m.find(50));
    TS_ASSERT(m.find(60));

    TS_ASSERT_THROWS_NOTHING(m.deleteIndex(50));
    TS_ASSERT_EQUALS(m.size(), 2);

    // Test out the clear method.
    m.clear();
    TS_ASSERT_EQUALS(m.size(), 0);
  }

  /** Access the MRU list in parallel */
  void test_threadSafety() {
    MRUList<MyTestClass> m(100);
    PRAGMA_OMP( parallel for )
    for (int i = 0; i < 1000; i++) {
      auto dropped = m.insert(std::make_shared<MyTestClass>(size_t(i), i));
      m.find(size_t(i));
    }
    TS_ASSERT_EQUALS(m.size(), 100);
  }
};

#endif /* MRULISTTEST_H_ */
