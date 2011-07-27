#ifndef MANTID_DATAOBJECTS_MANAGEDHISTOGRAM1DTEST_H_
#define MANTID_DATAOBJECTS_MANAGEDHISTOGRAM1DTEST_H_

#include "MantidDataObjects/ManagedHistogram1D.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::API;

class ManagedHistogram1DTest : public CxxTest::TestSuite
{
public:

  void test_constructor()
  {
    ManagedHistogram1D h(NULL, 1234);
    TS_ASSERT_EQUALS( h.getWorkspaceIndex(), 1234);
  }

  /** Const access does not set the dirty flag */
  void test_dirtyFlag_const()
  {
    const ManagedHistogram1D h(NULL, 1234);
    TS_ASSERT( !h.isDirty() );
    const MantidVec & x = h.dataX();
    TS_ASSERT( !h.isDirty() );
    const MantidVec & y = h.dataY();
    TS_ASSERT( !h.isDirty() );
    const MantidVec & e = h.dataE();
    TS_ASSERT( !h.isDirty() );
    const MantidVec & dx = h.dataDx();
    TS_ASSERT( !h.isDirty() );
    UNUSED_ARG(x);UNUSED_ARG(dx);UNUSED_ARG(y);UNUSED_ARG(e);
  }


  /** Non-const access does set the dirty flag (except for Dx)*/
  void test_dirtyFlag_isSet()
  {
    {
      ManagedHistogram1D h(NULL, 1234);
      TS_ASSERT( !h.isDirty() );
      h.dataX();
      TS_ASSERT( h.isDirty() );
      ManagedHistogram1D * h2 = new ManagedHistogram1D(NULL, 1234);
      // Check that the method is properly overridden in ISpectrum
      ISpectrum * spec = h2;
      TS_ASSERT( !h2->isDirty() );
      spec->dataX();
      TS_ASSERT( h2->isDirty() );
    }
    {
      ManagedHistogram1D h(NULL, 1234);
      TS_ASSERT( !h.isDirty() );
      h.ptrX();
      TS_ASSERT( h.isDirty() );
    }
    {
      ManagedHistogram1D h(NULL, 1234);
      TS_ASSERT( !h.isDirty() );
      h.dataY();
      TS_ASSERT( h.isDirty() );
    }
    {
      ManagedHistogram1D h(NULL, 1234);
      TS_ASSERT( !h.isDirty() );
      h.dataE();
      TS_ASSERT( h.isDirty() );
    }
    {
      ManagedHistogram1D h(NULL, 1234);
      TS_ASSERT( !h.isDirty() );
      h.dataDx();
      // Dx does NOT dirty it
      TS_ASSERT( !h.isDirty() );
    }
  }

  /** setX or setData() makes it dirty */
  void test_dirtyFlag_isSet_whenUsingSetData()
  {
    MantidVec X, Y, E;
    {
      ManagedHistogram1D h(NULL, 1234);
      TS_ASSERT( !h.isDirty() );
      h.setData(Y);
      TS_ASSERT( h.isDirty() );
    }
    {
      ManagedHistogram1D h(NULL, 1234);
      TS_ASSERT( !h.isDirty() );
      h.setData(Y, E);
      TS_ASSERT( h.isDirty() );
    }
    {
      ManagedHistogram1D h(NULL, 1234);
      TS_ASSERT( !h.isDirty() );
      h.setX(X);
      TS_ASSERT( h.isDirty() );
    }
    {
      ManagedHistogram1D h(NULL, 1234);
      TS_ASSERT( !h.isDirty() );
      h.setDx(X);
      // Dx does NOT dirty it
      TS_ASSERT( !h.isDirty() );
    }
  }


};


#endif /* MANTID_DATAOBJECTS_MANAGEDHISTOGRAM1DTEST_H_ */

