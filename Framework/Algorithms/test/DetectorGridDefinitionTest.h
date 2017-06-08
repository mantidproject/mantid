#ifndef MANTID_ALGORITHMS_DETECTORGRIDDEFINITIONTEST_H_
#define MANTID_ALGORITHMS_DETECTORGRIDDEFINITIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/SampleCorrections/DetectorGridDefinition.h"

using Mantid::Algorithms::DetectorGridDefinition;

class DetectorGridDefinitionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DetectorGridDefinitionTest *createSuite() { return new DetectorGridDefinitionTest(); }
  static void destroySuite( DetectorGridDefinitionTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_ALGORITHMS_DETECTORGRIDDEFINITIONTEST_H_ */