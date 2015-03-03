#ifndef MANTID_MDEVENTS_MDBINTEST_H_
#define MANTID_MDEVENTS_MDBINTEST_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidDataObjects/MDBin.h"
#include "MantidMDEvents/MDEventFactory.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid;
using namespace Mantid::MDEvents;

class MDBinTest : public CxxTest::TestSuite
{
public:

  void test_constructor()
  {
    typedef MDLeanEvent<3> MDE;
    MDBin<MDE,3> bin;
    for (size_t d=0; d<3; d++)
    {
      TS_ASSERT_EQUALS( bin.m_min[d], -std::numeric_limits<coord_t>::max() );
      TS_ASSERT_EQUALS( bin.m_max[d], std::numeric_limits<coord_t>::max() );
    }
  }


};


#endif /* MANTID_MDEVENTS_MDBINTEST_H_ */

