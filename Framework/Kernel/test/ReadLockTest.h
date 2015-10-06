#ifndef MANTID_KERNEL_READLOCKTEST_H_
#define MANTID_KERNEL_READLOCKTEST_H_

#include "MantidKernel/ReadLock.h"

#include "MantidKernel/DataItem.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Kernel;

class MockDataItem : public DataItem {
public:
  virtual const std::string id() const { return "MockDataItem"; }
  /// The name of the object
  virtual const std::string name() const { return "Noone"; }
  /// Can this object be accessed from multiple threads safely
  virtual bool threadSafe() const { return true; }
  /// Serializes the object to a string
  virtual const std::string toString() const { return "Nothing"; }
  friend class ReadLockTest;
};

class ReadLockTest : public CxxTest::TestSuite {
public:
  void test_Scoped_ReadLock() {
    MockDataItem item;
    ReadLock lock(item);
    // Can't directly check the underlying lock object as it is private...
  }

  void test_new_doesNotCompile() {
    MockDataItem item;
    // The next line does not compile, which is what we want!
    // ReadLock * lock = new ReadLock(item);
  }
};

#endif /* MANTID_KERNEL_READLOCKTEST_H_ */
