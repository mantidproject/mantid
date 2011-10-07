#ifndef MANTID_MDEVENTS_MDHISTOWORKSPACEITERATORTEST_H_
#define MANTID_MDEVENTS_MDHISTOWORKSPACEITERATORTEST_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidMDEvents/MDHistoWorkspaceIterator.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>
#include "MantidKernel/VMD.h"

using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace Mantid::API;
using Mantid::Kernel::VMD;

class MDHistoWorkspaceIteratorTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MDHistoWorkspaceIteratorTest *createSuite() { return new MDHistoWorkspaceIteratorTest(); }
  static void destroySuite( MDHistoWorkspaceIteratorTest *suite ) { delete suite; }

  void test_bad_constructor()
  {
    MDHistoWorkspace_sptr ws;
    TS_ASSERT_THROWS_ANYTHING( MDHistoWorkspaceIterator it(ws));
  }

  void do_test_iterator(size_t nd, size_t numPoints)
  {
    MDHistoWorkspace_sptr ws = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, nd, 10);
    for (size_t i=0; i<numPoints; i++)
      ws->setSignalAt(i, double(i));
    MDHistoWorkspaceIterator * it = new MDHistoWorkspaceIterator(ws);
    size_t i=0;

    // Position of the first box
    for (size_t d=0; d<nd; d++)
    { TS_ASSERT_DELTA( it->getInnerPosition(0,0), 0.5, 1e-6 ); }

    do
    {
      TS_ASSERT_DELTA( it->getNormalizedSignal(), double(i) / 1.0, 1e-5);
      TS_ASSERT_DELTA( it->getNormalizedError(), 1.0, 1e-5);
      coord_t * vertexes;
      size_t numVertices;
      vertexes = it->getVertexesArray(numVertices);
      TS_ASSERT( vertexes );
      TS_ASSERT_EQUALS( it->getNumEvents(), 1 );
      TS_ASSERT_EQUALS( it->getInnerDetectorID(0), 0 );
      TS_ASSERT_EQUALS( it->getInnerRunIndex(0), 0 );
      TS_ASSERT_EQUALS( it->getInnerSignal(0), double(i) );
      TS_ASSERT_EQUALS( it->getInnerError(0),  1.0 );
      i++;
    } while(it->next());
    TS_ASSERT_EQUALS( i, numPoints );

    // Now use a for loop
    for (size_t i=0; i < numPoints; i++)
    {
      it->jumpTo(i);
      TS_ASSERT_DELTA( it->getNormalizedSignal(), double(i) / 1.0, 1e-5);
    }
  }

  void test_iterator_1D()
  {
    do_test_iterator(1, 10);
  }

  void test_iterator_2D()
  {
    do_test_iterator(2, 100);
  }

  void test_iterator_3D()
  {
    do_test_iterator(3, 1000);
  }

  void test_iterator_4D()
  {
    do_test_iterator(4, 10000);
  }


};



class MDHistoWorkspaceIteratorTestPerformance : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MDHistoWorkspaceIteratorTestPerformance *createSuite() { return new MDHistoWorkspaceIteratorTestPerformance(); }
  static void destroySuite( MDHistoWorkspaceIteratorTestPerformance *suite ) { delete suite; }

  MDHistoWorkspace_sptr ws;

  MDHistoWorkspaceIteratorTestPerformance()
  {
    // 125^3 workspace = about 2 million
    ws = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 3, 125);
  }

  /** ~Two million iterations */
  void test_iterator_3D_signalAndErrorOnly()
  {
    MDHistoWorkspaceIterator * it = new MDHistoWorkspaceIterator(ws);
    do
    {
      signal_t sig = it->getNormalizedSignal();
      signal_t err = it->getNormalizedError();
      UNUSED_ARG(sig); UNUSED_ARG(err);
    } while(it->next());
  }

  /** ~Two million iterations */
  void test_iterator_3D_withGetVertexes()
  {
    MDHistoWorkspaceIterator * it = new MDHistoWorkspaceIterator(ws);
    size_t numVertices;
    do
    {
      signal_t sig = it->getNormalizedSignal();
      signal_t err = it->getNormalizedError();
      coord_t * vertexes = it->getVertexesArray(numVertices);
      delete [] vertexes;
      UNUSED_ARG(sig); UNUSED_ARG(err);
    } while(it->next());
  }

  /** ~Two million iterations */
  void test_iterator_3D_withGetCenter()
  {
    MDHistoWorkspaceIterator * it = new MDHistoWorkspaceIterator(ws);
    do
    {
      signal_t sig = it->getNormalizedSignal();
      signal_t err = it->getNormalizedError();
      VMD center = it->getCenter();
      UNUSED_ARG(sig); UNUSED_ARG(err);
    } while(it->next());
  }

  /** Use jumpTo() */
  void test_iterator_3D_withGetCenter_usingJumpTo()
  {
    MDHistoWorkspaceIterator * it = new MDHistoWorkspaceIterator(ws);
    int max = int(it->getDataSize());
    for (int i=0; i<max; i++)
    {
      it->jumpTo(size_t(i));
      signal_t sig = it->getNormalizedSignal();
      signal_t err = it->getNormalizedError();
      VMD center = it->getCenter();
      UNUSED_ARG(sig); UNUSED_ARG(err);
    }
  }

};


#endif /* MANTID_MDEVENTS_MDHISTOWORKSPACEITERATORTEST_H_ */

