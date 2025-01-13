// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ScopedWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"

using namespace Mantid::API;

/// MockWorkspace copied from AnalysisDataServiceTest so I have something to add
/// to the ADS
class MockWorkspace : public Workspace {
  const std::string id() const override { return "MockWorkspace"; }
  const std::string toString() const override { return ""; }
  size_t getMemorySize() const override { return 1; }

private:
  MockWorkspace *doClone() const override { throw std::runtime_error("Cloning of MockWorkspace is not implemented."); }
  MockWorkspace *doCloneEmpty() const override {
    throw std::runtime_error("Cloning of MockWorkspace is not implemented.");
  }
};
using MockWorkspace_sptr = std::shared_ptr<MockWorkspace>;

class ScopedWorkspaceTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ScopedWorkspaceTest *createSuite() { return new ScopedWorkspaceTest(); }
  static void destroySuite(ScopedWorkspaceTest *suite) { delete suite; }

  ScopedWorkspaceTest() : m_ads(AnalysisDataService::Instance()) { m_ads.clear(); }

  ~ScopedWorkspaceTest() override { m_ads.clear(); }

  void test_emptyConstructor() {
    ScopedWorkspace test;

    TS_ASSERT(!test.name().empty());
    TS_ASSERT(!m_ads.doesExist(test.name()));
  }

  void test_workspaceConstructor() {
    MockWorkspace_sptr ws = MockWorkspace_sptr(new MockWorkspace);
    ScopedWorkspace test(ws);

    TS_ASSERT(!test.name().empty());
    TS_ASSERT(m_ads.doesExist(test.name()));
  }

  void test_name() {
    ScopedWorkspace test;

    const std::string prefix("__ScopedWorkspace_");

    TS_ASSERT(test.name().starts_with(prefix));
    TS_ASSERT_EQUALS(test.name().size(), prefix.size() + 16);
  }

  void test_setAndRetrieve() {
    ScopedWorkspace test;

    TS_ASSERT(!test.retrieve());

    MockWorkspace_sptr ws = MockWorkspace_sptr(new MockWorkspace);
    test.set(ws);

    TS_ASSERT_EQUALS(ws, test.retrieve());
  }

  void test_removedWhenOutOfScope() {
    TS_ASSERT_EQUALS(
        m_ads.getObjectNames(Mantid::Kernel::DataServiceSort::Unsorted, Mantid::Kernel::DataServiceHidden::Include)
            .size(),
        0);

    { // Simulated scope
      MockWorkspace_sptr ws = MockWorkspace_sptr(new MockWorkspace);

      ScopedWorkspace test;
      m_ads.add(test.name(), ws);

      TS_ASSERT(m_ads.doesExist(test.name()));
    }

    // Should be removed when goes out of scope
    TS_ASSERT_EQUALS(
        m_ads.getObjectNames(Mantid::Kernel::DataServiceSort::Unsorted, Mantid::Kernel::DataServiceHidden::Include)
            .size(),
        0);
  }

  void test_removedWhenException() {
    TS_ASSERT_EQUALS(
        m_ads.getObjectNames(Mantid::Kernel::DataServiceSort::Unsorted, Mantid::Kernel::DataServiceHidden::Include)
            .size(),
        0);

    try {
      MockWorkspace_sptr ws = MockWorkspace_sptr(new MockWorkspace);

      ScopedWorkspace test;
      m_ads.add(test.name(), ws);

      TS_ASSERT(m_ads.doesExist(test.name()));

      throw std::runtime_error("Exception");

      TS_FAIL("Shouldn't get there");
    } catch (...) {
    }

    TS_ASSERT_EQUALS(
        m_ads.getObjectNames(Mantid::Kernel::DataServiceSort::Unsorted, Mantid::Kernel::DataServiceHidden::Include)
            .size(),
        0);
  }

  void test_workspaceGroups() {
    TS_ASSERT_EQUALS(
        m_ads.getObjectNames(Mantid::Kernel::DataServiceSort::Unsorted, Mantid::Kernel::DataServiceHidden::Include)
            .size(),
        0);

    { // Simulated scope
      MockWorkspace_sptr ws1 = MockWorkspace_sptr(new MockWorkspace);
      MockWorkspace_sptr ws2 = MockWorkspace_sptr(new MockWorkspace);

      WorkspaceGroup_sptr wsGroup = WorkspaceGroup_sptr(new WorkspaceGroup);

      wsGroup->addWorkspace(ws1);
      wsGroup->addWorkspace(ws2);

      ScopedWorkspace testGroup;
      m_ads.add(testGroup.name(), wsGroup);

      TS_ASSERT_EQUALS(
          m_ads.getObjectNames(Mantid::Kernel::DataServiceSort::Unsorted, Mantid::Kernel::DataServiceHidden::Include)
              .size(),
          3);
    }

    // Whole group should be removed
    TS_ASSERT_EQUALS(
        m_ads.getObjectNames(Mantid::Kernel::DataServiceSort::Unsorted, Mantid::Kernel::DataServiceHidden::Include)
            .size(),
        0);
  }

  void test_alreadyExistsInTheADS() {
    MockWorkspace_sptr ws = MockWorkspace_sptr(new MockWorkspace);

    ScopedWorkspace test(ws);

    TS_ASSERT_THROWS(ScopedWorkspace test2(ws), const std::invalid_argument &);
  }

  void test_boolConversion() {
    ScopedWorkspace test;

    TS_ASSERT(!test);

    test.set(MockWorkspace_sptr(new MockWorkspace));

    TS_ASSERT(test);
  }

  void test_settingTwice() {
    ScopedWorkspace test;

    MockWorkspace_sptr ws1 = MockWorkspace_sptr(new MockWorkspace);
    test.set(ws1);

    TS_ASSERT_EQUALS(ws1->getName(), test.name());

    MockWorkspace_sptr ws2 = MockWorkspace_sptr(new MockWorkspace);
    test.set(ws2);

    TS_ASSERT_EQUALS(ws2->getName(), test.name());
    TS_ASSERT(ws1->getName().empty());
    TS_ASSERT_EQUALS(
        m_ads.getObjectNames(Mantid::Kernel::DataServiceSort::Unsorted, Mantid::Kernel::DataServiceHidden::Include)
            .size(),
        1);
  }

private:
  AnalysisDataServiceImpl &m_ads;
};
