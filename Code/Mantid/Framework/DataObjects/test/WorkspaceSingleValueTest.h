#ifndef TESTWORKSPACESINGLEVALUE_
#define TESTWORKSPACESINGLEVALUE_

#include <vector>
#include <algorithm>
#include <boost/shared_ptr.hpp>
#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/WorkspaceSingleValue.h"

using Mantid::DataObjects::WorkspaceSingleValue;

class WorkspaceSingleValueTest : public CxxTest::TestSuite
{

public:
  void testConstructorDefaults()
  {
    WorkspaceSingleValue ws;
    TS_ASSERT_DELTA(0.0,ws.dataX()[0],1e-6);
    TS_ASSERT_DELTA(0.0,ws.dataY()[0],1e-6);
    TS_ASSERT_DELTA(0.0,ws.dataE()[0],1e-6);
  }
  void testConstructor()
  {
    WorkspaceSingleValue ws(1,2);
    TS_ASSERT_DELTA(0.0,ws.dataX()[0],1e-6);
    TS_ASSERT_DELTA(1,ws.dataY()[0],1e-6);
    TS_ASSERT_DELTA(2,ws.dataE()[0],1e-6);
  }

  void testsetgetXvector()
  {
    WorkspaceSingleValue ws;
    Mantid::MantidVec v1(1,1.1);
    ws.dataX() = v1;
    TS_ASSERT_EQUALS(v1,ws.dataX());
  }
  void testsetgetYvector()
  {
    WorkspaceSingleValue ws;
    Mantid::MantidVec v1(1,1.1);
    ws.dataY() = v1;
    TS_ASSERT_EQUALS(v1,ws.dataY());
  }

  void testsetgetEvector()
  {
    WorkspaceSingleValue ws;
    Mantid::MantidVec v1(1,1.1);
    ws.dataE() = v1;
    TS_ASSERT_EQUALS(v1,ws.dataE());
  }

};
#endif /*TESTWORKSPACESINGLEVALUE_*/
