#ifndef MANTID_CUSTOMINTERFACES_DATAPROCESSORALGORITHMBASETEST_H_
#define MANTID_CUSTOMINTERFACES_DATAPROCESSORALGORITHMBASETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidQtCustomInterfaces/Reflectometry/DataProcessorAlgorithmBase.h"

using MantidQt::CustomInterfaces::DataProcessorAlgorithmBase;

class DataProcessorAlgorithmBaseTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DataProcessorAlgorithmBaseTest *createSuite() { return new DataProcessorAlgorithmBaseTest(); }
  static void destroySuite( DataProcessorAlgorithmBaseTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_CUSTOMINTERFACES_DATAPROCESSORALGORITHMBASETEST_H_ */