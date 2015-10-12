#ifndef MANTID_API_MATRIXWORKSPACEVALIDATORTEST_H_
#define MANTID_API_MATRIXWORKSPACEVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/MatrixWorkspaceValidator.h"

using Mantid::API::MatrixWorkspaceValidator;

class MatrixWorkspaceValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MatrixWorkspaceValidatorTest *createSuite() { return new MatrixWorkspaceValidatorTest(); }
  static void destroySuite( MatrixWorkspaceValidatorTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_API_MATRIXWORKSPACEVALIDATORTEST_H_ */