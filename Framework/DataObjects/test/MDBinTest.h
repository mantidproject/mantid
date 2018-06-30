#ifndef MANTID_DATAOBJECTS_MDBINTEST_H_
#define MANTID_DATAOBJECTS_MDBINTEST_H_

#include "MantidDataObjects/MDBin.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::DataObjects;

class MDBinTest : public CxxTest::TestSuite {
public:
  void test_constructor() {
    using MDE = MDLeanEvent<3>;
    MDBin<MDE, 3> bin;
    for (size_t d = 0; d < 3; d++) {
      TS_ASSERT_EQUALS(bin.m_min[d], -std::numeric_limits<coord_t>::max());
      TS_ASSERT_EQUALS(bin.m_max[d], std::numeric_limits<coord_t>::max());
    }
  }
};

#endif /* MANTID_DATAOBJECTS_MDBINTEST_H_ */
