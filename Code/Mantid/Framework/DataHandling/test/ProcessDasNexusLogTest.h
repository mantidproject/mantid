#ifndef MANTID_DATAHANDLING_PROCESSDASNEXUSLOGTEST_H_
#define MANTID_DATAHANDLING_PROCESSDASNEXUSLOGTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidDataHandling/ProcessDasNexusLog.h"

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::API;

class ProcessDasNexusLogTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ProcessDasNexusLogTest *createSuite() { return new ProcessDasNexusLogTest(); }
  static void destroySuite( ProcessDasNexusLogTest *suite ) { delete suite; }


  void test_Something()
  {
  }


};


#endif /* MANTID_DATAHANDLING_PROCESSDASNEXUSLOGTEST_H_ */