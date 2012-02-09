#ifndef MANTID_API_MATRIXWORKSPACEMDITERATORTEST_H_
#define MANTID_API_MATRIXWORKSPACEMDITERATORTEST_H_

#include "MantidAPI/MatrixWorkspaceMDIterator.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidTestHelpers/FakeObjects.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;

class MatrixWorkspaceMDIteratorTest : public CxxTest::TestSuite
{
public:

  void test_iterating()
  {
    boost::shared_ptr<MatrixWorkspace> ws(new WorkspaceTester());
    // Matrix with 4 spectra, 5 bins each
    ws->initialize(4,6,5);
    for (size_t wi=0; wi<4; wi++)
      for (size_t x=0; x<6; x++)
      {
        ws->dataX(wi)[x] = double(x);
        if (x<5)
        {
          ws->dataY(wi)[x] = double(wi*10 + x);
          ws->dataE(wi)[x] = double((wi*10 + x)*2);
        }
      }
    IMDIterator * it;
    TS_ASSERT_THROWS_NOTHING( it = ws->createIterator(NULL) );
    TS_ASSERT_EQUALS( it->getDataSize(), 20);
    TS_ASSERT_DELTA( it->getSignal(), 0.0, 1e-5);
    it->next();
    TS_ASSERT_DELTA( it->getSignal(), 1.0, 1e-5);
    TS_ASSERT_DELTA( it->getError(), 2.0, 1e-5);
    it->next();
    it->next();
    it->next();
    TS_ASSERT_DELTA( it->getSignal(), 4.0, 1e-5);
    TS_ASSERT_DELTA( it->getError(), 8.0, 1e-5);
    it->next();
    it->next();
    // Workspace index 1, x index 1
    TS_ASSERT_DELTA( it->getSignal(), 11.0, 1e-5);
    TS_ASSERT_DELTA( it->getError(), 22.0, 1e-5);
    TS_ASSERT_DELTA( it->getCenter()[0], 1.5, 1e-5);
    TS_ASSERT_DELTA( it->getCenter()[1], 1.0, 1e-5);

  }


};


#endif /* MANTID_API_MATRIXWORKSPACEMDITERATORTEST_H_ */
