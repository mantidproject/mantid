#ifndef MANTID_API_WORKSPACEUNITVALIDATORTEST_H_
#define MANTID_API_WORKSPACEUNITVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/WorkspaceUnitValidator.h"

using Mantid::API::WorkspaceUnitValidator;

class WorkspaceUnitValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static WorkspaceUnitValidatorTest *createSuite() { return new WorkspaceUnitValidatorTest(); }
  static void destroySuite( WorkspaceUnitValidatorTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_API_WORKSPACEUNITVALIDATORTEST_H_ */