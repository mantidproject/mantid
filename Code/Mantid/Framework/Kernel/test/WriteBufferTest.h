#ifndef MANTID_KERNEL_WRITEBUFFERTEST_H_
#define MANTID_KERNEL_WRITEBUFFERTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidKernel/WriteBuffer.h"

using namespace Mantid;
using namespace Mantid::Kernel;

class WriteBufferTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static WriteBufferTest *createSuite() { return new WriteBufferTest(); }
  static void destroySuite( WriteBufferTest *suite ) { delete suite; }


  void test_Something()
  {
  }


};


#endif /* MANTID_KERNEL_WRITEBUFFERTEST_H_ */
