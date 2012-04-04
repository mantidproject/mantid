#ifndef MANTID_API_DATAPROCESSORALGORITHMTEST_H_
#define MANTID_API_DATAPROCESSORALGORITHMTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidAPI/DataProcessorAlgorithm.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::API;

class DataProcessorAlgorithmTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DataProcessorAlgorithmTest *createSuite() { return new DataProcessorAlgorithmTest(); }
  static void destroySuite( DataProcessorAlgorithmTest *suite ) { delete suite; }


  void test_Something()
  {
  }


};


#endif /* MANTID_API_DATAPROCESSORALGORITHMTEST_H_ */