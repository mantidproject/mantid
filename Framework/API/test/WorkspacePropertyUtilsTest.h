// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspacePropertyUtils.h"
#include "MantidKernel/Property.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

class WorkspacePropertyUtilsTest : public CxxTest::TestSuite {

public:
  static WorkspacePropertyUtilsTest *createSuite() { return new WorkspacePropertyUtilsTest(); }

  static void destroySuite(WorkspacePropertyUtilsTest *suite) { delete suite; }

  void test_set_property_mode_mandatory_for_different_workspace_types() {
    assertSetPropertyModeMandatory<Mantid::API::Workspace>();
    assertSetPropertyModeMandatory<Mantid::API::MatrixWorkspace>();
    assertSetPropertyModeMandatory<Mantid::API::WorkspaceGroup>();
    assertSetPropertyModeMandatory<Mantid::API::IEventWorkspace>();
    assertSetPropertyModeMandatory<Mantid::API::IMDHistoWorkspace>();
    assertSetPropertyModeMandatory<Mantid::API::IPeaksWorkspace>();
    assertSetPropertyModeMandatory<Mantid::API::ITableWorkspace>();
  }

  void test_set_property_mode_optional_for_different_workspace_types() {
    assertSetPropertyModeOptional<Mantid::API::Workspace>();
    assertSetPropertyModeOptional<Mantid::API::MatrixWorkspace>();
    assertSetPropertyModeOptional<Mantid::API::WorkspaceGroup>();
    assertSetPropertyModeOptional<Mantid::API::IEventWorkspace>();
    assertSetPropertyModeOptional<Mantid::API::IMDHistoWorkspace>();
    assertSetPropertyModeOptional<Mantid::API::IPeaksWorkspace>();
    assertSetPropertyModeOptional<Mantid::API::ITableWorkspace>();
  }

private:
  template <typename T> void assertSetPropertyModeMandatory() {
    auto prop = std::make_unique<WorkspaceProperty<T>>("Name", "", Direction::Output, PropertyMode::Type::Optional);
    TS_ASSERT(prop->isOptional());

    Mantid::API::setPropertyModeForWorkspaceProperty(prop.get(), PropertyMode::Type::Mandatory);
    TS_ASSERT(!prop->isOptional());
  }

  template <typename T> void assertSetPropertyModeOptional() {
    auto prop = std::make_unique<WorkspaceProperty<T>>("Name", "", Direction::Output);
    TS_ASSERT(!prop->isOptional());

    Mantid::API::setPropertyModeForWorkspaceProperty(prop.get(), PropertyMode::Type::Optional);
    TS_ASSERT(prop->isOptional());
  }
};
