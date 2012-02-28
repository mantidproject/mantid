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

  boost::shared_ptr<MatrixWorkspace> makeFakeWS()
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
    return ws;
  }

  void test_iterating()
  {
    boost::shared_ptr<MatrixWorkspace> ws = makeFakeWS();
    IMDIterator * it = NULL;
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


  /** Create a set of iterators that can be applied in parallel */
  void test_parallel_iterators()
  {
    boost::shared_ptr<MatrixWorkspace> ws = makeFakeWS();
    // The number of output cannot be larger than the number of histograms
    TS_ASSERT_EQUALS( ws->createIterators(10, NULL).size(), 4);

    // Split in 4 iterators
    std::vector<IMDIterator*> iterators = ws->createIterators(4, NULL);
    TS_ASSERT_EQUALS( iterators.size(), 4 );

    for (size_t i=0; i<iterators.size(); i++)
    {
      IMDIterator * it = iterators[i];
      // Only 5 elements per each iterator
      TS_ASSERT_EQUALS( it->getDataSize(), 5);
      TS_ASSERT_DELTA( it->getSignal(), double(i)*10 + 0.0, 1e-5);
      it->next();
      TS_ASSERT_DELTA( it->getSignal(), double(i)*10 + 1.0, 1e-5);
      TS_ASSERT_DELTA( it->getError(), double(i)*20 + 2.0, 1e-5);
      // Coordinates at X index = 1
      TS_ASSERT_DELTA( it->getCenter()[0], 1.5, 1e-5);
      // And this coordinate is the spectrum number
      TS_ASSERT_DELTA( it->getCenter()[1], double(i), 1e-5);
      TS_ASSERT( it->next() );
      TS_ASSERT( it->next() );
      TS_ASSERT( it->next() );
      TS_ASSERT( !it->next() );
    }
  }

  void test_get_is_masked()
  {
    boost::shared_ptr<MatrixWorkspace> ws = makeFakeWS();
    std::vector<IMDIterator*> iterators = ws->createIterators(1, NULL);

    //Characterisation test. Lock-down current behaviour.
    TS_ASSERT_THROWS(iterators[0]->getIsMasked(), std::runtime_error);
  }


};


#endif /* MANTID_API_MATRIXWORKSPACEMDITERATORTEST_H_ */
