// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/MultiThreaded.h"
#include <Poco/ActiveResult.h>
#include <Poco/Thread.h>
#include <stdexcept>
#include <vector>

using namespace Mantid::API;

class AlgTest : public Algorithm {
public:
  AlgTest() : Algorithm() {}
  ~AlgTest() override = default;
  void init() override {}
  void exec() override {}
  const std::string name() const override { return "AlgTest"; }
  int version() const override { return (1); }
  const std::string category() const override { return ("Cat1"); }
  const std::string summary() const override { return "Test summary"; }
};

class AlgTestFail : public Algorithm {
public:
  AlgTestFail() : Algorithm() {}
  ~AlgTestFail() override = default;
  void init() override {}
  void exec() override {}
  const std::string name() const override { return "AlgTest"; }
  int version() const override { return (1); }
  const std::string category() const override { return ("Cat2"); }
  const std::string summary() const override { return "Test summary"; }
};

class AlgTestPass : public Algorithm {
public:
  AlgTestPass() : Algorithm() {}
  ~AlgTestPass() override = default;
  void init() override {}
  void exec() override {}
  const std::string name() const override { return "AlgTest"; }
  int version() const override { return (2); }
  const std::string category() const override { return ("Cat4"); }
  const std::string summary() const override { return "Test summary"; }
};

class AlgTestSecond : public Algorithm {
public:
  AlgTestSecond() : Algorithm() {}
  ~AlgTestSecond() override = default;
  void init() override {}
  void exec() override {}
  const std::string name() const override { return "AlgTestSecond"; }
  int version() const override { return (1); }
  const std::string category() const override { return ("Cat3"); }
  const std::string summary() const override { return "Test summary"; }
};

/** Algorithm that always says it's running if asked */
class AlgRunsForever : public Algorithm {
private:
  bool isRunningFlag;

public:
  AlgRunsForever() : Algorithm(), isRunningFlag(true) {}
  ~AlgRunsForever() override = default;
  void init() override {}
  void exec() override {}
  const std::string name() const override { return "AlgRunsForever"; }
  int version() const override { return (1); }
  const std::string category() const override { return ("Cat1"); }
  const std::string summary() const override { return "Test summary"; }
  // Override methods so we can manipulate whether it appears to be running
  ExecutionState executionState() const override {
    return isRunningFlag ? ExecutionState::Running : ExecutionState::Finished;
  }
  ResultState resultState() const override { return isRunningFlag ? ResultState::NotFinished : ResultState::Failed; }
  void setIsRunningTo(bool runningFlag) { isRunningFlag = runningFlag; }
  void cancel() override { isRunningFlag = false; }
};

DECLARE_ALGORITHM(AlgTest)
DECLARE_ALGORITHM(AlgRunsForever)
DECLARE_ALGORITHM(AlgTestSecond)

class AlgorithmManagerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AlgorithmManagerTest *createSuite() { return new AlgorithmManagerTest(); }
  static void destroySuite(AlgorithmManagerTest *suite) { delete suite; }

  void testVersionFail() {
    const size_t nalgs = AlgorithmFactory::Instance().getKeys().size();
    TS_ASSERT_THROWS(AlgorithmFactory::Instance().subscribe<AlgTestFail>(), const std::runtime_error &);
    // Size should be the same
    TS_ASSERT_EQUALS(AlgorithmFactory::Instance().getKeys().size(), nalgs);
  }

  void testVersionPass() { TS_ASSERT_THROWS_NOTHING(AlgorithmFactory::Instance().subscribe<AlgTestPass>()); }

  void testInstance() {
    TS_ASSERT_THROWS_NOTHING(AlgorithmManager::Instance().create("AlgTest"));
    TS_ASSERT_THROWS(AlgorithmManager::Instance().create("AlgTest", 3), const std::runtime_error &);
    TS_ASSERT_THROWS(AlgorithmManager::Instance().create("aaaaaa"), const std::runtime_error &);
  }

  void testClear() {
    AlgorithmManager::Instance().clear();
    TS_ASSERT_THROWS_NOTHING(AlgorithmManager::Instance().create("AlgTest"));
    TS_ASSERT_THROWS_NOTHING(AlgorithmManager::Instance().create("AlgTestSecond"));
    TS_ASSERT_EQUALS(AlgorithmManager::Instance().size(), 2);
    AlgorithmManager::Instance().clear();
    TS_ASSERT_EQUALS(AlgorithmManager::Instance().size(), 0);
  }

  void testReturnType() {
    AlgorithmManager::Instance().clear();
    IAlgorithm_sptr alg;
    TS_ASSERT_THROWS_NOTHING(alg = AlgorithmManager::Instance().create("AlgTest", 1));
    TS_ASSERT_DIFFERS(dynamic_cast<Algorithm *>(alg.get()), static_cast<Algorithm *>(nullptr));
    TS_ASSERT_THROWS_NOTHING(alg = AlgorithmManager::Instance().create("AlgTestSecond", 1));
    TS_ASSERT_DIFFERS(dynamic_cast<Algorithm *>(alg.get()), static_cast<Algorithm *>(nullptr));
    TS_ASSERT_DIFFERS(dynamic_cast<IAlgorithm *>(alg.get()), static_cast<IAlgorithm *>(nullptr));
    TS_ASSERT_EQUALS(AlgorithmManager::Instance().size(),
                     2); // To check that crea is called on local objects
  }

  void testManagedType() {
    AlgorithmManager::Instance().clear();
    IAlgorithm_sptr Aptr, Bptr;
    Aptr = AlgorithmManager::Instance().create("AlgTest");
    Bptr = AlgorithmManager::Instance().createUnmanaged("AlgTest");
    TS_ASSERT_DIFFERS(Aptr, Bptr);
    TS_ASSERT_EQUALS(AlgorithmManager::Instance().size(), 1);
    TS_ASSERT_DIFFERS(Aptr.get(), static_cast<Algorithm *>(nullptr));
    TS_ASSERT_DIFFERS(Bptr.get(), static_cast<Algorithm *>(nullptr));
  }

  // This will be called back when an algo starts
  void handleAlgorithmStartingNotification(const Poco::AutoPtr<AlgorithmStartingNotification> & /*pNf*/) {
    m_notificationValue = 12345;
  }

  /** When running an algorithm in async mode, the
   * AlgorithmManager needs to send out a notification
   */
  void testStartingNotification() {
    AlgorithmManager::Instance().clear();
    Poco::NObserver<AlgorithmManagerTest, Mantid::API::AlgorithmStartingNotification> my_observer(
        *this, &AlgorithmManagerTest::handleAlgorithmStartingNotification);
    AlgorithmManager::Instance().notificationCenter.addObserver(my_observer);

    IAlgorithm_sptr Aptr, Bptr;
    Aptr = AlgorithmManager::Instance().create("AlgTest", -1);

    m_notificationValue = 0;
    Poco::ActiveResult<bool> resA = Aptr->executeAsync();
    resA.wait();
    TSM_ASSERT_EQUALS("Notification was received.", m_notificationValue, 12345);

    AlgorithmManager::Instance().notificationCenter.removeObserver(my_observer);
  }

  void testThreadSafety() {
    PARALLEL_FOR_NO_WSP_CHECK()
    for (int i = 0; i < 5000; i++) {
      AlgorithmManager::Instance().create("AlgTest");
    }
  }

  void testRemovingByIdRemovesCorrectObject() {
    auto &mgr = AlgorithmManager::Instance();
    const size_t initialManagerSize = mgr.size();
    // 2 different ids for same named algorithm
    auto alg1 = mgr.create("AlgTest");
    auto alg2 = mgr.create("AlgTest");
    TS_ASSERT_EQUALS(initialManagerSize + 2, mgr.size());

    TS_ASSERT_THROWS_NOTHING(mgr.removeById(alg1->getAlgorithmID()));
    TS_ASSERT_EQUALS(initialManagerSize + 1, mgr.size());
    // the right one?
    auto foundAlg = mgr.getAlgorithm(alg2->getAlgorithmID());
    TS_ASSERT(foundAlg);
  }

  void test_runningInstancesOf() {
    AlgorithmManager::Instance().clear();
    // Had better return empty at this point
    TS_ASSERT(AlgorithmManager::Instance().runningInstancesOf("AlgTest").empty())
    // Create an algorithm, but don't start it
    AlgorithmManager::Instance().create("AlgTest");
    // Still empty
    TS_ASSERT(AlgorithmManager::Instance().runningInstancesOf("AlgTest").empty())
    // Create the 'runs forever' algorithm
    AlgorithmManager::Instance().create("AlgRunsForever");
    auto runningAlgorithms = AlgorithmManager::Instance().runningInstancesOf("AlgRunsForever");
    TS_ASSERT_EQUALS(runningAlgorithms.size(), 1);
    TS_ASSERT_EQUALS(runningAlgorithms.at(0)->name(), "AlgRunsForever");
    // Create another 'runs forever' algorithm (without proxy) and another
    // 'normal' one
    auto aRunningAlgorithm = AlgorithmManager::Instance().create("AlgRunsForever", 1);
    TS_ASSERT(AlgorithmManager::Instance().runningInstancesOf("AlgTest").empty())
    TS_ASSERT_EQUALS(AlgorithmManager::Instance().runningInstancesOf("AlgRunsForever").size(), 2);
    // 'Stop' one of the running algorithms and check the count drops
    dynamic_cast<AlgRunsForever *>(aRunningAlgorithm.get())->setIsRunningTo(false);
    TS_ASSERT_EQUALS(AlgorithmManager::Instance().runningInstancesOf("AlgRunsForever").size(), 1);
    TS_ASSERT(AlgorithmManager::Instance().runningInstancesOf("AlgTest").empty())
    TS_ASSERT_EQUALS(AlgorithmManager::Instance().size(), 3);
    AlgorithmManager::Instance().cancelAll();
  }

  void test_cancelAll() {
    AlgorithmManager::Instance().clear();
    std::vector<Algorithm_sptr> algs(5);
    for (size_t i = 0; i < 5; i++) {
      // Create without proxy so that I can cast it to an Algorithm and get at
      // getCancel()
      algs[i] = std::dynamic_pointer_cast<Algorithm>(AlgorithmManager::Instance().create("AlgRunsForever", 1));
      TS_ASSERT(!algs[i]->getCancel());
    }

    AlgorithmManager::Instance().cancelAll();
    TS_ASSERT_EQUALS(AlgorithmManager::Instance().runningInstancesOf("AlgRunsForever").size(), 0);
    AlgorithmManager::Instance().clear();
  }

  int m_notificationValue;
};
