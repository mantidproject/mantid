// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "PropertyManagerHelper.h"

using namespace Mantid::DataObjects;
using namespace Mantid::API;

class OffsetsWorkspaceTest : public CxxTest::TestSuite {
public:
  void test_Something() {}

  void testClone() {
    auto inst = ComponentCreationHelper::createTestInstrumentCylindrical(5);

    OffsetsWorkspace ws(inst);
    TS_ASSERT_THROWS_NOTHING(ws.clone());
  }

  /**
   * Test declaring an input OffsetsWorkspace and retrieving it as const_sptr or
   * sptr
   */
  void testGetProperty_const_sptr() {
    const std::string wsName = "InputWorkspace";
    OffsetsWorkspace_sptr wsInput(new OffsetsWorkspace());
    PropertyManagerHelper manager;
    manager.declareProperty(wsName, wsInput, Mantid::Kernel::Direction::Input);

    // Check property can be obtained as const_sptr or sptr
    OffsetsWorkspace_const_sptr wsConst;
    OffsetsWorkspace_sptr wsNonConst;
    TS_ASSERT_THROWS_NOTHING(wsConst = manager.getValue<OffsetsWorkspace_const_sptr>(wsName));
    TS_ASSERT(wsConst != nullptr);
    TS_ASSERT_THROWS_NOTHING(wsNonConst = manager.getValue<OffsetsWorkspace_sptr>(wsName));
    TS_ASSERT(wsNonConst != nullptr);
    TS_ASSERT_EQUALS(wsConst, wsNonConst);

    // Check TypedValue can be cast to const_sptr or to sptr
    PropertyManagerHelper::TypedValue val(manager, wsName);
    OffsetsWorkspace_const_sptr wsCastConst;
    OffsetsWorkspace_sptr wsCastNonConst;
    TS_ASSERT_THROWS_NOTHING(wsCastConst = (OffsetsWorkspace_const_sptr)val);
    TS_ASSERT(wsCastConst != nullptr);
    TS_ASSERT_THROWS_NOTHING(wsCastNonConst = (OffsetsWorkspace_sptr)val);
    TS_ASSERT(wsCastNonConst != nullptr);
    TS_ASSERT_EQUALS(wsCastConst, wsCastNonConst);
  }
};
