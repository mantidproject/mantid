// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/SplittersWorkspace.h"
#include "MantidKernel/Timer.h"
#include "PropertyManagerHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::API;

class SplittersWorkspaceTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SplittersWorkspaceTest *createSuite() { return new SplittersWorkspaceTest(); }
  static void destroySuite(SplittersWorkspaceTest *suite) { delete suite; }

  void testClone() {
    SplittersWorkspace splitterws;

    Kernel::SplittingInterval s1(Types::Core::DateAndTime(10000), Types::Core::DateAndTime(15000), 1);
    Kernel::SplittingInterval s2(Types::Core::DateAndTime(20000), Types::Core::DateAndTime(30000), 3);
    Kernel::SplittingInterval s3(Types::Core::DateAndTime(40000), Types::Core::DateAndTime(50000), 2);

    splitterws.addSplitter(s1);
    splitterws.addSplitter(s2);
    splitterws.addSplitter(s3);

    auto cloned = splitterws.clone();

    // Check clone is same as original.
    TS_ASSERT_EQUALS(splitterws.columnCount(), cloned->columnCount());
    TS_ASSERT_EQUALS(splitterws.rowCount(), cloned->rowCount());
    TS_ASSERT_EQUALS(splitterws.getNumberSplitters(), 3);
  }

  void test_Add() {
    DataObjects::SplittersWorkspace splitterws;

    Kernel::SplittingInterval s1(Types::Core::DateAndTime(10000), Types::Core::DateAndTime(15000), 1);
    Kernel::SplittingInterval s2(Types::Core::DateAndTime(20000), Types::Core::DateAndTime(30000), 3);
    Kernel::SplittingInterval s3(Types::Core::DateAndTime(40000), Types::Core::DateAndTime(50000), 2);

    TS_ASSERT_THROWS_NOTHING(splitterws.addSplitter(s1));
    TS_ASSERT_THROWS_NOTHING(splitterws.addSplitter(s2));
    TS_ASSERT_THROWS_NOTHING(splitterws.addSplitter(s3));

    TS_ASSERT_EQUALS(splitterws.getNumberSplitters(), 3);
  }

  void test_AddGet() {
    DataObjects::SplittersWorkspace splitterws;

    Kernel::SplittingInterval s1(Types::Core::DateAndTime(10000), Types::Core::DateAndTime(15000), 1);
    Kernel::SplittingInterval s2(Types::Core::DateAndTime(20000), Types::Core::DateAndTime(30000), 3);
    Kernel::SplittingInterval s3(Types::Core::DateAndTime(40000), Types::Core::DateAndTime(50000), 2);

    std::vector<Kernel::SplittingInterval> splitters;
    splitters.emplace_back(s1);
    splitters.emplace_back(s2);
    splitters.emplace_back(s3);

    TS_ASSERT_THROWS_NOTHING(splitterws.addSplitter(s1));
    TS_ASSERT_THROWS_NOTHING(splitterws.addSplitter(s2));
    TS_ASSERT_THROWS_NOTHING(splitterws.addSplitter(s3));

    for (size_t i = 0; i < 3; i++) {
      Kernel::SplittingInterval splitter = splitterws.getSplitter(i);
      TS_ASSERT_EQUALS(splitter.start(), splitters[i].start());
      TS_ASSERT_EQUALS(splitter.stop(), splitters[i].stop());
      TS_ASSERT_EQUALS(splitter.index(), splitters[i].index());
    }
  }

  void test_known_to_property_for_unmangling() {
    Mantid::API::WorkspaceProperty<DataObjects::SplittersWorkspace> property("DummyProperty", "DummyWorkspace",
                                                                             Mantid::Kernel::Direction::Input);
    TS_ASSERT_EQUALS("SplittersWorkspace", Mantid::Kernel::getUnmangledTypeName(*property.type_info()));
  }

  /**
   * Test declaring an input SplittersWorkspace and retrieving it as const_sptr
   * or sptr
   */
  void testGetProperty_const_sptr() {
    const std::string wsName = "InputWorkspace";
    SplittersWorkspace_sptr wsInput(new SplittersWorkspace());
    PropertyManagerHelper manager;
    manager.declareProperty(wsName, wsInput, Mantid::Kernel::Direction::Input);

    // Check property can be obtained as const_sptr or sptr
    SplittersWorkspace_const_sptr wsConst;
    SplittersWorkspace_sptr wsNonConst;
    TS_ASSERT_THROWS_NOTHING(wsConst = manager.getValue<SplittersWorkspace_const_sptr>(wsName));
    TS_ASSERT(wsConst != nullptr);
    TS_ASSERT_THROWS_NOTHING(wsNonConst = manager.getValue<SplittersWorkspace_sptr>(wsName));
    TS_ASSERT(wsNonConst != nullptr);
    TS_ASSERT_EQUALS(wsConst, wsNonConst);

    // Check TypedValue can be cast to const_sptr or to sptr
    PropertyManagerHelper::TypedValue val(manager, wsName);
    SplittersWorkspace_const_sptr wsCastConst;
    SplittersWorkspace_sptr wsCastNonConst;
    TS_ASSERT_THROWS_NOTHING(wsCastConst = (SplittersWorkspace_const_sptr)val);
    TS_ASSERT(wsCastConst != nullptr);
    TS_ASSERT_THROWS_NOTHING(wsCastNonConst = (SplittersWorkspace_sptr)val);
    TS_ASSERT(wsCastNonConst != nullptr);
    TS_ASSERT_EQUALS(wsCastConst, wsCastNonConst);
  }
};
