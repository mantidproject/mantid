#ifndef MANTID_KERNEL_IFILETEST_H_
#define MANTID_KERNEL_IFILETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidKernel/IFile.h"

using namespace Mantid;
using namespace Mantid::Kernel;

class IFileTest : public CxxTest::TestSuite
{
public:

  void test_Something()
  {
    /// Pure abstract class, nothing to test.
  }


};


#endif /* MANTID_KERNEL_IFILETEST_H_ */

