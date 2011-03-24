#ifndef MANTID_MDEVENTS_MDBINTEST_H_
#define MANTID_MDEVENTS_MDBINTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidMDEvents/MDBin.h"
#include "MantidMDEvents/MDEventFactory.h"

using namespace Mantid::MDEvents;

class MDBinTest : public CxxTest::TestSuite
{
public:

  void test_constructor()
  {
    typedef MDEvent<3> MDE;
    MDBin<MDE,3> bin;
    for (size_t d=0; d<3; d++)
    {
      TS_ASSERT_EQUALS( bin.m_min[d], CoordType_min );
      TS_ASSERT_EQUALS( bin.m_max[d], CoordType_max );
    }
  }


};


#endif /* MANTID_MDEVENTS_MDBINTEST_H_ */

