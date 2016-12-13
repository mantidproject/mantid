#ifndef ALGORITHMPROXYTEST_H_
#define ALGORITHMPROXYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmProxy.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmObserver.h"

#include <Poco/ActiveResult.h>
#include <Poco/Thread.h>

#include <boost/lexical_cast.hpp>

using namespace Mantid::API;
using namespace Mantid::Kernel;

class ToyAlgorithmProxy : public Algorithm {
public:
  ToyAlgorithmProxy() : Algorithm() {}
  ~ToyAlgorithmProxy() override {}
  const std::string name() const override {
    return "ToyAlgorithmProxy";
  } ///< Algorithm's name for identification
  int version() const override {
    return 1;
  } ///< Algorithm's version for identification
  const std::string category() const override {
    return "ProxyCat";
  } ///< Algorithm's category for identification
  const std::string alias() const override {
    return "Dog";
  } ///< Algorithm's alias
  const std::string summary() const override { return "Test summary"; }
  const std::string workspaceMethodName() const override {
    return "toyalgorithm";
  }
  const std::string workspaceMethodOnTypes() const override {
    return "MatrixWorkspace;ITableWorkspace";
  }
  const std::string workspaceMethodInputProperty() const override {
    return "InputWorkspace";
  }

  void init() override {
    declareProperty("prop1", "value");
    declareProperty("prop2", 1);
    declareProperty("out", 8, Direction::Output);
  }
  void exec() override {
    std::string p1 = getProperty("prop1");
    int p2 = getProperty("prop2");

    Poco::Thread::current()->sleep(500);
    progress(0.333, "Running");
    interruption_point();
    Algorithm *alg = dynamic_cast<Algorithm *>(this);
    TS_ASSERT(alg);

    TS_ASSERT_EQUALS(p1, "stuff");
    TS_ASSERT_EQUALS(p2, 17);

    setProperty("out", 28);
  }
};

class ToyAlgorithmProxyMultipleCategory : public Algorithm {
public:
  ToyAlgorithmProxyMultipleCategory() : Algorithm() {}
  ~ToyAlgorithmProxyMultipleCategory() override {}
  const std::string name() const override {
    return "ToyAlgorithmProxyMultipleCategory";
  } ///< Algorithm's name for identification
  int version() const override {
    return 1;
  } ///< Algorithm's version for identification
  const std::string category() const override {
    return "ProxyCat;ProxyLeopard";
  } ///< Algorithm's category for identification
  const std::string alias() const override {
    return "Dog";
  } ///< Algorithm's alias
  const std::string summary() const override { return "Test summary"; }
  void init() override {
    declareProperty("prop1", "value");
    declareProperty("prop2", 1);
    declareProperty("out", 8, Direction::Output);
  }
  void exec() override {
    std::string p1 = getProperty("prop1");
    int p2 = getProperty("prop2");

    Poco::Thread::current()->sleep(500);
    progress(0.333, "Running");
    interruption_point();
    Algorithm *alg = dynamic_cast<Algorithm *>(this);
    TS_ASSERT(alg);

    TS_ASSERT_EQUALS(p1, "stuff");
    TS_ASSERT_EQUALS(p2, 17);

    setProperty("out", 28);
  }
};

DECLARE_ALGORITHM(ToyAlgorithmProxy)
DECLARE_ALGORITHM(ToyAlgorithmProxyMultipleCategory)

class TestProxyObserver : public AlgorithmObserver {
public:
  bool start, progress, finish;
  TestProxyObserver()
      : AlgorithmObserver(), start(false), progress(false), finish(false) {}
  TestProxyObserver(IAlgorithm_const_sptr alg)
      : AlgorithmObserver(alg), start(false), progress(false), finish(false) {}
  void startHandle(const IAlgorithm *) override { start = true; }
  void progressHandle(const IAlgorithm *, double p,
                      const std::string &msg) override {
    progress = true;
    TS_ASSERT_EQUALS(p, 0.333);
    TS_ASSERT_EQUALS(msg, "Running");
  }
  void finishHandle(const IAlgorithm *) override { finish = true; }
};

class AlgorithmProxyTest : public CxxTest::TestSuite {
public:
  void testCreateProxy() {
    IAlgorithm_sptr alg =
        AlgorithmManager::Instance().create("ToyAlgorithmProxy");
    TS_ASSERT(dynamic_cast<AlgorithmProxy *>(alg.get()));
    TS_ASSERT_EQUALS(alg->name(), "ToyAlgorithmProxy");
    TS_ASSERT_EQUALS(alg->version(), 1);
    TS_ASSERT_EQUALS(alg->category(), "ProxyCat");
    TS_ASSERT_EQUALS(alg->alias(), "Dog");
    TS_ASSERT(alg->isInitialized());
    TS_ASSERT(alg->existsProperty("prop1"));
    TS_ASSERT(alg->existsProperty("prop2"));
    TS_ASSERT(!alg->isRunning());
    alg->setProperty("prop1", "stuff");
    alg->setProperty("prop2", 17);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
    int out = alg->getProperty("out");
    TS_ASSERT_EQUALS(out, 28);
  }

  void testMultipleCategory() {
    IAlgorithm_sptr alg = AlgorithmManager::Instance().create(
        "ToyAlgorithmProxyMultipleCategory");
    TS_ASSERT(dynamic_cast<AlgorithmProxy *>(alg.get()));
    TS_ASSERT_EQUALS(alg->name(), "ToyAlgorithmProxyMultipleCategory");
    TS_ASSERT_EQUALS(alg->version(), 1);
    TS_ASSERT_EQUALS(alg->category(), "ProxyCat;ProxyLeopard");
    std::vector<std::string> result{"ProxyCat", "ProxyLeopard"};
    TS_ASSERT_EQUALS(alg->categories(), result);
    TS_ASSERT_EQUALS(alg->alias(), "Dog");
    TS_ASSERT(alg->isInitialized());
  }

  /**
   * Disabled due to random failures that cannot be pinned down and are most
   * likely timing issues.
   * This test has never failed legitimately and only serves to cause confusion
   * when it fails
   * due to completely unrelated changes.
   */
  void xtestRunning() {
    IAlgorithm_sptr alg =
        AlgorithmManager::Instance().create("ToyAlgorithmProxy");
    TS_ASSERT(dynamic_cast<AlgorithmProxy *>(alg.get()));
    alg->setProperty("prop1", "stuff");
    alg->setProperty("prop2", 17);
    Poco::ActiveResult<bool> res = alg->executeAsync();
    res.tryWait(60);
    TS_ASSERT(alg->isRunning());

    res.wait();
    TS_ASSERT(res.data());
    TS_ASSERT(alg->isExecuted());
  }

  void testCancel() {
    IAlgorithm_sptr alg =
        AlgorithmManager::Instance().create("ToyAlgorithmProxy");
    TS_ASSERT(dynamic_cast<AlgorithmProxy *>(alg.get()));
    alg->setProperty("prop1", "stuff");
    alg->setProperty("prop2", 17);
    Poco::ActiveResult<bool> res = alg->executeAsync();
    res.tryWait(100);
    alg->cancel();
    res.wait();
    TS_ASSERT(!alg->isExecuted());
    int out = alg->getProperty("out");
    TS_ASSERT_EQUALS(out, 8);
  }
  void testAddObserver() {
    IAlgorithm_sptr alg =
        AlgorithmManager::Instance().create("ToyAlgorithmProxy");
    TS_ASSERT(dynamic_cast<AlgorithmProxy *>(alg.get()));
    alg->setProperty("prop1", "stuff");
    alg->setProperty("prop2", 17);
    TestProxyObserver obs(alg);
    Poco::ActiveResult<bool> res = alg->executeAsync();
    res.wait();
    TS_ASSERT(obs.start);
    TS_ASSERT(obs.progress);
    TS_ASSERT(obs.finish);
  }

  void test_WorkspaceMethodFunctionsReturnProxiedContent() {
    IAlgorithm_sptr alg =
        AlgorithmManager::Instance().create("ToyAlgorithmProxy");

    TS_ASSERT_EQUALS("toyalgorithm", alg->workspaceMethodName());

    auto types = alg->workspaceMethodOn();
    TS_ASSERT_EQUALS(2, types.size());
    if (types.size() == 2) {
      TS_ASSERT_EQUALS("MatrixWorkspace", types[0]);
      TS_ASSERT_EQUALS("ITableWorkspace", types[1]);
    }
    TS_ASSERT_EQUALS("InputWorkspace", alg->workspaceMethodInputProperty());
  }

  void test_copyPropertiesFrom() {
    IAlgorithm_sptr alg =
        AlgorithmManager::Instance().create("ToyAlgorithmProxy");
    alg->initialize();
    alg->setPropertyValue("prop1", "string");
    alg->setPropertyValue("prop2", "1");
    IAlgorithm_sptr algCopy =
        AlgorithmManager::Instance().create("ToyAlgorithmProxy");

    auto algProxy = boost::dynamic_pointer_cast<AlgorithmProxy>(alg);
    auto algCopyProxy = boost::dynamic_pointer_cast<AlgorithmProxy>(algCopy);
    algCopyProxy->copyPropertiesFrom(*algProxy);

    int val = boost::lexical_cast<int>(algCopy->getPropertyValue("prop2"));

    TS_ASSERT_EQUALS(val, 1);

    // set another value and check the other value is unaffected
    algCopy->setPropertyValue("prop1", "A difference");
    int val2 = boost::lexical_cast<int>(algCopy->getPropertyValue("prop2"));

    TS_ASSERT_EQUALS(val, val2);
  }
};

#endif /*ALGORITHMPROXYTEST_H_*/
