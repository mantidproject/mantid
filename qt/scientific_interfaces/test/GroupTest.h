#ifndef MANTID_CUSTOMINTERFACES_GROUPTEST_H_
#define MANTID_CUSTOMINTERFACES_GROUPTEST_H_
#include <cxxtest/TestSuite.h>
#include "../ISISReflectometry/Reduction/Group.h"

using namespace MantidQt::CustomInterfaces;

class GroupTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GroupTest *createSuite() { return new GroupTest(); }
  static void destroySuite(GroupTest *suite) { delete suite; }

  void testConstructorSetsGroupName() {
    auto slicedGroup = SlicedGroup("Group1", {}, "Postprocessed Ws Name");
    TS_ASSERT_EQUALS("Group1", slicedGroup.name());
  }

  void testCanAddEmptyRowToGroup() {
    auto slicedGroup = SlicedGroup("Group1", {}, "Postprocessed Ws Name");
    auto run = SlicedRun({"000000", "000002"}, 0.02, {"", ""},
                         RangeInQ(0, 1, 10), 1.2, boost::none, {});
    slicedGroup.appendRun(run);
    TS_ASSERT_EQUALS(run, slicedGroup[0]);
  }
};
#endif // MANTID_CUSTOMINTERFACES_GROUPTEST_H_
