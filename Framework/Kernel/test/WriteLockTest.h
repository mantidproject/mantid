#ifndef MANTID_KERNEL_WRITELOCKTEST_H_
#define MANTID_KERNEL_WRITELOCKTEST_H_

#include "MantidKernel/WriteLock.h"

#include "MantidKernel/DataItem.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Kernel;

class MockDataItem : public DataItem {
public:
  const std::string id() const override { return "MockDataItem"; }
  /// The name of the object
  const std::string &getName() const override { return m_name; }
  /// Can this object be accessed from multiple threads safely
  bool threadSafe() const override { return true; }
  /// Serializes the object to a string
  const std::string toString() const override { return "Nothing"; }
  friend class WriteLockTest;

private:
  std::string m_name{"Noone"};
};

class WriteLockTest : public CxxTest::TestSuite {
public:
  void test_Scoped_WriteLock() {
    MockDataItem item;
    WriteLock lock(item);
    // Can't directly check the underlying lock object as it is private...
  }

  void test_new_doesNotCompile() {
    MockDataItem item;
    // The next line does not compile, which is what we want!
    //    WriteLock * lock = new WriteLock(item);
  }
};

#endif /* MANTID_KERNEL_READLOCKTEST_H_ */
