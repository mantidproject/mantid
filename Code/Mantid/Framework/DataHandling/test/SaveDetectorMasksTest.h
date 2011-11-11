#ifndef MANTID_DATAHANDLING_SAVEMASKINGTOFILETEST_H_
#define MANTID_DATAHANDLING_SAVEMASKINGTOFILETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidDataHandling/SaveDetectorMasks.h"

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::API;

class SaveDetectorMasksTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveDetectorMasks *createSuite() { return new SaveDetectorMasksTest(); }

  static void destroySuite( SaveMaskingToFileTest *suite ) { delete suite; }

  void test_Something()
  {
  }


};


#endif /* MANTID_DATAHANDLING_SAVEMASKINGTOFILETEST_H_ */
