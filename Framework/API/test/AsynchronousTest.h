// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/WorkspaceGroup.h"

#include "MantidFrameworkTestHelpers/FakeObjects.h"
#include <Poco/ActiveResult.h>
#include <Poco/NObserver.h>
#include <Poco/Thread.h>

#include <condition_variable>
#include <mutex>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;

class AsynchronousTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AsynchronousTest *createSuite() { return new AsynchronousTest(); }
  static void destroySuite(AsynchronousTest *suite) { delete suite; }

  class AsyncAlgorithm : public Algorithm {
  public:
    AsyncAlgorithm() : Algorithm(), result(0), throw_exception(false) {}
    AsyncAlgorithm(const bool throw_default) : Algorithm(), result(0), throw_exception(throw_default) {}
    ~AsyncAlgorithm() override = default;
    const std::string name() const override { return "AsyncAlgorithm"; } ///< Algorithm's name for identification
    int version() const override { return 1; }                           ///< Algorithm's version for identification
    const std::string category() const override { return "Cat"; }        ///< Algorithm's category for identification
    const std::string summary() const override { return "Test summary"; }

    void init() override {
      declareProperty(
          std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input, PropertyMode::Optional));
    }

    void exec() override {
      if (g_synchronise) {
        std::unique_lock<std::mutex> lock(g_mtx);
        g_execStarted = true;
        g_condition1.notify_all();
        if (!g_testStarted) {
          g_condition2.wait(lock);
        }
      }
      Poco::Thread *thr = Poco::Thread::current();
      for (int i = 0; i < NO_OF_LOOPS; i++) {
        result = i;
        if (thr)
          thr->sleep(10);
        progress(double(i) / NO_OF_LOOPS); // send progress notification
        interruption_point();              // check for a termination request
        if (throw_exception && i == NO_OF_LOOPS / 2)
          throw std::runtime_error("Exception thrown");
      }
    }
    int result;

  protected:
    bool throw_exception;
  };

  // AsyncAlgorithmThrows is the same as AsyncAlgorithm except that it
  // throws by default. This provides an easy way to make sure any child
  // algorithms also throw.
  class AsyncAlgorithmThrows : public AsyncAlgorithm {
  public:
    AsyncAlgorithmThrows() : AsyncAlgorithm(true) {}
    const std::string name() const override { return "AsyncAlgorithmThrows"; } ///< Algorithm's name for identification
  };

  AsynchronousTest()
      : m_startedObserver(*this, &AsynchronousTest::handleStarted), startedNotificationReseived(false),
        m_finishedObserver(*this, &AsynchronousTest::handleFinished), finishedNotificationReseived(false),
        m_errorObserver(*this, &AsynchronousTest::handleError), errorNotificationReseived(false),
        errorNotificationMessage(""), m_progressObserver(*this, &AsynchronousTest::handleProgress), count(0) {
    // DECLARE_ALGORITHM macro doesn't work because the class name contains '::'
    // The algorithms need to be registered because cloning is done through
    // AlgorithmFactory
    if (!AlgorithmFactory::Instance().exists("AsyncAlgorithm")) {
      AlgorithmFactory::Instance().subscribe<AsyncAlgorithm>();
      AlgorithmFactory::Instance().subscribe<AsyncAlgorithmThrows>();
    }
  }

  void testExecution() {
    AsyncAlgorithm alg;
    setupTest(alg);
    auto result = alg.executeAsync();
    TS_ASSERT(!result.available())
    result.wait();
    generalChecks(alg, true, true, true, false);
    TS_ASSERT(result.available())
    TS_ASSERT_EQUALS(count, NO_OF_LOOPS)
    TS_ASSERT_EQUALS(alg.result, NO_OF_LOOPS - 1)
  }

  void testCancel() {
    AsyncAlgorithm alg;
    setupTest(alg);
    auto result = alg.executeAsync();
    alg.cancel();
    result.wait();
    generalChecks(alg, false, true, false, true);
    TS_ASSERT_LESS_THAN(alg.result, NO_OF_LOOPS - 1)
  }

  void testException() {
    AsyncAlgorithmThrows alg;
    setupTest(alg);
    alg.addObserver(m_errorObserver);
    auto result = alg.executeAsync();
    result.wait();
    generalChecks(alg, false, true, false, true);
    TS_ASSERT_LESS_THAN(alg.result, NO_OF_LOOPS - 1)
    TS_ASSERT_EQUALS(errorNotificationMessage, "Exception thrown")
  }

  void testExecutionGroupWS() {
    WorkspaceGroup_sptr groupWS = makeGroupWorkspace();
    AsyncAlgorithm alg;
    setupTest(alg);
    alg.setPropertyValue("InputWorkspace", "groupWS");
    auto result = alg.executeAsync();
    TS_ASSERT(!result.available())
    result.wait();
    generalChecks(alg, true, true, true, false);
    TS_ASSERT(result.available())
    // There are 2 * NO_OF_LOOPS because there are two child workspaces
    TS_ASSERT_EQUALS(count, NO_OF_LOOPS * 2)
    // The parent algorithm is not executed directly, so the result remains 0
    TS_ASSERT_EQUALS(alg.result, 0)
  }

  void testCancelGroupWS() {
    g_synchronise = true;
    g_testStarted = false;
    g_execStarted = false;
    WorkspaceGroup_sptr groupWS = makeGroupWorkspace();
    AsyncAlgorithm alg;
    setupTest(alg);
    alg.setPropertyValue("InputWorkspace", "groupWS");
    auto result = alg.executeAsync();
    {
      std::unique_lock<std::mutex> lock(g_mtx);
      if (!g_execStarted) {
        g_condition1.wait(lock);
      }
    }
    alg.cancel();
    {
      std::unique_lock<std::mutex> lock(g_mtx);
      g_testStarted = true;
    }
    g_condition2.notify_all();
    result.wait();
    generalChecks(alg, false, true, false, true);
    // The parent algorithm is not executed directly, so the result remains 0
    TS_ASSERT_EQUALS(alg.result, 0)
  }

  void testExceptionGroupWS() {
    WorkspaceGroup_sptr groupWS = makeGroupWorkspace();
    AsyncAlgorithmThrows alg;
    setupTest(alg);
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", "groupWS");
    auto result = alg.executeAsync();
    result.wait();
    generalChecks(alg, false, true, false, true);
    TS_ASSERT_EQUALS(errorNotificationMessage, "Execution of AsyncAlgorithmThrows for group entry 1 "
                                               "failed: Exception thrown")
    // The parent algorithm is not executed directly, so the result remains 0
    TS_ASSERT_EQUALS(alg.result, 0)
  }

private:
  Poco::NObserver<AsynchronousTest, Mantid::API::Algorithm::StartedNotification> m_startedObserver;
  bool startedNotificationReseived;
  void handleStarted(const Poco::AutoPtr<Mantid::API::Algorithm::StartedNotification> &) {
    startedNotificationReseived = true;
  }

  Poco::NObserver<AsynchronousTest, Mantid::API::Algorithm::FinishedNotification> m_finishedObserver;
  bool finishedNotificationReseived;
  void handleFinished(const Poco::AutoPtr<Mantid::API::Algorithm::FinishedNotification> &) {
    finishedNotificationReseived = true;
  }

  Poco::NObserver<AsynchronousTest, Mantid::API::Algorithm::ErrorNotification> m_errorObserver;
  bool errorNotificationReseived;
  std::string errorNotificationMessage;
  void handleError(const Poco::AutoPtr<Mantid::API::Algorithm::ErrorNotification> &pNf) {
    errorNotificationReseived = true;
    errorNotificationMessage = pNf->what;
  }

  Poco::NObserver<AsynchronousTest, Mantid::API::Algorithm::ProgressNotification> m_progressObserver;
  int count;
  void handleProgress(const Poco::AutoPtr<Mantid::API::Algorithm::ProgressNotification> &pNf) {
    count++;
    TS_ASSERT_LESS_THAN(pNf->progress, 1.000001)
  }

  WorkspaceGroup_sptr makeGroupWorkspace() {
    std::shared_ptr<WorkspaceTester> ws0 = std::make_shared<WorkspaceTester>();
    ws0->initialize(2, 4, 3);
    AnalysisDataService::Instance().addOrReplace("ws0", ws0);

    std::shared_ptr<WorkspaceTester> ws1 = std::make_shared<WorkspaceTester>();
    ws1->initialize(2, 4, 3);
    AnalysisDataService::Instance().addOrReplace("ws1", ws1);

    WorkspaceGroup_sptr groupWS(new WorkspaceGroup());
    AnalysisDataService::Instance().addOrReplace("groupWS", groupWS);
    groupWS->add("ws0");
    groupWS->add("ws1");

    return groupWS;
  }

  // Generic setup for all tests
  void setupTest(AsyncAlgorithm &alg) {
    startedNotificationReseived = false;
    finishedNotificationReseived = false;
    errorNotificationReseived = false;
    count = 0;

    alg.initialize();
    alg.addObserver(m_startedObserver);
    alg.addObserver(m_finishedObserver);
    alg.addObserver(m_progressObserver);
    alg.addObserver(m_errorObserver);
  }

  void generalChecks(AsyncAlgorithm &alg, const bool expectExecuted, const bool expectStarted,
                     const bool expectFinished, const bool expectError) {
    TS_ASSERT_EQUALS(alg.isExecuted(), expectExecuted)
    TS_ASSERT_EQUALS(startedNotificationReseived, expectStarted)
    TS_ASSERT_EQUALS(finishedNotificationReseived, expectFinished)
    TS_ASSERT_EQUALS(errorNotificationReseived, expectError)
  }

  static const int NO_OF_LOOPS = 10;
  static bool g_synchronise;
  static std::condition_variable g_condition1;
  static std::condition_variable g_condition2;
  static std::mutex g_mtx;
  static bool g_testStarted;
  static bool g_execStarted;
};

bool AsynchronousTest::g_synchronise = false;
std::condition_variable AsynchronousTest::g_condition1;
std::condition_variable AsynchronousTest::g_condition2;
std::mutex AsynchronousTest::g_mtx;
bool AsynchronousTest::g_testStarted = false;
bool AsynchronousTest::g_execStarted = false;
