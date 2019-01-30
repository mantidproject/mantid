// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_GROUPTEST_H_
#define MANTID_CUSTOMINTERFACES_GROUPTEST_H_
#include "../../../ISISReflectometry/Reduction/Group.h"
#include "../../../ISISReflectometry/Reduction/ReductionWorkspaces.h"
#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces;

class GroupTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GroupTest *createSuite() { return new GroupTest(); }
  static void destroySuite(GroupTest *suite) { delete suite; }

  ReductionWorkspaces workspaceNames() const {
    return ReductionWorkspaces({}, TransmissionRunPair{});
  }

  void testConstructorSetsGroupName() {
    auto group = Group("Group1", {});
    TS_ASSERT_EQUALS("Group1", group.name());
  }

  void testCanAddEmptyRowToGroup() {
    auto group = Group("Group1", {});
    auto run = Row({"000000", "000002"}, 0.02, {"", ""},
                   RangeInQ(0.0, 1.0, 10.0), 1.2, {}, workspaceNames());
    group.appendRow(run);
    TS_ASSERT_EQUALS(run.runNumbers(), group[0].get().runNumbers());
  }
};
#endif // MANTID_CUSTOMINTERFACES_GROUPTEST_H_
