#ifndef MANTID_DATAOBJECTS_PEAKCOLUMNTEST_H_
#define MANTID_DATAOBJECTS_PEAKCOLUMNTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeakColumn.h"

using namespace Mantid::DataObjects;

class PeakColumnTest : public CxxTest::TestSuite
{
public:

  void test_constructor()
  {
    std::vector<Peak> peaks;
    PeakColumn pc(peaks, "h");
    TS_ASSERT_EQUALS( pc.name(), "h");
    TS_ASSERT_EQUALS( pc.size(), 0);
  }



};


#endif /* MANTID_DATAOBJECTS_PEAKCOLUMNTEST_H_ */

