#ifndef ASYNCHRONOUSTEST_H_
#define ASYNCHRONOUSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Algorithm.h"
#include <Poco/NObserver.h>
#include <iostream>

using namespace Mantid::Kernel; 
using namespace Mantid::API;
using namespace std;

#define NofLoops 10

class AsyncAlgorithm : public Algorithm
{
public:
  AsyncAlgorithm() : Algorithm(),throw_exception(false) {}
  virtual ~AsyncAlgorithm() {}
  const std::string name() const { return "AsyncAlgorithm";} ///< Algorithm's name for identification
  int version() const  { return 1;} ///< Algorithm's version for identification
  const std::string category() const { return "Cat";} ///< Algorithm's category for identification

  void init() {}
  void exec() 
  {
      Poco::Thread *thr = Poco::Thread::current();
      for(int i=0;i<NofLoops;i++)
      {
          result = i;
          if (thr) thr->sleep(1);
          progress(double(i)/NofLoops); // send progress notification
          interruption_point();  // check for a termination request
          if ( throw_exception && i == NofLoops/2 ) throw std::runtime_error("Exception thrown");
      }
  }
  int result;
  bool throw_exception;
};

DECLARE_ALGORITHM(AsyncAlgorithm)

class AsynchronousTest : public CxxTest::TestSuite
{
public: 

    AsynchronousTest():
      m_startedObserver(*this,&AsynchronousTest::handleStarted),
      startedNotificationReseived(false),
      m_finishedObserver(*this,&AsynchronousTest::handleFinished),
      finishedNotificationReseived(false),
      m_errorObserver(*this,&AsynchronousTest::handleError),
      errorNotificationReseived(false),
      m_progressObserver(*this,&AsynchronousTest::handleProgress),
      count(0)
    {}
  
    Poco::NObserver<AsynchronousTest, Mantid::API::Algorithm::StartedNotification> m_startedObserver;
    bool startedNotificationReseived;
    void handleStarted(const Poco::AutoPtr<Mantid::API::Algorithm::StartedNotification>&)
    {
        startedNotificationReseived = true;
    }

    Poco::NObserver<AsynchronousTest, Mantid::API::Algorithm::FinishedNotification> m_finishedObserver;
    bool finishedNotificationReseived;
    void handleFinished(const Poco::AutoPtr<Mantid::API::Algorithm::FinishedNotification>&)
    {
        finishedNotificationReseived = true;
    }

    Poco::NObserver<AsynchronousTest, Mantid::API::Algorithm::ErrorNotification> m_errorObserver;
    bool errorNotificationReseived;
    void handleError(const Poco::AutoPtr<Mantid::API::Algorithm::ErrorNotification>& pNf)
    {
        TS_ASSERT_EQUALS( pNf->what, "Exception thrown" )
        errorNotificationReseived = true;
    }

    Poco::NObserver<AsynchronousTest, Mantid::API::Algorithm::ProgressNotification> m_progressObserver;
    int count;
    void handleProgress(const Poco::AutoPtr<Mantid::API::Algorithm::ProgressNotification>& pNf)
    {
        count++;
        TS_ASSERT_LESS_THAN( pNf->progress, 1.000001 )
    }

    void testExecution()
    {
        AsyncAlgorithm alg;
        alg.initialize();
        alg.addObserver(m_startedObserver);
        alg.addObserver(m_finishedObserver);
        alg.addObserver(m_progressObserver);
        Poco::ActiveResult<bool> result = alg.executeAsync();
        TS_ASSERT( !result.available() )
        result.wait();
        TS_ASSERT( result.available() )
        TS_ASSERT( alg.isExecuted() )
        TS_ASSERT( startedNotificationReseived )
        TS_ASSERT( finishedNotificationReseived )
        TS_ASSERT_EQUALS( count, NofLoops )
        TS_ASSERT_EQUALS( alg.result, NofLoops-1 )
    }

    void testCancel()
    {
        finishedNotificationReseived = false;
        AsyncAlgorithm alg;
        alg.addObserver(m_startedObserver);
        alg.addObserver(m_finishedObserver);
        alg.addObserver(m_progressObserver);
        alg.initialize();
        Poco::ActiveResult<bool> result = alg.executeAsync();
        alg.cancel();
        result.wait();
        TS_ASSERT( !alg.isExecuted() )
        TS_ASSERT_LESS_THAN( alg.result, NofLoops-1 )
        TS_ASSERT( !finishedNotificationReseived )
    }

    void testException()
    {
        finishedNotificationReseived = false;
        AsyncAlgorithm alg;
        alg.addObserver(m_startedObserver);
        alg.addObserver(m_finishedObserver);
        alg.addObserver(m_progressObserver);
        alg.addObserver(m_errorObserver);
        alg.initialize();
        alg.throw_exception = true;
        Poco::ActiveResult<bool> result = alg.executeAsync();
        result.wait();
        TS_ASSERT( !alg.isExecuted() )
        TS_ASSERT_LESS_THAN( alg.result, NofLoops-1 )
        TS_ASSERT( !finishedNotificationReseived )
        TS_ASSERT( errorNotificationReseived )
    }

};

 

#endif /*ASYNCHRONOUSTEST_H_*/
