#ifndef MANTID_DATAHANDLING_SAVEDETECTORSGROUPINGTEST_H_
#define MANTID_DATAHANDLING_SAVEDETECTORSGROUPINGTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidDataHandling/SaveDetectorsGrouping.h"

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::API;

class SaveDetectorsGroupingTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveDetectorsGroupingTest *createSuite() { return new SaveDetectorsGroupingTest(); }
  static void destroySuite( SaveDetectorsGroupingTest *suite ) { delete suite; }


  void test_Something()
  {
  }


};


#endif /* MANTID_DATAHANDLING_SAVEDETECTORSGROUPINGTEST_H_ */