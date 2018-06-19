#ifndef MANTID_CUSTOMINTERFACES_GROUPTEST_H_
#define MANTID_CUSTOMINTERFACES_GROUPTEST_H_
#include <cxxtest/TestSuite.h>
#include "../../../ISISReflectometry/Reduction/Group.h"
#include "../../../ISISReflectometry/Reduction/ReductionWorkspaces.h"

using namespace MantidQt::CustomInterfaces;

class GroupTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GroupTest *createSuite() { return new GroupTest(); }
  static void destroySuite(GroupTest *suite) { delete suite; }

  ReductionWorkspaces workspaceNames() const {
    return ReductionWorkspaces({}, {"", ""}, "", "", "", "");
  }

  void testConstructorSetsGroupName() {
    auto slicedGroup = UnslicedGroup("Group1", {});
    TS_ASSERT_EQUALS("Group1", slicedGroup.name());
  }

  void testCanAddEmptyRowToGroup() {
    auto slicedGroup = UnslicedGroup("Group1", {});
    auto run = UnslicedRow({"000000", "000002"}, 0.02, {"", ""},
                           RangeInQ(0, 1, 10), 1.2, {}, workspaceNames());
    slicedGroup.appendRow(run);
    TS_ASSERT_EQUALS(run, slicedGroup[0]);
  }
};
#endif // MANTID_CUSTOMINTERFACES_GROUPTEST_H_
