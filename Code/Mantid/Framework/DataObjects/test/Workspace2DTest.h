#ifndef WORKSPACE2DTEST_H_
#define WORKSPACE2DTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

using namespace std;
using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using Mantid::MantidVec;

class Workspace2DTest : public CxxTest::TestSuite
{
public:
  int nbins, nhist;
  Workspace2D_sptr ws;

  Workspace2DTest()
  {
    nbins = 5;
    nhist = 10;
    ws = Create2DWorkspaceBinned(nhist, nbins);
  }

  static Workspace2D_sptr Create2DWorkspaceBinned(int nhist, int nbins, double x0=0.0, double deltax = 1.0)
  {
    MantidVecPtr x,y,e;
    x.access().resize(nbins+1);
    y.access().resize(nbins,2); // Value of 2.0 in all ys
    e.access().resize(nbins,sqrt(2.0));
    for (int i =0; i < nbins+1; ++i)
    {
      x.access()[i] = x0+i*deltax;
    }
    Workspace2D_sptr retVal(new Workspace2D());
    retVal->initialize(nhist,nbins+1,nbins);
    for (int i=0; i< nhist; i++)
    {
      retVal->setX(i,x);
      retVal->setData(i,y,e);
    }

    return retVal;
  }


  void testInit()
  {
    ws->setTitle("testInit");
    TS_ASSERT_EQUALS( ws->getNumberHistograms(), nhist );;
    TS_ASSERT_EQUALS( ws->blocksize(), nbins );;
    TS_ASSERT_EQUALS( ws->size(), nbins*nhist );;

    for (int i = 0; i < nhist; ++i)
    {
      TS_ASSERT_EQUALS( ws->dataX(i).size(), nbins+1 );;
      TS_ASSERT_EQUALS( ws->dataY(i).size(), nbins );;
      TS_ASSERT_EQUALS( ws->dataE(i).size(), nbins );;
    }
  }

  void testId()
  {
    TS_ASSERT_EQUALS( ws->id(), "Workspace2D" );
  }

  void testSetX()
  {
    double aNumber = 5.3;
    boost::shared_ptr<MantidVec > v(new MantidVec(nbins, aNumber));
    TS_ASSERT_THROWS_NOTHING( ws->setX(0,v) );
    TS_ASSERT_EQUALS( ws->dataX(0)[0], aNumber );
    TS_ASSERT_THROWS( ws->setX(-1,v), std::range_error );
    TS_ASSERT_THROWS( ws->setX(nhist+5,v), std::range_error );
  }

  void testSetX_cowptr()
  {
    double aNumber = 5.4;
    MantidVecPtr v;
    v.access() = MantidVec(nbins, aNumber);
    TS_ASSERT_THROWS_NOTHING( ws->setX(0,v) );
    TS_ASSERT_EQUALS( ws->dataX(0)[0], aNumber );
    TS_ASSERT_THROWS( ws->setX(-1,v), std::range_error );
    TS_ASSERT_THROWS( ws->setX(nhist+5,v), std::range_error );
  }

  void testSetData_cowptr()
  {
    double aNumber = 5.5;
    MantidVecPtr v;
    v.access() = MantidVec(nbins, aNumber);
    TS_ASSERT_THROWS_NOTHING( ws->setData(0,v) );
    TS_ASSERT_EQUALS( ws->dataY(0)[0], aNumber );
    TS_ASSERT_DIFFERS( ws->dataY(1)[0], aNumber );
  }

  void testSetData_cowptr2()
  {
    double aNumber = 5.6;
    MantidVecPtr v,e;
    v.access() = MantidVec(nbins, aNumber);
    e.access() = MantidVec(nbins, aNumber*2);
    TS_ASSERT_THROWS_NOTHING( ws->setData(0,v,e) );
    TS_ASSERT_EQUALS( ws->dataY(0)[0], aNumber );
    TS_ASSERT_EQUALS( ws->dataE(0)[0], aNumber*2 );
    TS_ASSERT_DIFFERS( ws->dataY(1)[0], aNumber );
    TS_ASSERT_DIFFERS( ws->dataE(1)[0], aNumber*2 );
  }

  void testSetData()
  {
    double aNumber = 5.7;
    const boost::shared_ptr<MantidVec > v(new MantidVec(nbins, aNumber));
    const boost::shared_ptr<MantidVec > e(new MantidVec(nbins, aNumber*2));
    TS_ASSERT_THROWS_NOTHING( ws->setData(0,v,e) );
    TS_ASSERT_EQUALS( ws->dataY(0)[0], aNumber );
    TS_ASSERT_EQUALS( ws->dataE(0)[0], aNumber*2 );
    TS_ASSERT_DIFFERS( ws->dataY(1)[0], aNumber );
    TS_ASSERT_DIFFERS( ws->dataE(1)[0], aNumber*2 );
  }

  void testIntegrateSpectra_entire_range()
  {
    ws = Create2DWorkspaceBinned(nhist, nbins);
    MantidVec sums;
    ws->getIntegratedSpectra(sums, 10, 5, true);
    for (int i = 0; i < nhist; ++i)
    {
      TS_ASSERT_EQUALS( sums[i], nbins * 2.0 );;
    }
  }
  void testIntegrateSpectra_empty_range()
  {
    ws = Create2DWorkspaceBinned(nhist, nbins);
    MantidVec sums;
    ws->getIntegratedSpectra(sums, 10, 5, false);
    for (int i = 0; i < nhist; ++i)
    {
      TS_ASSERT_EQUALS( sums[i], 0.0 );;
    }
  }

  void testIntegrateSpectra_partial_range()
  {
    ws = Create2DWorkspaceBinned(nhist, nbins);
    MantidVec sums;
    ws->getIntegratedSpectra(sums, 1.9, 3.2, false);
    for (int i = 0; i < nhist; ++i)
    {
      TS_ASSERT_EQUALS( sums[i], 4.0 );;
    }
  }

  void testReadYE()
  {
    ws = Create2DWorkspaceBinned(nhist, nbins);
    MantidVec const * Y=NULL;
    MantidVec const * E=NULL;
    ws->readYE(0, Y, E);
    TS_ASSERT( Y );
    TS_ASSERT( E );
    TS_ASSERT_EQUALS( (*Y)[0], 2.0 );
    TS_ASSERT_EQUALS( (*E)[0], sqrt(2.0) );
  }

  void test_getMemorySizeForXAxes()
  {
    ws = Create2DWorkspaceBinned(nhist, nbins);
    // Here they are shared, so only 1 X axis
    TS_ASSERT_EQUALS( ws->getMemorySizeForXAxes(), 1*(nbins+1)*sizeof(double));
    for (int i=0; i < nhist; i++)
    {
      ws->dataX(i)[0] += 1; // This modifies the X axis in-place, creatign a copy of it.
    }
    // Now there is a different one for each
    TS_ASSERT_EQUALS( ws->getMemorySizeForXAxes(), nhist*(nbins+1)*sizeof(double));
  }

};

#endif
