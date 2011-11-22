#ifndef MANTID_MDEVENTS_BINTOMDHISTOWORKSPACETEST_H_
#define MANTID_MDEVENTS_BINTOMDHISTOWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidMDEvents/BinToMDHistoWorkspace.h"

using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace Mantid::API;

class BinToMDHistoWorkspaceTest : public CxxTest::TestSuite
{
public:
  
  void test_deprecated()
  {
  }
};


#endif /* MANTID_MDEVENTS_BINTOMDHISTOWORKSPACETEST_H_ */
