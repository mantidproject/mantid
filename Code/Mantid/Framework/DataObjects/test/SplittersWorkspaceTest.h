#ifndef MANTID_DATAOBJECTS_SPLITTERSWORKSPACETEST_H_
#define MANTID_DATAOBJECTS_SPLITTERSWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidDataObjects/SplittersWorkspace.h"

using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::API;

class SplittersWorkspaceTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SplittersWorkspaceTest *createSuite() { return new SplittersWorkspaceTest(); }
  static void destroySuite( SplittersWorkspaceTest *suite ) { delete suite; }


  void test_Add()
  {
    DataObjects::SplittersWorkspace splitterws;

    Kernel::SplittingInterval s1(Kernel::DateAndTime(10000), Kernel::DateAndTime(15000), 1);
    Kernel::SplittingInterval s2(Kernel::DateAndTime(20000), Kernel::DateAndTime(30000), 3);
    Kernel::SplittingInterval s3(Kernel::DateAndTime(40000), Kernel::DateAndTime(50000), 2);

    TS_ASSERT_THROWS_NOTHING(splitterws.addSplitter(s1));
    TS_ASSERT_THROWS_NOTHING(splitterws.addSplitter(s2));
    TS_ASSERT_THROWS_NOTHING(splitterws.addSplitter(s3));

    TS_ASSERT_EQUALS(splitterws.getNumberSplitters(), 3);
  }

  void test_AddGet()
  {
    DataObjects::SplittersWorkspace splitterws;

    Kernel::SplittingInterval s1(Kernel::DateAndTime(10000), Kernel::DateAndTime(15000), 1);
    Kernel::SplittingInterval s2(Kernel::DateAndTime(20000), Kernel::DateAndTime(30000), 3);
    Kernel::SplittingInterval s3(Kernel::DateAndTime(40000), Kernel::DateAndTime(50000), 2);

    std::vector<Kernel::SplittingInterval> splitters;
    splitters.push_back(s1);
    splitters.push_back(s2);
    splitters.push_back(s3);

    TS_ASSERT_THROWS_NOTHING(splitterws.addSplitter(s1));
    TS_ASSERT_THROWS_NOTHING(splitterws.addSplitter(s2));
    TS_ASSERT_THROWS_NOTHING(splitterws.addSplitter(s3));

    for (size_t i = 0; i < 3; i ++)
    {
      Kernel::SplittingInterval splitter = splitterws.getSplitter(i);
      TS_ASSERT_EQUALS(splitter.start(), splitters[i].start());
      TS_ASSERT_EQUALS(splitter.stop(), splitters[i].stop());
      TS_ASSERT_EQUALS(splitter.index(), splitters[i].index());
    }

  }


};


#endif /* MANTID_DATAOBJECTS_SPLITTERSWORKSPACETEST_H_ */
