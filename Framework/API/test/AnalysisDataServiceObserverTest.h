// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef ANALYSISDATASERVICEOBSERVERTEST_H_
#define ANALYSISDATASERVICEOBSERVERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataServiceObserver.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"

using namespace Mantid::API;

class FakeAnalysisDataServiceObserver
    : public Mantid::API::AnalysisDataServiceObserver {

public:
  FakeAnalysisDataServiceObserver()
      : m_anyChangeHandleCalled(false), m_addHandleCalled(false),
        m_replaceHandleCalled(false), m_deleteHandleCalled(false),
        m_clearHandleCalled(false), m_renameHandleCalled(false),
        m_groupHandleCalled(false), m_unGroupHandleCalled(false),
        m_groupUpdateHandleCalled(false) {
    this->observeAll(false);
  }

  ~FakeAnalysisDataServiceObserver() { this->observeAll(false); }

  void anyChangeHandle() override { m_anyChangeHandleCalled = true; }
  void addHandle(const std::string &wsName, const Workspace_sptr &ws) override {
    UNUSED_ARG(wsName)
    UNUSED_ARG(ws)
    m_addHandleCalled = true;
  }
  void replaceHandle(const std::string &wsName,
                     const Workspace_sptr &ws) override {
    UNUSED_ARG(wsName)
    UNUSED_ARG(ws)
    m_replaceHandleCalled = true;
  }
  void deleteHandle(const std::string &wsName,
                    const Workspace_sptr &ws) override {
    UNUSED_ARG(wsName)
    UNUSED_ARG(ws)
    m_deleteHandleCalled = true;
  }
  void clearHandle() override { m_clearHandleCalled = true; }
  void renameHandle(const std::string &wsName,
                    const std::string &newName) override {
    UNUSED_ARG(wsName)
    UNUSED_ARG(newName)
    m_renameHandleCalled = true;
  }
  void groupHandle(const std::string &wsName,
                   const Workspace_sptr &ws) override {
    UNUSED_ARG(wsName)
    UNUSED_ARG(ws)
    m_groupHandleCalled = true;
  }
  void unGroupHandle(const std::string &wsName,
                     const Workspace_sptr &ws) override {
    UNUSED_ARG(wsName)
    UNUSED_ARG(ws)
    m_unGroupHandleCalled = true;
  }
  void groupUpdateHandle(const std::string &wsName,
                         const Workspace_sptr &ws) override {
    UNUSED_ARG(wsName)
    UNUSED_ARG(ws)
    m_groupUpdateHandleCalled = true;
  }

public:
  bool m_anyChangeHandleCalled, m_addHandleCalled, m_replaceHandleCalled,
      m_deleteHandleCalled, m_clearHandleCalled, m_renameHandleCalled,
      m_groupHandleCalled, m_unGroupHandleCalled, m_groupUpdateHandleCalled;
};

class AnalysisDataServiceObserverTest : public CxxTest::TestSuite {
private:
  AnalysisDataServiceImpl &ads;
  std::unique_ptr<FakeAnalysisDataServiceObserver> m_mockInheritingClass;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AnalysisDataServiceObserverTest *createSuite() {
    return new AnalysisDataServiceObserverTest();
  }
  static void destroySuite(AnalysisDataServiceObserverTest *suite) {
    delete suite;
  }

  AnalysisDataServiceObserverTest()
      : ads(AnalysisDataService::Instance()),
        m_mockInheritingClass(
            std::make_unique<FakeAnalysisDataServiceObserver>()) {
    // Loads the framework manager
    Mantid::API::FrameworkManager::Instance();
  }

  void setUp() override {
    ads.clear();
    m_mockInheritingClass = std::make_unique<FakeAnalysisDataServiceObserver>();
  }

  void addWorkspaceToADS(std::string name = "dummy") {
    IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().createUnmanaged(
            "CreateSampleWorkspace");
    alg->setChild(true);
    alg->initialize();
    alg->setPropertyValue("OutputWorkspace", name);
    alg->execute();
    MatrixWorkspace_sptr ws = alg->getProperty("OutputWorkspace");
    ads.addOrReplace(name, ws);
  }

  void test_anyChangeHandle_is_called_on_add() {
    m_mockInheritingClass->observeAll();
    addWorkspaceToADS("dummy");

    TS_ASSERT(m_mockInheritingClass->m_anyChangeHandleCalled)
  }

  void test_addHandle_is_called_on_add() {
    m_mockInheritingClass->observeAdd();
    addWorkspaceToADS("dummy");

    TS_ASSERT(m_mockInheritingClass->m_addHandleCalled)
  }

  void test_deleteHandle_is_called_on_delete() {
    addWorkspaceToADS();

    m_mockInheritingClass->observeDelete();
    ads.remove("dummy");

    TS_ASSERT(m_mockInheritingClass->m_deleteHandleCalled)
  }

  void test_replaceHandle_is_called_on_replace() {
    addWorkspaceToADS("dummy");

    m_mockInheritingClass->observeReplace();
    addWorkspaceToADS("dummy");

    TS_ASSERT(m_mockInheritingClass->m_replaceHandleCalled)
  }

  void test_clearHandle_is_called_on_clear() {
    addWorkspaceToADS("dummy");

    m_mockInheritingClass->observeClear();
    ads.clear();

    TS_ASSERT(m_mockInheritingClass->m_clearHandleCalled)
  }

  void test_renameHandle_is_called_on_rename() {
    addWorkspaceToADS("dummy");

    m_mockInheritingClass->observeRename();
    IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().createUnmanaged(
            "RenameWorkspace");
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", "dummy");
    alg->setPropertyValue("OutputWorkspace", "dummy2");
    alg->execute();

    TS_ASSERT(m_mockInheritingClass->m_renameHandleCalled)
  }

  void test_groupHandle_is_called_on_group_made() {
    addWorkspaceToADS("dummy");
    addWorkspaceToADS("dummy2");

    m_mockInheritingClass->observeGroup();

    IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().createUnmanaged(
            "GroupWorkspaces");
    alg->initialize();
    alg->setPropertyValue("InputWorkspaces", "dummy,dummy2");
    alg->setPropertyValue("OutputWorkspace", "newGroup");
    alg->execute();

    TS_ASSERT(m_mockInheritingClass->m_groupHandleCalled)
  }

  void test_unGroupHandle_is_called_on_un_grouping() {
    addWorkspaceToADS("dummy");
    addWorkspaceToADS("dummy2");

    IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().createUnmanaged(
            "GroupWorkspaces");
    alg->initialize();
    alg->setPropertyValue("InputWorkspaces", "dummy,dummy2");
    alg->setPropertyValue("OutputWorkspace", "newGroup");
    alg->execute();

    m_mockInheritingClass->observeUnGroup();

    IAlgorithm_sptr alg2 =
        Mantid::API::AlgorithmManager::Instance().createUnmanaged(
            "UnGroupWorkspace");
    alg2->initialize();
    alg2->setPropertyValue("InputWorkspace", "newGroup");
    alg2->execute();

    TS_ASSERT(m_mockInheritingClass->m_unGroupHandleCalled)
  }

  void test_groupUpdated_is_called_on_group_updated() {
    addWorkspaceToADS("dummy");
    addWorkspaceToADS("dummy2");
    addWorkspaceToADS("dummy3");

    IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().createUnmanaged(
            "GroupWorkspaces");
    alg->initialize();
    alg->setPropertyValue("InputWorkspaces", "dummy,dummy2");
    alg->setPropertyValue("OutputWorkspace", "newGroup");
    alg->execute();

    m_mockInheritingClass->observeGroupUpdate();

    ads.addToGroup("newGroup", "dummy3");

    TS_ASSERT(m_mockInheritingClass->m_groupUpdateHandleCalled)
  }
};

#endif /* ANALYSISDATASERVICEOBSERVERTEST_H_ */