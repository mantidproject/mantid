#ifndef MANTID_ALGORITHMS_JOINWORKSPACESTEST_H_
#define MANTID_ALGORITHMS_JOINWORKSPACESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/JoinWorkspaces.h"

using Mantid::Algorithms::JoinWorkspaces;

class JoinWorkspacesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static JoinWorkspacesTest *createSuite() { return new JoinWorkspacesTest(); }
  static void destroySuite( JoinWorkspacesTest *suite ) { delete suite; }

  void test_Init()
  {
    JoinWorkspaces alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

};


#endif /* MANTID_ALGORITHMS_JOINWORKSPACESTEST_H_ */
