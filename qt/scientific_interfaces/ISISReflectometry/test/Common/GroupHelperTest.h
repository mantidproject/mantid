// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/Common/GroupHelper.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using ::testing::_;
using ::testing::Invoke;

class GroupHelperTest : public CxxTest::TestSuite {
public:
  static GroupHelperTest *createSuite() { return new GroupHelperTest(); }
  static void destroySuite(GroupHelperTest *suite) { delete suite; }

  void test_getMembers() {
    // Given
    auto ws1 = WorkspaceCreationHelper::create2DWorkspace(1, 1);
    auto ws2 = WorkspaceCreationHelper::create2DWorkspace(1, 1);
    auto group = std::make_shared<Mantid::API::WorkspaceGroup>();
    group->addWorkspace(ws1);
    group->addWorkspace(ws2);

    // When
    auto const &groupMembers = getMembers(group);

    // Then
    TS_ASSERT_EQUALS(groupMembers.size(), 2);
    TS_ASSERT_EQUALS(groupMembers[0], ws1);
    TS_ASSERT_EQUALS(groupMembers[1], ws2)
  }
};
