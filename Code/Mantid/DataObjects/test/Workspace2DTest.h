#ifndef WORKSPACE2DTEST_H_
#define WORKSPACE2DTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/Workspace2D.h"

using Mantid::DataObjects::Workspace2D;
using Mantid::DataObjects::Histogram1D;

class Workspace2DTest : public CxxTest::TestSuite
{
public:
  void testSetX()
  {
    Workspace2D ws;
    ws.initialize(2,1,1);
    Histogram1D::RCtype X;
    Mantid::MantidVec XVec(1);
    X.access() = XVec;
    TS_ASSERT_THROWS_NOTHING( ws.setX(X) )
    TS_ASSERT_EQUALS( &(ws.readX(0)), &(ws.readX(1)) )
  }
};
#endif /*WORKSPACE2DTEST_H_*/
