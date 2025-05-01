// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "FakeAlgorithms.h"
#include "MantidAPI/Algorithm.hxx"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/AlgorithmHistory.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidFrameworkTestHelpers/FakeObjects.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/ReadLock.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/WriteLock.h"
#include "PropertyManagerHelper.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <map>
#include <utility>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::Indexing::SpectrumIndexSet;

class StubbedWorkspaceAlgorithm : public Algorithm {
public:
  StubbedWorkspaceAlgorithm() : Algorithm() {}
  ~StubbedWorkspaceAlgorithm() override = default;

  const std::string name() const override { return "StubbedWorkspaceAlgorithm"; }
  int version() const override { return 1; }
  const std::string category() const override { return "Cat;Leopard;Mink"; }
  const std::string summary() const override { return "Test summary"; }

  void init() override {
    declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace1", "", Direction::Input));
    declareProperty(
        std::make_unique<WorkspaceProperty<>>("InputWorkspace2", "", Direction::Input, PropertyMode::Optional));
    declareProperty(
        std::make_unique<WorkspaceProperty<>>("InOutWorkspace", "", Direction::InOut, PropertyMode::Optional));
    declareProperty("Number", 0.0);
    declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace1", "", Direction::Output));
    declareProperty(
        std::make_unique<WorkspaceProperty<>>("OutputWorkspace2", "", Direction::Output, PropertyMode::Optional));
  }

  void exec() override {
    const std::string outName = getPropertyValue("InputWorkspace1") + "+" + getPropertyValue("InputWorkspace2") + "+" +
                                getPropertyValue("InOutWorkspace");
    auto out1 = std::make_shared<WorkspaceTester>();
    out1->initialize(10, 10, 10);
    out1->setTitle(outName);
    out1->dataY(0)[0] = getProperty("Number");
    setProperty("OutputWorkspace1", out1);
    if (!getPropertyValue("OutputWorkspace2").empty()) {
      auto out2 = std::make_shared<WorkspaceTester>();
      out2->initialize(10, 10, 10);
      out2->setTitle(outName);
      setProperty("OutputWorkspace2", out2);
    }
  }
};
DECLARE_ALGORITHM(StubbedWorkspaceAlgorithm)

class StubbedWorkspaceAlgorithm2 : public Algorithm {
public:
  StubbedWorkspaceAlgorithm2() : Algorithm() {}
  ~StubbedWorkspaceAlgorithm2() override = default;

  const std::string name() const override { return "StubbedWorkspaceAlgorithm2"; }
  int version() const override { return 2; }
  const std::string category() const override { return "Cat;Leopard;Mink"; }
  const std::string summary() const override { return "Test summary"; }
  void init() override {
    declareProperty(std::make_unique<WorkspaceProperty<>>("NonLockingInputWorkspace", "", Direction::Input,
                                                          PropertyMode::Optional, LockMode::NoLock));
    declareProperty(std::make_unique<WorkspaceProperty<>>("NonLockingOutputWorkspace", "", Direction::Output,
                                                          PropertyMode::Optional, LockMode::NoLock));
  }
  void exec() override {}
};
DECLARE_ALGORITHM(StubbedWorkspaceAlgorithm2)

class AlgorithmWithValidateInputs : public Algorithm {
public:
  AlgorithmWithValidateInputs() : Algorithm() {}
  ~AlgorithmWithValidateInputs() override = default;
  const std::string name() const override { return "StubbedWorkspaceAlgorithm2"; }
  int version() const override { return 1; }
  const std::string category() const override { return "Cat;Leopard;Mink"; }
  const std::string summary() const override { return "Test summary"; }
  const std::string workspaceMethodName() const override { return "methodname"; }
  const std::string workspaceMethodOnTypes() const override { return "MatrixWorkspace;ITableWorkspace"; }
  const std::string workspaceMethodInputProperty() const override { return "InputWorkspace"; }

  void init() override {
    declareProperty("PropertyA", 12);
    declareProperty("PropertyB", 12);
  }
  void exec() override {}
  std::map<std::string, std::string> validateInputs() override {
    std::map<std::string, std::string> out;
    int A = getProperty("PropertyA");
    int B = getProperty("PropertyB");
    if (B < A)
      out["PropertyB"] = "B must be >= A!";
    return out;
  }
};
DECLARE_ALGORITHM(AlgorithmWithValidateInputs)

/**
 * Algorithm which fails on specified workspace
 */
class FailingAlgorithm : public Algorithm {
public:
  FailingAlgorithm() : Algorithm() {}
  ~FailingAlgorithm() override = default;
  const std::string name() const override { return "FailingAlgorithm"; }
  int version() const override { return 1; }
  const std::string summary() const override { return "Test summary"; }
  static const std::string FAIL_MSG;

  void init() override {
    declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input));
    declareProperty("WsNameToFail", "");
  }

  void exec() override {
    std::string wsNameToFail = getPropertyValue("WsNameToFail");
    std::string wsName = getPropertyValue("InputWorkspace");

    if (wsName == wsNameToFail) {
      throw std::runtime_error(FAIL_MSG);
    }
  }
};

const std::string FailingAlgorithm::FAIL_MSG("Algorithm failed as requested");

DECLARE_ALGORITHM(FailingAlgorithm)

class IndexingAlgorithm : public Algorithm {
public:
  const std::string name() const override { return "IndexingAlgorithm"; }
  int version() const override { return 1; }
  const std::string summary() const override { return "Test indexing property creation"; }
  static const std::string FAIL_MSG;

  void init() override {
    declareWorkspaceInputProperties<MatrixWorkspace>("InputWorkspace", "");
    declareProperty(
        std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace2", "", Mantid::Kernel::Direction::Input));
    declareWorkspaceInputProperties<MatrixWorkspace, static_cast<int>(IndexType::SpectrumNum) |
                                                         static_cast<int>(IndexType::WorkspaceIndex)>("InputWorkspace3",
                                                                                                      "");
    declareWorkspaceInputProperties<MatrixWorkspace, static_cast<int>(IndexType::SpectrumNum) |
                                                         static_cast<int>(IndexType::WorkspaceIndex)>(
        "InputWorkspace4", "", std::make_shared<HistogramValidator>());
  }

  void exec() override {}
};

DECLARE_ALGORITHM(IndexingAlgorithm)

class AlgorithmTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AlgorithmTest *createSuite() { return new AlgorithmTest(); }
  static void destroySuite(AlgorithmTest *suite) { delete suite; }

  AlgorithmTest() {
    Mantid::API::FrameworkManager::Instance();
    AnalysisDataService::Instance();
    Mantid::API::AlgorithmFactory::Instance().subscribe<ToyAlgorithm>();
    Mantid::API::AlgorithmFactory::Instance().subscribe<ToyAlgorithmTwo>();
  }

  ~AlgorithmTest() override {
    Mantid::API::AlgorithmFactory::Instance().unsubscribe("ToyAlgorithm", 1);
    Mantid::API::AlgorithmFactory::Instance().unsubscribe("ToyAlgorithmTwo", 1);
  }

  void setUp() override { AnalysisDataService::Instance().clear(); }

  void testAlgorithm() {
    std::string theName = alg.name();
    TS_ASSERT(!theName.compare("ToyAlgorithm"));
    int theVersion = alg.version();
    TS_ASSERT_EQUALS(theVersion, 1);
    TS_ASSERT(!alg.isInitialized());
    TS_ASSERT(!alg.isExecuted());
  }

  void testName() {
    std::string theName = alg.name();
    TS_ASSERT(!theName.compare("ToyAlgorithm"));
  }

  void testVersion() {
    int theVersion = alg.version();
    TS_ASSERT_EQUALS(theVersion, 1);
  }

  void testCategory() {
    TS_ASSERT_EQUALS(alg.category(), "Cat");
    TS_ASSERT_EQUALS(algv2.category(), "Cat,Leopard,Mink");
  }

  void testCategories() {
    std::vector<std::string> result{"Cat"};
    TS_ASSERT_EQUALS(alg.categories(), result);
    result.emplace_back("Leopard");
    result.emplace_back("Mink");
    TS_ASSERT_EQUALS(algv2.categories(), result);
    TS_ASSERT_EQUALS(algv3.categories(), result);
  }

  void testSeeAlso() {
    std::vector<std::string> result{"rabbit"};
    result.emplace_back("goldfish");
    result.emplace_back("Spotted Hyena");
    TS_ASSERT_EQUALS(alg.seeAlso(), result);
  }

  void testAlias() { TS_ASSERT_EQUALS(alg.alias(), "Dog"); }

  void testIsChild() {
    TS_ASSERT_EQUALS(false, alg.isChild());
    alg.setChild(true);
    TS_ASSERT_EQUALS(true, alg.isChild());
    alg.setChild(false);
    TS_ASSERT_EQUALS(false, alg.isChild());
  }

  void testAlwaysStoreInADSGetterSetter() {
    TS_ASSERT(alg.getAlwaysStoreInADS())
    alg.setAlwaysStoreInADS(false);
    TS_ASSERT(!alg.getAlwaysStoreInADS())
    alg.setAlwaysStoreInADS(true);
    TS_ASSERT(alg.getAlwaysStoreInADS())
  }

  void testAlgStartupLogging() {
    TSM_ASSERT_EQUALS("Default logging should be true", true, alg.getAlgStartupLogging());
    alg.setAlgStartupLogging(false);
    TSM_ASSERT_EQUALS("After setting logging should be false", false, alg.getAlgStartupLogging());
    alg.setAlgStartupLogging(true);
    TSM_ASSERT_EQUALS("After setting logging it back it should be true", true, alg.getAlgStartupLogging());
  }

  void testInitialize() {
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void testExecute() {
    ToyAlgorithm myAlg;
    TS_ASSERT_EQUALS(ExecutionState::Uninitialized, myAlg.executionState());
    TS_ASSERT_THROWS(myAlg.execute(), const std::runtime_error &);
    TS_ASSERT(!myAlg.isExecuted());
    TS_ASSERT_EQUALS(ExecutionState::Uninitialized, myAlg.executionState());
    TS_ASSERT_THROWS_NOTHING(myAlg.initialize());
    TS_ASSERT_EQUALS(ExecutionState::Initialized, myAlg.executionState());
    TS_ASSERT_THROWS_NOTHING(myAlg.execute());
    TS_ASSERT(myAlg.isExecuted());
    TS_ASSERT_EQUALS(ExecutionState::Finished, myAlg.executionState());
    TS_ASSERT_EQUALS(ResultState::Success, myAlg.resultState());
  }

  void testSetPropertyValue() {
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("prop1", "val"))
    TS_ASSERT_THROWS(alg.setPropertyValue("prop3", "1"), const Exception::NotFoundError &)
  }

  void testExistsProperty() {
    TS_ASSERT(alg.existsProperty("prop1"))
    TS_ASSERT(!alg.existsProperty("notThere"))
  }

  void testGetPropertyValue() {
    std::string value;
    TS_ASSERT_THROWS_NOTHING(value = alg.getPropertyValue("prop2"))
    TS_ASSERT(!value.compare("1"))
    TS_ASSERT_THROWS(alg.getPropertyValue("ghjkgh"), const Exception::NotFoundError &)
  }

  void testGetProperties() {
    std::vector<Property *> vec = alg.getProperties();
    TS_ASSERT(!vec.empty())
    TS_ASSERT(vec.size() == 2)
    TS_ASSERT(!vec[0]->name().compare("prop1"))
  }

  /** The check in validateInputs() makes the algo throw if there is anything
   * wrong */
  void test_validateInputs_makesAlgorithmFail() {
    AlgorithmWithValidateInputs alg;
    alg.initialize();
    alg.setProperty("PropertyA", 12);
    alg.setProperty("PropertyB", 5);
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
    // Algoritm never executed as property validation failed
    TS_ASSERT(!alg.isExecuted());
    TS_ASSERT_EQUALS(ExecutionState::Initialized, alg.executionState());
    TS_ASSERT_EQUALS(ResultState::NotFinished, alg.resultState());

    alg.setProperty("PropertyB", 15);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    TS_ASSERT_EQUALS(ExecutionState::Finished, alg.executionState());
    TS_ASSERT_EQUALS(ResultState::Success, alg.resultState());
  }

  void test_WorkspaceMethodFunctionsReturnEmptyByDefault() {
    StubbedWorkspaceAlgorithm alg;

    TS_ASSERT_EQUALS("", alg.workspaceMethodName());
    TS_ASSERT_EQUALS(std::vector<std::string>(), alg.workspaceMethodOn());
    TS_ASSERT_EQUALS("", alg.workspaceMethodInputProperty());
  }

  void test_WorkspaceMethodsReturnTypesCorrectly() {
    AlgorithmWithValidateInputs alg;

    TS_ASSERT_EQUALS("methodname", alg.workspaceMethodName());
    auto types = alg.workspaceMethodOn();
    TS_ASSERT_EQUALS(2, types.size());
    if (types.size() == 2) {
      TS_ASSERT_EQUALS("MatrixWorkspace", types[0]);
      TS_ASSERT_EQUALS("ITableWorkspace", types[1]);
    }
    TS_ASSERT_EQUALS("InputWorkspace", alg.workspaceMethodInputProperty());
  }

  void testStringization() {
    // Set the properties so that we know what they are
    alg.setPropertyValue("prop1", "value1");
    alg.setProperty("prop2", 5);
    std::string expected =
        "{\"name\":\"ToyAlgorithm\",\"properties\":{\"prop1\":\"value1\",\"prop2\":5},\"version\":1}";
    TS_ASSERT_EQUALS(alg.toString(), expected);
  }

  void test_From_String_With_Invalid_Input_Throws() {
    const std::string input = "()";
    TS_ASSERT_THROWS(Algorithm::fromString(input), const std::runtime_error &);
  }

  void test_Construction_Via_Valid_String_With_No_Properties() {
    auto testAlg = runFromString(R"({"name":"ToyAlgorithm"})");
    TS_ASSERT_EQUALS(testAlg->name(), "ToyAlgorithm");
    TS_ASSERT_EQUALS(testAlg->version(), 2);
  }

  void test_Construction_Via_Valid_String_With_Version() {
    auto testAlg = runFromString("{\"name\":\"ToyAlgorithm\","
                                 "\"version\":1}");
    TS_ASSERT_EQUALS(testAlg->name(), "ToyAlgorithm");
    TS_ASSERT_EQUALS(testAlg->version(), 1);
  }

  void test_Construction_Via_Valid_String_With_Version_And_Empty_Props() {
    auto testAlg = runFromString("{\"name\":\"ToyAlgorithm\",\"properties\":{"
                                 "},\"version\":1}\n");
    TS_ASSERT_EQUALS(testAlg->name(), "ToyAlgorithm");
    TS_ASSERT_EQUALS(testAlg->version(), 1);
  }

  void test_Construction_Via_Valid_String_With_Set_Properties_And_Version() {

    auto testAlg = runFromString("{\"name\":\"ToyAlgorithm\",\"properties\":{\"Binning\":"
                                 "\"0.2,0.2,1.4\",\"prop1\":\"val1\",\"prop2\":\"8\","
                                 "\"prop3\":\"10\"},\"version\":2}\n");
    TS_ASSERT_EQUALS(testAlg->name(), "ToyAlgorithm");
    TS_ASSERT_EQUALS(testAlg->version(), 2);

    // On gcc we get ambiguous function calls doing
    // std::string s;
    // s = getProperty(...);
    // so we have to do this
    try {
      std::string prop1 = testAlg->getProperty("prop1");
      TS_ASSERT_EQUALS(prop1, "val1");
    } catch (...) {
      TS_FAIL("Cannot retrieve property 'prop1'");
    }
    try {
      int prop2 = testAlg->getProperty("prop2");
      TS_ASSERT_EQUALS(prop2, 8);
    } catch (...) {
      TS_FAIL("Cannot retrieve property 'prop2'");
    }
    try {
      double prop3 = testAlg->getProperty("prop3");
      TS_ASSERT_EQUALS(prop3, 10.0);
    } catch (...) {
      TS_FAIL("Cannot retrieve property 'prop3'");
    }
    try {
      std::vector<double> prop3 = testAlg->getProperty("Binning");
      TS_ASSERT_EQUALS(prop3.size(), 3);
    } catch (...) {
      TS_FAIL("Cannot retrieve property 'Binning'");
    }
  }

  void test_Construction_Via_Valid_String_With_Single_Property_And_Version() {
    auto testAlg = runFromString("{\"name\":\"ToyAlgorithm\",\"properties\":{"
                                 "\"prop3\":\"10.0\"},\"version\":2}\n");
    TS_ASSERT_EQUALS(testAlg->name(), "ToyAlgorithm");
    TS_ASSERT_EQUALS(testAlg->version(), 2);

    try {
      double prop3 = testAlg->getProperty("prop3");
      TS_ASSERT_EQUALS(prop3, 10.0);
    } catch (...) {
      TS_FAIL("Cannot retrieve property 'prop3'");
    }
    try {
      std::vector<double> prop3 = testAlg->getProperty("Binning");
      TS_ASSERT_EQUALS(prop3.size(), 3);
    } catch (...) {
      TS_FAIL("Cannot retrieve property 'Binning'");
    }
  }

  void test_Construction_Via_Valid_String_With_Single_Property_Array() {
    auto testAlg = runFromString("{\"name\":\"ToyAlgorithm\",\"properties\":{"
                                 "\"Binning\":\"0.2,0.2,1.4\"},\"version\":2}\n");
    TS_ASSERT_EQUALS(testAlg->name(), "ToyAlgorithm");
    TS_ASSERT_EQUALS(testAlg->version(), 2);

    try {
      std::vector<double> prop3 = testAlg->getProperty("Binning");
      TS_ASSERT_EQUALS(prop3.size(), 3);
      TS_ASSERT_EQUALS(prop3[2], 1.4);
    } catch (...) {
      TS_FAIL("Cannot retrieve property 'Binning'");
    }
  }

  void test_Construction_Via_Valid_String_With_Empty_Properties() {
    auto testAlg = runFromString(("{\"name\":\"ToyAlgorithm\",\"properties\":{}}\n"));
    TS_ASSERT_EQUALS(testAlg->name(), "ToyAlgorithm");
    TS_ASSERT_EQUALS(testAlg->version(), 2);
    try {
      std::string prop1 = testAlg->getProperty("prop1");
      TS_ASSERT_EQUALS(prop1, "value");
    } catch (...) {
      TS_FAIL("Cannot retrieve property 'prop1'");
    }
    try {
      int prop2 = testAlg->getProperty("prop2");
      TS_ASSERT_EQUALS(prop2, 1);
    } catch (...) {
      TS_FAIL("Cannot retrieve property 'prop2'");
    }
    try {
      std::vector<double> prop3 = testAlg->getProperty("Binning");
      TS_ASSERT_EQUALS(prop3.size(), 3);
    } catch (...) {
      TS_FAIL("Cannot retrieve property 'Binning'");
    }
  }

  //------------------------------------------------------------------------
  /** Test of setting read and/or write locks
   * for various combinations of input/output workspaces.
   */
  void do_test_locking(const std::string &in1, const std::string &in2, const std::string &inout,
                       const std::string &out1, const std::string &out2) {
    for (size_t i = 0; i < 6; i++) {
      std::shared_ptr<WorkspaceTester> ws = std::make_shared<WorkspaceTester>();
      AnalysisDataService::Instance().addOrReplace("ws" + Strings::toString(i), ws);
    }
    StubbedWorkspaceAlgorithm alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace1", in1);
    alg.setPropertyValue("InputWorkspace2", in2);
    alg.setPropertyValue("InOutWorkspace", inout);
    alg.setPropertyValue("OutputWorkspace1", out1);
    alg.setPropertyValue("OutputWorkspace2", out2);
    // This throws or hangs if the code is wrong
    alg.execute();
  }

  //------------------------------------------------------------------------
  void test_lockingWorkspaces() {
    // Input and output are different
    do_test_locking("ws0", "", "", "ws1", "");
    // Repeated output workspaces
    do_test_locking("ws0", "", "", "ws1", "ws1");
    // Different output workspaces
    do_test_locking("ws0", "", "", "ws1", "ws2");
    // Input and output are same
    do_test_locking("ws0", "", "", "ws0", "");
    // Two input workspaces
    do_test_locking("ws0", "ws0", "", "ws5", "");
    // Also in-out workspace
    do_test_locking("ws0", "ws0", "ws0", "ws0", "");
    // All the same
    do_test_locking("ws0", "ws0", "ws0", "ws0", "ws0");
  }

  /** Have a workspace property that does NOT lock the workspace.
   * The failure mode of this test is HANGING. */
  void test_workspace_notLocking() {
    std::shared_ptr<WorkspaceTester> ws1 = std::make_shared<WorkspaceTester>();
    AnalysisDataService::Instance().addOrReplace("ws1", ws1);

    {
      // Get a write lock.
      WriteLock _lock(*ws1);
      // The algorithm would hang waiting for the write-lock to release if the
      // property were locking.
      StubbedWorkspaceAlgorithm2 alg;
      alg.initialize();
      alg.setPropertyValue("NonLockingInputWorkspace", "ws1");
      alg.execute();
      TS_ASSERT(alg.isExecuted());
    }
    {
      // Acquire a scoped read-lock on ws1.
      ReadLock _lock(*ws1);
      // The algo would lock up when trying to WRITE-lock the workspace again
      StubbedWorkspaceAlgorithm2 alg;
      alg.initialize();
      alg.setPropertyValue("NonLockingOutputWorkspace", "ws1");
      alg.execute();
      TS_ASSERT(alg.isExecuted());
    }
  }

  void test_Algorithm_Drops_Workspace_References_When_Stored_In_ADS() {
    // create an input workspace, add it to the ADS
    auto inputWorkspace = std::make_shared<WorkspaceTester>();
    const std::string inputName("testIn"), outputName("testOut");
    auto &ads = AnalysisDataService::Instance();
    ads.addOrReplace(inputName, inputWorkspace);

    auto workspaceAlg = std::make_unique<StubbedWorkspaceAlgorithm>();
    workspaceAlg->initialize();
    workspaceAlg->setProperty("InputWorkspace1", inputName);
    workspaceAlg->setProperty("OutputWorkspace1", outputName);
    workspaceAlg->execute();

    // The input workspace should have references from the local inputWorkspace
    // variable and in the ADS but nothing else
    TS_ASSERT_EQUALS(2, inputWorkspace.use_count());

    // dropping algorithm shouldn't alter the use count
    workspaceAlg.reset();
    TS_ASSERT_EQUALS(2, inputWorkspace.use_count());

    // drop ADS reference and left with local
    ads.remove(inputName);
    TS_ASSERT_EQUALS(1, inputWorkspace.use_count());
  }

  void test_Algorithm_Keeps_Only_WorkspaceProperty_Ref_If_Not_Stored_In_ADS() {
    // create an input workspace, add it to the ADS
    auto inputWorkspace = std::make_shared<WorkspaceTester>();
    const std::string inputName("testIn"), outputName("testOut");

    auto workspaceAlg = std::make_unique<StubbedWorkspaceAlgorithm>();
    workspaceAlg->initialize();
    workspaceAlg->setAlwaysStoreInADS(false);
    workspaceAlg->setProperty("InputWorkspace1", inputWorkspace);
    workspaceAlg->setProperty("OutputWorkspace1", outputName);
    workspaceAlg->execute();

    // The input workspace should have references from the algorithm
    // and the local variable
    TS_ASSERT_EQUALS(2, inputWorkspace.use_count());

    // dropping algorithm should leave the local variable
    workspaceAlg.reset();
    TS_ASSERT_EQUALS(1, inputWorkspace.use_count());
  }

  //------------------------------------------------------------------------
  /** Make a workspace group with:
   *
   * @param group1 :: name of the group. Do nothing if blank.
   * @param contents1 :: comma-sep names of fake workspaces in the group
   *        Make no group if blank, just 1 workspace
   * @return The new WorkspaceGroup object
   */
  Workspace_sptr makeWorkspaceGroup(const std::string &group1, std::string contents1) {
    auto &ads = AnalysisDataService::Instance();
    if (contents1.empty()) {
      if (group1.empty())
        return Workspace_sptr();
      auto ws = std::make_shared<WorkspaceTester>();
      ads.addOrReplace(group1, ws);
      return ws;
    }

    std::vector<std::string> names;
    boost::split(names, contents1, boost::algorithm::detail::is_any_ofF<char>(","));
    if (names.size() >= 1) {
      auto wsGroup = WorkspaceGroup_sptr(new WorkspaceGroup());
      ads.addOrReplace(group1, wsGroup);
      for (const auto &name : names) {
        auto ws = std::make_shared<WorkspaceTester>();
        ws->initialize(10, 10, 10);
        ads.addOrReplace(name, ws);
        wsGroup->add(name);
      }
      return wsGroup;
    }
    return Workspace_sptr();
  }

  //------------------------------------------------------------------------
  WorkspaceGroup_sptr do_test_groups(const std::string &group1, std::string contents1, const std::string &group2,
                                     std::string contents2, const std::string &group3, std::string contents3,
                                     bool expectFail = false, int expectedNumber = 3) {
    makeWorkspaceGroup(group1, std::move(contents1));
    makeWorkspaceGroup(group2, std::move(contents2));
    makeWorkspaceGroup(group3, std::move(contents3));

    StubbedWorkspaceAlgorithm alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace1", group1);
    alg.setPropertyValue("InputWorkspace2", group2);
    alg.setPropertyValue("InOutWorkspace", group3);
    alg.setPropertyValue("Number", "234");
    alg.setPropertyValue("OutputWorkspace1", "D");
    alg.setPropertyValue("OutputWorkspace2", "E");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    if (expectFail) {
      TS_ASSERT(!alg.isExecuted());
      return WorkspaceGroup_sptr();
    }
    TS_ASSERT(alg.isExecuted())
    Workspace_sptr out1 = AnalysisDataService::Instance().retrieve("D");
    WorkspaceGroup_sptr group = std::dynamic_pointer_cast<WorkspaceGroup>(out1);

    TS_ASSERT_EQUALS(group->getName(), "D")
    TS_ASSERT_EQUALS(group->getNumberOfEntries(), expectedNumber)
    if (group->getNumberOfEntries() < 1)
      return group;
    ws1 = std::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(0));
    if (group->getNumberOfEntries() < 2)
      return group;
    ws2 = std::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(1));
    if (group->getNumberOfEntries() < 3)
      return group;
    ws3 = std::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(2));
    return group;
  }

  void test_processGroups_failures() {
    // Fails due to unequal sizes.
    do_test_groups("A", "A_1,A_2,A_3", "B", "B_1,B_2,B_3,B_4", "", "", true /*fails*/);
  }

  /// All groups are the same size
  void test_processGroups_allSameSize() {
    WorkspaceGroup_sptr group = do_test_groups("A", "A_1,A_2,A_3", "B", "B_1,B_2,B_3", "C", "C_1,C_2,C_3");

    TS_ASSERT_EQUALS(ws1->getName(), "D_1");
    TS_ASSERT_EQUALS(ws1->getTitle(), "A_1+B_1+C_1");
    TS_ASSERT_EQUALS(ws1->readY(0)[0], 234);
    TS_ASSERT_EQUALS(ws2->getName(), "D_2");
    TS_ASSERT_EQUALS(ws2->getTitle(), "A_2+B_2+C_2");
    TS_ASSERT_EQUALS(ws3->getName(), "D_3");
    TS_ASSERT_EQUALS(ws3->getTitle(), "A_3+B_3+C_3");
  }

  /// All groups are the same size, but they don't all match the rigid naming
  void test_processGroups_allSameSize_namesNotSimilar() {
    WorkspaceGroup_sptr group = do_test_groups("A", "A_1,A_2,A_3", "B", "B_1,B_2,B_3", "C", "alice,bob,charlie");

    TS_ASSERT_EQUALS(ws1->getName(), "A_1_B_1_alice_D");
    TS_ASSERT_EQUALS(ws1->getTitle(), "A_1+B_1+alice");
    TS_ASSERT_EQUALS(ws1->readY(0)[0], 234);
    TS_ASSERT_EQUALS(ws2->getName(), "A_2_B_2_bob_D");
    TS_ASSERT_EQUALS(ws2->getTitle(), "A_2+B_2+bob");
    TS_ASSERT_EQUALS(ws3->getName(), "A_3_B_3_charlie_D");
    TS_ASSERT_EQUALS(ws3->getTitle(), "A_3+B_3+charlie");
  }

  /// One input is a group, rest are singles
  void test_processGroups_onlyOneGroup() {
    WorkspaceGroup_sptr group = do_test_groups("A", "A_1,A_2,A_3", "B", "", "C", "");

    TS_ASSERT_EQUALS(ws1->getName(), "D_1");
    TS_ASSERT_EQUALS(ws1->getTitle(), "A_1+B+C");
    TS_ASSERT_EQUALS(ws1->readY(0)[0], 234);
    TS_ASSERT_EQUALS(ws2->getName(), "D_2");
    TS_ASSERT_EQUALS(ws2->getTitle(), "A_2+B+C");
    TS_ASSERT_EQUALS(ws3->getName(), "D_3");
    TS_ASSERT_EQUALS(ws3->getTitle(), "A_3+B+C");
  }

  /// One optional WorkspaceProperty is not specified
  void test_processGroups_optionalInput() {
    WorkspaceGroup_sptr group = do_test_groups("A", "A_1,A_2,A_3", "B", "", "", "");

    TS_ASSERT_EQUALS(ws1->getName(), "D_1");
    TS_ASSERT_EQUALS(ws1->getTitle(), "A_1+B+");
    TS_ASSERT_EQUALS(ws1->readY(0)[0], 234);
    TS_ASSERT_EQUALS(ws2->getName(), "D_2");
    TS_ASSERT_EQUALS(ws2->getTitle(), "A_2+B+");
    TS_ASSERT_EQUALS(ws3->getName(), "D_3");
    TS_ASSERT_EQUALS(ws3->getTitle(), "A_3+B+");
  }

  /// One optional WorkspaceProperty is not specified
  void test_processGroups_twoGroups_and_optionalInput() {
    WorkspaceGroup_sptr group = do_test_groups("A", "A_1,A_2,A_3", "", "", "C", "C_1,C_2,C_3");

    TS_ASSERT_EQUALS(ws1->getName(), "D_1");
    TS_ASSERT_EQUALS(ws1->getTitle(), "A_1++C_1");
    TS_ASSERT_EQUALS(ws1->readY(0)[0], 234);
    TS_ASSERT_EQUALS(ws2->getName(), "D_2");
    TS_ASSERT_EQUALS(ws2->getTitle(), "A_2++C_2");
    TS_ASSERT_EQUALS(ws3->getName(), "D_3");
    TS_ASSERT_EQUALS(ws3->getTitle(), "A_3++C_3");
  }

  /// One input is a group with only one member (not possible via GUI)
  void test_processGroups_onlyOneGroup_withOnlyOneMember() {
    WorkspaceGroup_sptr group = do_test_groups("A", "A_1", "B", "", "C", "", false, 1);

    TS_ASSERT_EQUALS(ws1->getName(), "D_1");
    TS_ASSERT_EQUALS(ws1->getTitle(), "A_1+B+C");
    TS_ASSERT_EQUALS(ws1->readY(0)[0], 234);
  }

  /// Two inputs are groups with one member (each)
  void test_processGroups_twoGroup_withOnlyOneMember() {
    WorkspaceGroup_sptr group = do_test_groups("A", "A_1", "B", "B_1", "C", "", false, 1);

    TS_ASSERT_EQUALS(ws1->getName(), "D_1");
    TS_ASSERT_EQUALS(ws1->getTitle(), "A_1+B_1+C");
    TS_ASSERT_EQUALS(ws1->readY(0)[0], 234);
  }

  void test_processGroups_failOnGroupMemberErrorMessage() {
    makeWorkspaceGroup("A", "A_1,A_2,A_3");

    FailingAlgorithm alg;
    alg.initialize();
    alg.setRethrows(true);
    alg.setLogging(false);
    alg.setPropertyValue("InputWorkspace", "A");
    alg.setPropertyValue("WsNameToFail", "A_2");

    try {
      alg.execute();
      TS_FAIL("Exception wasn't thrown");
    } catch (std::runtime_error &e) {
      std::string msg(e.what());

      TSM_ASSERT("Error message should contain original error",
                 msg.find(FailingAlgorithm::FAIL_MSG) != std::string::npos);
    }
  }

  /// Rewrite first input group
  void test_processGroups_rewriteFirstGroup() {
    WorkspaceGroup_sptr group = do_test_groups("D", "D1,D2,D3", "B", "B1,B2,B3", "C", "C1,C2,C3");

    TS_ASSERT_EQUALS(ws1->getName(), "D1");
    TS_ASSERT_EQUALS(ws1->getTitle(), "D1+B1+C1");
    TS_ASSERT_EQUALS(ws1->readY(0)[0], 234);
    TS_ASSERT_EQUALS(ws2->getName(), "D2");
    TS_ASSERT_EQUALS(ws2->getTitle(), "D2+B2+C2");
    TS_ASSERT_EQUALS(ws3->getName(), "D3");
    TS_ASSERT_EQUALS(ws3->getTitle(), "D3+B3+C3");
  }

  /// Rewrite second group
  void test_processGroups_rewriteSecondGroup() {
    WorkspaceGroup_sptr group = do_test_groups("A", "A1,A2,A3", "D", "D1,D2,D3", "C", "C1,C2,C3");

    TS_ASSERT_EQUALS(ws1->getName(), "D1");
    TS_ASSERT_EQUALS(ws1->getTitle(), "A1+D1+C1");
    TS_ASSERT_EQUALS(ws1->readY(0)[0], 234);
    TS_ASSERT_EQUALS(ws2->getName(), "D2");
    TS_ASSERT_EQUALS(ws2->getTitle(), "A2+D2+C2");
    TS_ASSERT_EQUALS(ws3->getName(), "D3");
    TS_ASSERT_EQUALS(ws3->getTitle(), "A3+D3+C3");
  }

  /// Rewrite multiple group
  void test_processGroups_rewriteMultipleGroup() {
    WorkspaceGroup_sptr group = do_test_groups("A", "A1,A2,A3", "D", "D1,D2,D3", "D", "D1,D2,D3");

    TS_ASSERT_EQUALS(ws1->getName(), "D1");
    TS_ASSERT_EQUALS(ws1->getTitle(), "A1+D1+D1");
    TS_ASSERT_EQUALS(ws1->readY(0)[0], 234);
    TS_ASSERT_EQUALS(ws2->getName(), "D2");
    TS_ASSERT_EQUALS(ws2->getTitle(), "A2+D2+D2");
    TS_ASSERT_EQUALS(ws3->getName(), "D3");
    TS_ASSERT_EQUALS(ws3->getTitle(), "A3+D3+D3");
  }

  void doHistoryCopyTest(const std::string &inputWSName, const std::string &outputWSName) {
    auto inputWS = std::make_shared<WorkspaceTester>();
    inputWS->history().addHistory(
        std::make_shared<AlgorithmHistory>("Load", 1, "b5b65a94-e656-468e-987c-644288fac655"));
    auto &ads = AnalysisDataService::Instance();
    ads.addOrReplace(inputWSName, inputWS);

    StubbedWorkspaceAlgorithm nextStep;
    nextStep.initialize();
    nextStep.setPropertyValue("InputWorkspace1", inputWSName);
    nextStep.setPropertyValue("OutputWorkspace1", outputWSName);
    nextStep.execute();

    auto outputWS = ads.retrieve(outputWSName);
    const auto &outputHistory = outputWS->history();
    TS_ASSERT_EQUALS(2, outputHistory.size());
    TS_ASSERT_EQUALS("Load", outputHistory.getAlgorithmHistory(0)->name());
    TS_ASSERT_EQUALS("StubbedWorkspaceAlgorithm", outputHistory.getAlgorithmHistory(1)->name());
  }

  void test_singleInputWorkspaceHistoryCopiedToOutputWorkspace() {
    doHistoryCopyTest("copyHistoryIn", "copyHistoryOut");
  }

  void test_singleInputWorkspaceHistoryCopiedToReplacedOutputWorkspace() {
    doHistoryCopyTest("copyHistoryInOut", "copyHistoryInOut");
  }

  void doHistoryCopyOnGroupsTest(const std::string &inputWSName, const std::string &outputWSName) {
    using Mantid::Types::Core::DateAndTime;
    const auto group = std::dynamic_pointer_cast<WorkspaceGroup>(
        makeWorkspaceGroup(inputWSName, inputWSName + "_1," + inputWSName + "_2"));
    const DateAndTime execDate{Mantid::Types::Core::DateAndTime::getCurrentTime()};
    for (auto &item : *group) {
      item->history().addHistory(
          std::make_shared<AlgorithmHistory>("Load", 1, "49ea7cb9-6172-4e5c-acf5-c3edccd0bb27", execDate));
    }
    auto &ads = AnalysisDataService::Instance();
    StubbedWorkspaceAlgorithm nextStep;
    nextStep.initialize();
    nextStep.setPropertyValue("InputWorkspace1", inputWSName);
    nextStep.setPropertyValue("OutputWorkspace1", outputWSName);
    nextStep.execute();

    auto outputGroup = ads.retrieveWS<WorkspaceGroup>(outputWSName);
    TS_ASSERT(outputGroup);
    for (auto &item : *outputGroup) {
      const auto &outputHistory = item->history();
      TS_ASSERT_EQUALS(2, outputHistory.size());
      TS_ASSERT_EQUALS("Load", outputHistory.getAlgorithmHistory(0)->name());
      TS_ASSERT_EQUALS("StubbedWorkspaceAlgorithm", outputHistory.getAlgorithmHistory(1)->name());
    }
  }

  void test_InputWorkspaceGroupHistoryCopiedToOutputWorkspaceGroup() {
    doHistoryCopyOnGroupsTest("copyHistoryGroupIn", "copyHistoryGroupOut");
  }

  void test_InputWorkspaceGroupHistoryCopiedToReplacedOutputWorkspaceGroup() {
    doHistoryCopyOnGroupsTest("copyHistoryGroupInOut", "copyHistoryGroupInOut");
  }

  /**
   * Test declaring an algorithm property and retrieving as const
   * and non-const
   */
  void testGetProperty_const_sptr() {
    const std::string algName = "InputAlgorithm";
    IAlgorithm_sptr algInput(new StubbedWorkspaceAlgorithm());
    PropertyManagerHelper manager;
    manager.declareProperty(algName, algInput, Mantid::Kernel::Direction::Input);

    // Check property can be obtained as const or non-const sptr
    IAlgorithm_const_sptr algConst;
    IAlgorithm_sptr algNonConst;
    TS_ASSERT_THROWS_NOTHING(algConst = manager.getValue<IAlgorithm_const_sptr>(algName));
    TS_ASSERT(algConst != nullptr);
    TS_ASSERT_THROWS_NOTHING(algNonConst = manager.getValue<IAlgorithm_sptr>(algName));
    TS_ASSERT(algNonConst != nullptr);
    TS_ASSERT_EQUALS(algConst, algNonConst);

    // Check TypedValue can be cast to const_sptr or to sptr
    PropertyManagerHelper::TypedValue val(manager, algName);
    IAlgorithm_const_sptr algCastConst;
    IAlgorithm_sptr algCastNonConst;
    TS_ASSERT_THROWS_NOTHING(algCastConst = (IAlgorithm_const_sptr)val);
    TS_ASSERT(algCastConst != nullptr);
    TS_ASSERT_THROWS_NOTHING(algCastNonConst = (IAlgorithm_sptr)val);
    TS_ASSERT(algCastNonConst != nullptr);
    TS_ASSERT_EQUALS(algCastConst, algCastNonConst);
  }

  void testIndexingAlgorithm_declareWorkspaceInputPropertiesMethod() {
    IndexingAlgorithm indexAlg;
    TS_ASSERT_THROWS_NOTHING(indexAlg.init());
  }

  void testIndexingAlgorithm_setWorkspaceInputPropertiesWithWorkspacePointerAndVectorOfIntegers() {
    auto wksp = WorkspaceFactory::Instance().create("WorkspaceTester", 10, 10, 9);
    IndexingAlgorithm indexAlg;
    indexAlg.init();
    TS_ASSERT_THROWS_NOTHING((indexAlg.setWorkspaceInputProperties("InputWorkspace", wksp, IndexType::WorkspaceIndex,
                                                                   std::vector<int64_t>{1, 2, 3, 4, 5})));
  }

  void testIndexingAlgorithm_setWorkspaceInputPropertiesWithWorkspacePointerAndStringList() {
    auto wksp = WorkspaceFactory::Instance().create("WorkspaceTester", 10, 10, 9);
    IndexingAlgorithm indexAlg;
    indexAlg.init();
    TS_ASSERT_THROWS_NOTHING((indexAlg.setWorkspaceInputProperties<MatrixWorkspace, std::string>(
        "InputWorkspace", wksp, IndexType::WorkspaceIndex, "1:5")));
  }

  void testIndexingAlgorithm_setWorkspaceInputPropertiesWithWorkspaceNameAndVectorOfIntegers() {
    auto wksp = WorkspaceFactory::Instance().create("WorkspaceTester", 10, 10, 9);
    AnalysisDataService::Instance().add("wksp", wksp);
    IndexingAlgorithm indexAlg;
    indexAlg.init();
    // Requires workspace in ADS due to validity checks
    TS_ASSERT_THROWS_NOTHING((indexAlg.setWorkspaceInputProperties<MatrixWorkspace>(
        "InputWorkspace", "wksp", IndexType::WorkspaceIndex, std::vector<int64_t>{1, 2, 3, 4, 5})));
    AnalysisDataService::Instance().remove("wksp");
  }

  void testIndexingAlgorithm_setWorkspaceInputPropertiesWithWorkspaceNameAndStringList() {
    auto wksp = WorkspaceFactory::Instance().create("WorkspaceTester", 10, 10, 9);
    AnalysisDataService::Instance().add("wksp", wksp);
    IndexingAlgorithm indexAlg;
    indexAlg.init();
    // Requires workspace in ADS due to validity checks
    TS_ASSERT_THROWS_NOTHING((indexAlg.setWorkspaceInputProperties<MatrixWorkspace, std::string>(
        "InputWorkspace", "wksp", IndexType::WorkspaceIndex, "1:5")));
    AnalysisDataService::Instance().remove("wksp");
  }

  void testIndexingAlgorithm_getWorkspaceAndIndicesMethod() {
    IndexingAlgorithm indexAlg;
    indexAlg.init();
    auto wksp = WorkspaceFactory::Instance().create("WorkspaceTester", 10, 10, 9);
    indexAlg.setWorkspaceInputProperties<MatrixWorkspace, std::string>("InputWorkspace", wksp,
                                                                       IndexType::WorkspaceIndex, "1:5");

    MatrixWorkspace_sptr wsTest;
    SpectrumIndexSet indexSet;

    TS_ASSERT_THROWS_NOTHING(std::tie(wsTest, indexSet) =
                                 indexAlg.getWorkspaceAndIndices<MatrixWorkspace>("InputWorkspace"));

    TS_ASSERT_EQUALS(wsTest, wksp);

    for (size_t i = 0; i < indexSet.size(); i++)
      TS_ASSERT_EQUALS(indexSet[i], i + 1);
  }

  void testIndexingAlgorithm_accessFailInvalidPropertyType() {
    IndexingAlgorithm indexAlg;

    TS_ASSERT_THROWS(indexAlg.getWorkspaceAndIndices<MatrixWorkspace>("InputWorkspace2"), const std::runtime_error &);
    TS_ASSERT_THROWS((indexAlg.setWorkspaceInputProperties<MatrixWorkspace, std::string>(
                         "InputWorkspace2", "wksp", IndexType::SpectrumNum, "1:5")),
                     const std::runtime_error &);
  }

  void testIndexingAlgorithm_failExistingIndexProperty() {
    IndexingAlgorithm indexAlg;
    indexAlg.init();
    TS_ASSERT_THROWS(indexAlg.declareProperty(
                         std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input)),
                     const std::runtime_error &);
  }

private:
  IAlgorithm_sptr runFromString(const std::string &input) {
    IAlgorithm_sptr testAlg;
    TS_ASSERT_THROWS_NOTHING(testAlg = Algorithm::fromString(input));
    TS_ASSERT(testAlg);
    if (!testAlg)
      TS_FAIL("Failed to create algorithm, cannot continue test.");
    return testAlg;
  }

  ToyAlgorithm alg;
  ToyAlgorithmTwo algv2;
  ToyAlgorithmThree algv3;

  MatrixWorkspace_sptr ws1;
  MatrixWorkspace_sptr ws2;
  MatrixWorkspace_sptr ws3;
};
