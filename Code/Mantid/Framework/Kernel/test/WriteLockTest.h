#ifndef MANTID_KERNEL_WRITELOCKTEST_H_
#define MANTID_KERNEL_WRITELOCKTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidKernel/WriteLock.h"
#include "MantidKernel/ReadLock.h"

using namespace Mantid;
using namespace Mantid::Kernel;

class MockDataItem : public DataItem
{
public:
  virtual const std::string id() const { return "MockDataItem"; }
  /// The name of the object
  virtual const std::string name() const{ return "Noone"; }
  /// Can this object be accessed from multiple threads safely
  virtual bool threadSafe() const { return true; }
  /// Serializes the object to a string
  virtual const std::string toString() const { return "Nothing"; }
  friend class WriteLockTest;
};



class WriteLockTest : public CxxTest::TestSuite
{
public:

  void test_Scoped_WriteLock()
  {
    MockDataItem item;
    WriteLock lock(item);
    // Can't directly check the underlying lock object as it is private...
  }

  void test_new_doesNotCompile()
  {
    MockDataItem item;
    // The next line does not compile, which is what we want!
//    WriteLock * lock = new WriteLock(item);
  }

};


#endif /* MANTID_KERNEL_READLOCKTEST_H_ */
