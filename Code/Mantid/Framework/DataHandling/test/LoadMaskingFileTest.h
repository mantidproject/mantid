#ifndef MANTID_DATAHANDLING_LOADMASKINGFILETEST_H_
#define MANTID_DATAHANDLING_LOADMASKINGFILETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidDataHandling/LoadMaskingFile.h"

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::API;

class LoadMaskingFileTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadMaskingFileTest *createSuite() { return new LoadMaskingFileTest(); }
  static void destroySuite( LoadMaskingFileTest *suite ) { delete suite; }


  void test_Something()
  {
  }


};


#endif /* MANTID_DATAHANDLING_LOADMASKINGFILETEST_H_ */

