#ifndef MANTID_ALGORITHMS_MERGERUNS_SAMPLELOGSBEHAVIOURTEST_H_
#define MANTID_ALGORITHMS_MERGERUNS_SAMPLELOGSBEHAVIOURTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/MergeRuns/SampleLogsBehaviour.h"

using Mantid::Algorithms::SampleLogsBehaviour;

class MergeRunsSampleLogsBehaviourTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MergeRunsSampleLogsBehaviourTest *createSuite() { return new MergeRunsSampleLogsBehaviourTest(); }
  static void destroySuite( MergeRunsSampleLogsBehaviourTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_ALGORITHMS_MERGERUNS_SAMPLELOGSBEHAVIOURTEST_H_ */