#ifndef MANTID_ALGORITHMS_CREATETRANSMISSIONWORKSPACETEST_H_
#define MANTID_ALGORITHMS_CREATETRANSMISSIONWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CreateTransmissionWorkspace.h"

using Mantid::Algorithms::CreateTransmissionWorkspace;

class CreateTransmissionWorkspaceTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CreateTransmissionWorkspaceTest *createSuite() { return new CreateTransmissionWorkspaceTest(); }
  static void destroySuite( CreateTransmissionWorkspaceTest *suite ) { delete suite; }


  void test_Init()
  {
    CreateTransmissionWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {
  }
  
  void test_Something()
  {
  }


};


#endif /* MANTID_ALGORITHMS_CREATETRANSMISSIONWORKSPACETEST_H_ */
