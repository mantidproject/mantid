// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef AlgorithmManagerTest_H_
#define AlgorithmManagerTest_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmProxy.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/ConfigService.h"
#include <Poco/ActiveResult.h>
#include <Poco/Thread.h>
#include <stdexcept>
#include <vector>

using namespace Mantid::API;

class AlgTest : public Algorithm {
public:
  AlgTest() : Algorithm() {}
  ~AlgTest() override {}
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
  ~AlgTestFail() override {}
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
  ~AlgTestPass() override {}
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
  ~AlgTestSecond() override {}
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
  ~AlgRunsForever() override {}
  void init() override {}
  void exec() override {}
  const std::string name() const override { return "AlgRunsForever"; }
  int version() const override { return (1); }
  const std::string category() const override { return ("Cat1"); }
  const std::string summary() const override { return "Test summary"; }
  // Override method so we can manipulate whether it appears to be running
  bool isRunning() const override { return isRunningFlag; }
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
  static AlgorithmManagerTest *createSuite() {
    return new AlgorithmManagerTest();
  }
  static void destroySuite(AlgorithmManagerTest *suite) { delete suite; }

  AlgorithmManagerTest() {
    // A test fails unless algorithms.retained is big enough
    Mantid::Kernel::ConfigService::Instance().setString("algorithms.retained",
                                                        "5");
  }

  void testVersionFail() {
    const size_t nalgs = AlgorithmFactory::Instance().getKeys().size();
    TS_ASSERT_THROWS(AlgorithmFactory::Instance().subscribe<AlgTestFail>(),
                     const std::runtime_error &);
    // Size should be the same
    TS_ASSERT_EQUALS(AlgorithmFactory::Instance().getKeys().size(), nalgs);
  }

  void testVersionPass() {
    TS_ASSERT_THROWS_NOTHING(
        AlgorithmFactory::Instance().subscribe<AlgTestPass>());
  }

  void testInstance() {
    TS_ASSERT_THROWS_NOTHING(AlgorithmManager::Instance().create("AlgTest"));
    TS_ASSERT_THROWS(AlgorithmManager::Instance().create("AlgTest", 3),
                     const std::runtime_error &);
    TS_ASSERT_THROWS(AlgorithmManager::Instance().create("aaaaaa"),
                     const std::runtime_error &);
  }

  void testClear() {
    AlgorithmManager::Instance().clear();
    TS_ASSERT_THROWS_NOTHING(AlgorithmManager::Instance().create("AlgTest"));
    TS_ASSERT_THROWS_NOTHING(
        AlgorithmManager::Instance().create("AlgTestSecond"));
    TS_ASSERT_EQUALS(AlgorithmManager::Instance().size(), 2);
    AlgorithmManager::Instance().clear();
    TS_ASSERT_EQUALS(AlgorithmManager::Instance().size(), 0);
  }

  void testReturnType() {
    AlgorithmManager::Instance().clear();
    IAlgorithm_sptr alg;
    TS_ASSERT_THROWS_NOTHING(
        alg = AlgorithmManager::Instance().create("AlgTest", 1));
    TS_ASSERT_DIFFERS(dynamic_cast<AlgorithmProxy *>(alg.get()),
                      static_cast<AlgorithmProxy *>(nullptr));
    TS_ASSERT_THROWS_NOTHING(
        alg = AlgorithmManager::Instance().create("AlgTestSecond", 1));
    TS_ASSERT_DIFFERS(dynamic_cast<AlgorithmProxy *>(alg.get()),
                      static_cast<AlgorithmProxy *>(nullptr));
    TS_ASSERT_DIFFERS(dynamic_cast<IAlgorithm *>(alg.get()),
                      static_cast<IAlgorithm *>(nullptr));
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

  void testCreateNoProxy() {
    AlgorithmManager::Instance().clear();
    IAlgorithm_sptr Aptr, Bptr;
    Aptr = AlgorithmManager::Instance().create("AlgTest", -1, true);
    Bptr = AlgorithmManager::Instance().create("AlgTest", -1, false);
    TSM_ASSERT("Was created as a AlgorithmProxy",
               dynamic_cast<AlgorithmProxy *>(Aptr.get()));
    TSM_ASSERT("Was NOT created as a AlgorithmProxy",
               dynamic_cast<AlgorithmProxy *>(Bptr.get()) == nullptr);
  }

  // This will be called back when an algo starts
  void handleAlgorithmStartingNotification(
      const Poco::AutoPtr<AlgorithmStartingNotification> & /*pNf*/) {
    m_notificationValue = 12345;
  }

  /** When running an algorithm in async mode, the
   * AlgorithmManager needs to send out a notification
   */
  void testStartingNotification() {
    AlgorithmManager::Instance().clear();
    Poco::NObserver<AlgorithmManagerTest,
                    Mantid::API::AlgorithmStartingNotification>
        my_observer(*this,
                    &AlgorithmManagerTest::handleAlgorithmStartingNotification);
    AlgorithmManager::Instance().notificationCenter.addObserver(my_observer);

    IAlgorithm_sptr Aptr, Bptr;
    Aptr = AlgorithmManager::Instance().create("AlgTest", -1, true);
    Bptr = AlgorithmManager::Instance().create("AlgTest", -1, false);

    m_notificationValue = 0;
    Poco::ActiveResult<bool> resB = Bptr->executeAsync();
    resB.wait();
    TSM_ASSERT_EQUALS("Notification was received.", m_notificationValue, 12345);

    m_notificationValue = 0;
    Poco::ActiveResult<bool> resA = Aptr->executeAsync();
    resA.wait();
    TSM_ASSERT_EQUALS("Notification was received (proxy).", m_notificationValue,
                      12345);

    AlgorithmManager::Instance().notificationCenter.removeObserver(my_observer);
  }

  /** Keep the right number of algorithms in the list.
   *  This also tests setMaxAlgorithms().
   */
  void testDroppingOldOnes() {
    AlgorithmManager::Instance().setMaxAlgorithms(5);
    AlgorithmManager::Instance().clear();
    TS_ASSERT_EQUALS(AlgorithmManager::Instance().size(), 0);

    IAlgorithm_sptr first = AlgorithmManager::Instance().create("AlgTest");
    // Fill up the list
    for (size_t i = 1; i < 5; i++)
      AlgorithmManager::Instance().create("AlgTest");
    TS_ASSERT_EQUALS(AlgorithmManager::Instance().size(), 5);

    // The first one is still in the list
    TS_ASSERT(AlgorithmManager::Instance().getAlgorithm(
                  first->getAlgorithmID()) == first);

    // Add one more, drops the oldest one
    AlgorithmManager::Instance().create("AlgTest");
    TS_ASSERT_EQUALS(AlgorithmManager::Instance().size(), 5);
    TS_ASSERT(
        !AlgorithmManager::Instance().getAlgorithm(first->getAlgorithmID()));
  }

  /** Keep one algorithm running, drop the second-oldest one etc. */
  void testDroppingOldOnes_whenAnAlgorithmIsStillRunning() {
    AlgorithmManager::Instance().clear();
    TS_ASSERT_EQUALS(AlgorithmManager::Instance().size(), 0);

    // Create one algorithm that appears never to stop
    IAlgorithm_sptr first =
        AlgorithmManager::Instance().create("AlgRunsForever");

    IAlgorithm_sptr second = AlgorithmManager::Instance().create("AlgTest");

    // Another long-running algo
    IAlgorithm_sptr third =
        AlgorithmManager::Instance().create("AlgRunsForever");

    for (size_t i = 3; i < 5; i++)
      AlgorithmManager::Instance().create("AlgTest");
    TS_ASSERT_EQUALS(AlgorithmManager::Instance().size(), 5);

    // The first three created are in the list
    TS_ASSERT(AlgorithmManager::Instance().getAlgorithm(
                  first->getAlgorithmID()) == first);
    TS_ASSERT(AlgorithmManager::Instance().getAlgorithm(
                  second->getAlgorithmID()) == second);
    TS_ASSERT(AlgorithmManager::Instance().getAlgorithm(
                  third->getAlgorithmID()) == third);

    // Add one more, drops the SECOND oldest one
    AlgorithmManager::Instance().create("AlgTest");
    TS_ASSERT_EQUALS(AlgorithmManager::Instance().size(), 5);

    TSM_ASSERT("The oldest algorithm (is still running) so it is still there",
               AlgorithmManager::Instance().getAlgorithm(
                   first->getAlgorithmID()) == first);
    TSM_ASSERT(
        "The second oldest was popped, so trying to get it should return null",
        !AlgorithmManager::Instance().getAlgorithm(second->getAlgorithmID()));

    // One more time
    AlgorithmManager::Instance().create("AlgTest");
    TS_ASSERT_EQUALS(AlgorithmManager::Instance().size(), 5);

    // The right ones are at the front
    TSM_ASSERT("The oldest algorithm (is still running) so it is still there",
               AlgorithmManager::Instance().getAlgorithm(
                   first->getAlgorithmID()) == first);
    TSM_ASSERT("The third algorithm (is still running) so it is still there",
               AlgorithmManager::Instance().getAlgorithm(
                   third->getAlgorithmID()) == third);
    AlgorithmManager::Instance().cancelAll();
  }

  void testDroppingOldOnes_extremeCase() {
    /** Extreme case where your queue fills up and all algos are running */
    AlgorithmManager::Instance().clear();
    for (size_t i = 0; i < 5; i++) {
      AlgorithmManager::Instance().create("AlgRunsForever");
    }

    TS_ASSERT_EQUALS(AlgorithmManager::Instance().size(), 5);
    // Create another that takes it past the normal max size (of 5)
    AlgorithmManager::Instance().create("AlgTest");
    TS_ASSERT_EQUALS(AlgorithmManager::Instance().size(), 6);
    AlgorithmManager::Instance().cancelAll();
  }

  void testThreadSafety() {
    PARALLEL_FOR_NO_WSP_CHECK()
    for (int i = 0; i < 5000; i++) {
      AlgorithmManager::Instance().create("AlgTest");
    }
  }

  void testRemovingByIdRemovesCorrectObject() {
    auto &mgr = AlgorithmManager::Instance();
    mgr.setMaxAlgorithms(10);
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

    mgr.setMaxAlgorithms(5);
  }

  void test_newestInstanceOf() {
    auto &am = AlgorithmManager::Instance();
    am.clear();
    auto first = am.create("AlgTest");
    TS_ASSERT_EQUALS(am.newestInstanceOf("AlgTest"), first);
    auto second = am.create("AlgTest");
    TS_ASSERT_EQUALS(am.newestInstanceOf("AlgTest"), second);
    TS_ASSERT(!am.newestInstanceOf("AlgTestSecond"));
    // Create a different algorithm
    am.create("AlgTestSecond");
    // Make sure we still get back the latest instance of other algorithm
    TS_ASSERT_EQUALS(am.newestInstanceOf("AlgTest"), second);
  }

  void test_runningInstancesOf() {
    AlgorithmManager::Instance().clear();
    // Had better return empty at this point
    TS_ASSERT(
        AlgorithmManager::Instance().runningInstancesOf("AlgTest").empty())
    // Create an algorithm, but don't start it
    AlgorithmManager::Instance().create("AlgTest");
    // Still empty
    TS_ASSERT(
        AlgorithmManager::Instance().runningInstancesOf("AlgTest").empty())
    // Create the 'runs forever' algorithm
    AlgorithmManager::Instance().create("AlgRunsForever");
    auto runningAlgorithms =
        AlgorithmManager::Instance().runningInstancesOf("AlgRunsForever");
    TS_ASSERT_EQUALS(runningAlgorithms.size(), 1);
    TS_ASSERT_EQUALS(runningAlgorithms.at(0)->name(), "AlgRunsForever");
    // Create another 'runs forever' algorithm (without proxy) and another
    // 'normal' one
    auto aRunningAlgorithm =
        AlgorithmManager::Instance().create("AlgRunsForever", 1, false);
    TS_ASSERT(
        AlgorithmManager::Instance().runningInstancesOf("AlgTest").empty())
    TS_ASSERT_EQUALS(AlgorithmManager::Instance()
                         .runningInstancesOf("AlgRunsForever")
                         .size(),
                     2);
    // 'Stop' one of the running algorithms and check the count drops
    dynamic_cast<AlgRunsForever *>(aRunningAlgorithm.get())
        ->setIsRunningTo(false);
    TS_ASSERT_EQUALS(AlgorithmManager::Instance()
                         .runningInstancesOf("AlgRunsForever")
                         .size(),
                     1);
    TS_ASSERT(
        AlgorithmManager::Instance().runningInstancesOf("AlgTest").empty())
    TS_ASSERT_EQUALS(AlgorithmManager::Instance().size(), 3);
    AlgorithmManager::Instance().cancelAll();
  }

  void test_cancelAll() {
    AlgorithmManager::Instance().clear();
    std::vector<Algorithm_sptr> algs(5);
    for (size_t i = 0; i < 5; i++) {
      // Create without proxy so that I can cast it to an Algorithm and get at
      // getCancel()
      algs[i] = boost::dynamic_pointer_cast<Algorithm>(
          AlgorithmManager::Instance().create("AlgRunsForever", 1, false));
      TS_ASSERT(!algs[i]->getCancel());
    }

    AlgorithmManager::Instance().cancelAll();
    TS_ASSERT_EQUALS(AlgorithmManager::Instance()
                         .runningInstancesOf("AlgRunsForever")
                         .size(),
                     0);
    AlgorithmManager::Instance().clear();
  }

  int m_notificationValue;
};

#endif /* AlgorithmManagerTest_H_*/
