#ifndef MANTID_DATAOBJECTS_WORKSPACECREATIONTEST_H_
#define MANTID_DATAOBJECTS_WORKSPACECREATIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/WorkspaceCreation.h"

class WorkspaceCreationTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static WorkspaceCreationTest *createSuite() { return new WorkspaceCreationTest(); }
  static void destroySuite( WorkspaceCreationTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_DATAOBJECTS_WORKSPACECREATIONTEST_H_ */
