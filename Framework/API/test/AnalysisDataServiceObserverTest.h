// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef ANALYSISDATASERVICEOBSERVERTEST_H_
#define ANALYSISDATASERVICEOBSERVERTEST_H_

#include <cxxtest/TestSuite.h>

#import "MantidAPI/AnalysisDataServiceObserver.h"

class MockInheritingClass : public Mantid::API::AnalysisDataServiceObserver {

  MockInheritingClass()
      : m_anyChangeHandleCalled(false), m_addHandleCalled(false),
        m_replaceHandleCalled(false), m_deleteHandleCalled(false),
        m_clearHandleCalled(false), m_renameHandleCalled(false),
        m_groupHandleCalled(false), m_unGroupHandleCalled(false),
        m_groupUpdateHandleCalled(false) {
    this->observeAll(false);
  }

  ~MockInheritingClass() { this->observeAll(false); }

  void anyChangeHandle() override { m_anyChangeHandleCalled = true; }
  void addHandle(const std::string &wsName, const Workspace_sptr ws) override {
    m_addHandleCalled = true;
  }
  void replaceHandle(const std::string &wsName,
                     const Workspace_sptr ws) override {
    m_replaceHandleCalled = true;
  }
  void deleteHandle(const std::string &wsName,
                    const Workspace_sptr ws) override {
    m_deleteHandleCalled = true;
  }
  void clearHandle() { m_clearHandleCalled = true; }
  void renameHandle(const std::string &wsName, const std::string &newName) {
    m_renameHandleCalled = true;
  }
  void groupHandle(const std::string &wsName, const Workspace_sptr ws) {
    m_groupHandleCalled = true;
  }
  void unGroupHandle(const std::string &wsName, const Workspace_sptr ws) {
    m_unGroupHandleCalled = true;
  }
  void groupUpdateHandle(const std::string &wsName, const Workspace_sptr ws) {
    m_groupUpdateHandleCalled = true;
  }

  bool m_anyChangeHandleCalled, m_addHandleCalled, m_replaceHandleCalled,
      m_deleteHandleCalled, m_clearHandleCalled, m_renameHandleCalled,
      m_groupHandleCalled, m_unGroupHandleCalled, m_groupUpdateHandleCalled;
}

class AnalysisDataServiceObserverTest : public CxxTest::TestSuite {
private:
  AnalysisDataServiceImpl &ads;
  std::unique_ptr<MockInheritingClass> m_mockInheritingClass;

  void setUp() {
    ads.clear();
    m_mockInheritingClass = std::make_unique<MockInheritingClass>()
  }

  void addWorkspaceToADS(std::string name = "dummy") {
    CreateSampleWorkspace alg;
    alg.setChild(true);
    alg.initialize();
    alg.setPropertyValue("OutputWorkspace", name);
    alg.execute();
  }

  void test_anyChangeHandle_is_called_on_add() {
    m_mockInheritingClass->observeAll();
    addWorkspaceToADS();

    TS_ASSERT(m_mockInheritingClass.m_anyChangeHandleCalled)
  }

  void test_addHandle_is_called_on_add() {
    m_mockInheritingClass->observeAdd();
    addWorkspaceToADS();

    TS_ASSERT(m_mockInheritingClass.m_addHandleCalled)
  }

  void test_deleteHandle_is_called_on_delete() {
    addWorkspaceToADS();

    m_mockInheritingClass->observeDelete();
    ads.remove("dummy");

    TS_ASSERT(m_mockInheritingClass.m_deleteHandleCalled)
  }

  void test_replaceHandle_is_called_on_replace() {
    addWorkspaceToADS();

    m_mockInheritingClass->observeReplace();
    addWorkspaceToADS();

    TS_ASSERT(m_mockInheritingClass.m_replaceHandleCalled)
  }

  void test_clearHandle_is_called_on_clear() {
    addWorkspaceToADS();

    m_mockInheritingClass->observeClear();
    ads.clear();

    TS_ASSERT(m_mockInheritingClass.m_clearHandleCalled)
  }

  void test_renameHandle_is_called_on_rename() {
    addWorkspaceToADS();

    m_mockInheritingClass->observeRename();
    Mantid::Algorithms::RenameWorkspace alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", "dummy");
    alg.setPropertyValue("OutputWorkspace", "dummy2");
    alg.execute();

    TS_ASSERT(m_mockInheritingClass.m_renameHandleCalled)
  }

  void test_groupHandle_is_called_on_group_made() {
    addWorkspaceToADS();
    addWorkspaceToADS("dummy2");

    m_mockInheritingClass->observeGroup();

    Mantid::Algorithms::GroupWorkspaces alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspaces", "dummy,dummy2");
    alg.setPropertyValue("OutputWorkspace", "newGroup");
    alg.execute();

    TS_ASSERT(m_mockInheritingClass.m_groupHandleCalled)
  }

  void test_unGroupHandle_is_called_on_un_grouping() {
    addWorkspaceToADS();
    addWorkspaceToADS("dummy2");

    Mantid::Algorithms::GroupWorkspaces alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspaces", "dummy,dummy2");
    alg.setPropertyValue("OutputWorkspace", "newGroup");
    alg.execute();

    m_mockInheritingClass->observeUnGroup();

    Mantid::Algorithms::UnGroupWorkspaces alg2;
    alg2.initialize();
    alg2.setPropertyValue("InputWorkspace", "newGroup");
    alg.exectute();

    TS_ASSERT(m_mockInheritingClass.m_unGroupHandleCalled)
  }

  void test_groupUpdated_is_called_on_group_updated() {
    addWorkspaceToADS();
    addWorkspaceToADS("dummy2");
    addWorkspaceToADS("dummy3");

    Mantid::Algorithms::GroupWorkspaces alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspaces", "dummy,dummy2");
    alg.setPropertyValue("OutputWorkspace", "newGroup");
    alg.execute();

    m_mockInheritingClass->observeGroup();

    ads.addToGroup("newGroup", "dummy3");

    TS_ASSERT(m_mockInheritingClass.m_groupUpdateHandleCalled)
  }
};

#endif /* ANALYSISDATASERVICEOBSERVERTEST_H_ */