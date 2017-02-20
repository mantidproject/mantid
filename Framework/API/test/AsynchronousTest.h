#ifndef ASYNCHRONOUSTEST_H_
#define ASYNCHRONOUSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidTestHelpers/FakeObjects.h"
#include <Poco/ActiveResult.h>
#include <Poco/NObserver.h>
#include <Poco/Thread.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace std;

#define NofLoops 10

class AsyncAlgorithm : public Algorithm {
public:
  AsyncAlgorithm() : Algorithm(), throw_exception(false), result(0) {}
  AsyncAlgorithm(const bool throw_default)
      : Algorithm(), throw_exception(throw_default), result(0) {}
  ~AsyncAlgorithm() override {}
  const std::string name() const override {
    return "AsyncAlgorithm";
  } ///< Algorithm's name for identification
  int version() const override {
    return 1;
  } ///< Algorithm's version for identification
  const std::string category() const override {
    return "Cat";
  } ///< Algorithm's category for identification
  const std::string summary() const override { return "Test summary"; }

  void init() override {
    declareProperty(std::make_unique<WorkspaceProperty<>>(
        "InputWorkspace", "", Direction::Input, PropertyMode::Optional));
  }

  void exec() override {
    Poco::Thread *thr = Poco::Thread::current();
    for (int i = 0; i < NofLoops; i++) {
      result = i;
      if (thr)
        thr->sleep(1);
      progress(double(i) / NofLoops); // send progress notification
      interruption_point();           // check for a termination request
      if (throw_exception && i == NofLoops / 2)
        throw std::runtime_error("Exception thrown");
    }
  }
  int result;

protected:
  bool throw_exception;
};

DECLARE_ALGORITHM(AsyncAlgorithm)

// AsyncAlgorithmThrows is the same as AsyncAlgorithm except that it
// throws by default. This provides an easy way to make sure any child
// algorithms also throw.
class AsyncAlgorithmThrows : public AsyncAlgorithm {
public:
  AsyncAlgorithmThrows() : AsyncAlgorithm(true) {}
  const std::string name() const override {
    return "AsyncAlgorithmThrows";
  } ///< Algorithm's name for identification
};

DECLARE_ALGORITHM(AsyncAlgorithmThrows)

class AsynchronousTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AsynchronousTest *createSuite() { return new AsynchronousTest(); }
  static void destroySuite(AsynchronousTest *suite) { delete suite; }

  AsynchronousTest()
      : m_startedObserver(*this, &AsynchronousTest::handleStarted),
        startedNotificationReseived(false),
        m_finishedObserver(*this, &AsynchronousTest::handleFinished),
        finishedNotificationReseived(false),
        m_errorObserver(*this, &AsynchronousTest::handleError),
        errorNotificationReseived(false), errorNotificationMessage(""),
        m_progressObserver(*this, &AsynchronousTest::handleProgress), count(0) {
  }

  Poco::NObserver<AsynchronousTest, Mantid::API::Algorithm::StartedNotification>
      m_startedObserver;
  bool startedNotificationReseived;
  void handleStarted(
      const Poco::AutoPtr<Mantid::API::Algorithm::StartedNotification> &) {
    startedNotificationReseived = true;
  }

  Poco::NObserver<AsynchronousTest,
                  Mantid::API::Algorithm::FinishedNotification>
      m_finishedObserver;
  bool finishedNotificationReseived;
  void handleFinished(
      const Poco::AutoPtr<Mantid::API::Algorithm::FinishedNotification> &) {
    finishedNotificationReseived = true;
  }

  Poco::NObserver<AsynchronousTest, Mantid::API::Algorithm::ErrorNotification>
      m_errorObserver;
  bool errorNotificationReseived;
  std::string errorNotificationMessage;
  void handleError(
      const Poco::AutoPtr<Mantid::API::Algorithm::ErrorNotification> &pNf) {
    errorNotificationReseived = true;
    errorNotificationMessage = pNf->what;
  }

  Poco::NObserver<AsynchronousTest,
                  Mantid::API::Algorithm::ProgressNotification>
      m_progressObserver;
  int count;
  void handleProgress(
      const Poco::AutoPtr<Mantid::API::Algorithm::ProgressNotification> &pNf) {
    count++;
    TS_ASSERT_LESS_THAN(pNf->progress, 1.000001)
  }

  void testExecution() {
    startedNotificationReseived = false;
    finishedNotificationReseived = false;
    errorNotificationReseived = false;
    count = 0;
    AsyncAlgorithm alg;
    alg.addObserver(m_startedObserver);
    alg.addObserver(m_finishedObserver);
    alg.addObserver(m_progressObserver);
    alg.initialize();
    Poco::ActiveResult<bool> result = alg.executeAsync();
    TS_ASSERT(!result.available())
    result.wait();
    TS_ASSERT(result.available())
    TS_ASSERT(alg.isExecuted())
    TS_ASSERT(startedNotificationReseived)
    TS_ASSERT(finishedNotificationReseived)
    TS_ASSERT_EQUALS(count, NofLoops)
    TS_ASSERT_EQUALS(alg.result, NofLoops - 1)
  }

  void testCancel() {
    startedNotificationReseived = false;
    finishedNotificationReseived = false;
    errorNotificationReseived = false;
    AsyncAlgorithm alg;
    alg.addObserver(m_startedObserver);
    alg.addObserver(m_finishedObserver);
    alg.addObserver(m_progressObserver);
    alg.initialize();
    Poco::ActiveResult<bool> result = alg.executeAsync();
    alg.cancel();
    result.wait();
    TS_ASSERT(!alg.isExecuted())
    TS_ASSERT_LESS_THAN(alg.result, NofLoops - 1)
    TS_ASSERT(!finishedNotificationReseived)
  }

  void testException() {
    startedNotificationReseived = false;
    finishedNotificationReseived = false;
    errorNotificationReseived = false;
    AsyncAlgorithmThrows alg;
    alg.addObserver(m_startedObserver);
    alg.addObserver(m_finishedObserver);
    alg.addObserver(m_progressObserver);
    alg.addObserver(m_errorObserver);
    alg.initialize();
    Poco::ActiveResult<bool> result = alg.executeAsync();
    result.wait();
    TS_ASSERT(!alg.isExecuted())
    TS_ASSERT_LESS_THAN(alg.result, NofLoops - 1)
    TS_ASSERT(!finishedNotificationReseived)
    TS_ASSERT(errorNotificationReseived)
    TS_ASSERT_EQUALS(errorNotificationMessage, "Exception thrown")
  }

  void testExecutionGroupWS() {
    startedNotificationReseived = false;
    finishedNotificationReseived = false;
    errorNotificationReseived = false;
    count = 0;
    WorkspaceGroup_sptr groupWS = makeGroupWorkspace();
    AsyncAlgorithm alg;
    alg.addObserver(m_startedObserver);
    alg.addObserver(m_finishedObserver);
    alg.addObserver(m_progressObserver);
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", "groupWS");
    Poco::ActiveResult<bool> result = alg.executeAsync();
    TS_ASSERT(!result.available())
    result.wait();
    TS_ASSERT(result.available())
    TS_ASSERT(alg.isExecuted())
    TS_ASSERT(startedNotificationReseived)
    TS_ASSERT(finishedNotificationReseived)
    // There are 2 * NofLoops because there are two child workspaces
    TS_ASSERT_EQUALS(count, NofLoops * 2)
    // The parent algorithm is not executed directly, so the result remains 0
    TS_ASSERT_EQUALS(alg.result, 0)
  }

  void testCancelGroupWS() {
    startedNotificationReseived = false;
    finishedNotificationReseived = false;
    errorNotificationReseived = false;
    WorkspaceGroup_sptr groupWS = makeGroupWorkspace();
    AsyncAlgorithm alg;
    alg.addObserver(m_startedObserver);
    alg.addObserver(m_finishedObserver);
    alg.addObserver(m_progressObserver);
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", "groupWS");
    Poco::ActiveResult<bool> result = alg.executeAsync();
    alg.cancel();
    result.wait();
    TS_ASSERT(!alg.isExecuted())
    TS_ASSERT(startedNotificationReseived)
    TS_ASSERT(!finishedNotificationReseived)
    TS_ASSERT(!errorNotificationReseived)
    // The parent algorithm is not executed directly, so the result remains 0
    TS_ASSERT_EQUALS(alg.result, 0)
  }

  void testExceptionGroupWS() {
    startedNotificationReseived = false;
    finishedNotificationReseived = false;
    errorNotificationReseived = false;
    WorkspaceGroup_sptr groupWS = makeGroupWorkspace();
    AsyncAlgorithmThrows alg;
    alg.addObserver(m_startedObserver);
    alg.addObserver(m_finishedObserver);
    alg.addObserver(m_progressObserver);
    alg.addObserver(m_errorObserver);
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", "groupWS");
    Poco::ActiveResult<bool> result = alg.executeAsync();
    result.wait();
    TS_ASSERT(!alg.isExecuted())
    TS_ASSERT(startedNotificationReseived)
    TS_ASSERT(!finishedNotificationReseived)
    TS_ASSERT(errorNotificationReseived)
    TS_ASSERT_EQUALS(errorNotificationMessage,
                     "Execution of AsyncAlgorithmThrows for group entry 1 "
                     "failed: Exception thrown")
    // The parent algorithm is not executed directly, so the result remains 0
    TS_ASSERT_EQUALS(alg.result, 0)
  }

private:
  WorkspaceGroup_sptr makeGroupWorkspace() {
    boost::shared_ptr<WorkspaceTester> ws0 =
        boost::make_shared<WorkspaceTester>();
    ws0->initialize(2, 4, 3);
    AnalysisDataService::Instance().addOrReplace("ws0", ws0);

    boost::shared_ptr<WorkspaceTester> ws1 =
        boost::make_shared<WorkspaceTester>();
    ws1->initialize(2, 4, 3);
    AnalysisDataService::Instance().addOrReplace("ws1", ws1);

    WorkspaceGroup_sptr groupWS(new WorkspaceGroup());
    AnalysisDataService::Instance().addOrReplace("groupWS", groupWS);
    groupWS->add("ws0");
    groupWS->add("ws1");

    return groupWS;
  }
};

#endif /*ASYNCHRONOUSTEST_H_*/
