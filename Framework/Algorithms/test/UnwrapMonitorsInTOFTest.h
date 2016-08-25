#ifndef MANTID_ALGORITHMS_UNWRAPMONITORSINTOFTEST_H_
#define MANTID_ALGORITHMS_UNWRAPMONITORSINTOFTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/UnwrapMonitorsInTOF.h"

using Mantid::Algorithms::UnwrapMonitorsInTOF;

class UnwrapMonitorsInTOFTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static UnwrapMonitorsInTOFTest *createSuite() { return new UnwrapMonitorsInTOFTest(); }
  static void destroySuite( UnwrapMonitorsInTOFTest *suite ) { delete suite; }


  void test_Init()
  {
    UnwrapMonitorsInTOF alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_exec()
  {
    // Create test input if necessary
    MatrixWorkspace_sptr inputWS = //-- Fill in appropriate code. Consider using TestHelpers/WorkspaceCreationHelpers.h --

    UnwrapMonitorsInTOF alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", inputWS) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", "_unused_for_child") );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    // Retrieve the workspace from the algorithm. The type here will probably need to change. It should
    // be the type using in declareProperty for the "OutputWorkspace" type.
    // We can't use auto as it's an implicit conversion.
    Workspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    TS_FAIL("TODO: Check the results and remove this line");
  }
  
  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_ALGORITHMS_UNWRAPMONITORSINTOFTEST_H_ */