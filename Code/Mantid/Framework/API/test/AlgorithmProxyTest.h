#ifndef ALGORITHMPROXYTEST_H_
#define ALGORITHMPROXYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmProxy.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmObserver.h"


using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

class ToyAlgorithmProxy : public Algorithm
{
public:
  ToyAlgorithmProxy() : Algorithm() {}
  virtual ~ToyAlgorithmProxy() {}
  const std::string name() const { return "ToyAlgorithmProxy";} ///< Algorithm's name for identification
  int version() const  { return 1;}                        ///< Algorithm's version for identification
  const std::string category() const { return "Cat";}           ///< Algorithm's category for identification

  void init()
  { 
      declareProperty("prop1","value");
      declareProperty("prop2",1);   
      declareProperty("out",8,Direction::Output);
  }
  void exec() 
  {
      std::string p1 = getProperty("prop1");
      int p2 = getProperty("prop2");

      Poco::Thread::current()->sleep(500);
      progress(0.333,"Running");
      interruption_point();
      Algorithm* alg = dynamic_cast<Algorithm*>( this );
      TS_ASSERT( alg );
      
      TS_ASSERT_EQUALS( p1, "stuff" );
      TS_ASSERT_EQUALS( p2, 17 );

      setProperty("out",28);
  }
  
};

DECLARE_ALGORITHM(ToyAlgorithmProxy)

class TestProxyObserver: public AlgorithmObserver
{
public:
    bool start,progress,finish;
    TestProxyObserver():AlgorithmObserver(),start(false),progress(false),finish(false){}
    TestProxyObserver(IAlgorithm_const_sptr alg):AlgorithmObserver(alg),start(false),progress(false),finish(false){}
    void startHandle(const IAlgorithm*)
    {
        start = true;
    }
    void progressHandle(const IAlgorithm* ,double p,const std::string& msg)
    {
        progress = true;
        TS_ASSERT_EQUALS( p, 0.333 );
        TS_ASSERT_EQUALS( msg, "Running" );
    }
    void finishHandle(const IAlgorithm* )
    {
        finish = true;
    }
};

class AlgorithmProxyTest : public CxxTest::TestSuite
{
public:
    void testCreateProxy()
    {
        IAlgorithm_sptr alg = AlgorithmManager::Instance().create("ToyAlgorithmProxy");
        TS_ASSERT( dynamic_cast<AlgorithmProxy*>(alg.get()) );
        TS_ASSERT_EQUALS( alg->name() , "ToyAlgorithmProxy" );
        TS_ASSERT_EQUALS( alg->version() , 1 );
        TS_ASSERT_EQUALS( alg->category() , "Cat" );
        TS_ASSERT( alg->isInitialized() );
        alg->setChild(true);
        TS_ASSERT( !alg->isChild() );
        TS_ASSERT( alg->existsProperty("prop1") );
        TS_ASSERT( alg->existsProperty("prop2") );
        TS_ASSERT( !alg->isRunning() );
        TS_ASSERT( !alg->isRunningAsync() );
        alg->setProperty("prop1","stuff");
        alg->setProperty("prop2",17);
        TS_ASSERT_THROWS_NOTHING( alg->execute() );
        TS_ASSERT( alg->isExecuted() );
        int out = alg->getProperty("out");
        TS_ASSERT_EQUALS(out,28);
    }
    void testRunning()
    {
        IAlgorithm_sptr alg = AlgorithmManager::Instance().create("ToyAlgorithmProxy");
        TS_ASSERT( dynamic_cast<AlgorithmProxy*>(alg.get()) );
        alg->setProperty("prop1","stuff");
        alg->setProperty("prop2",17);
        Poco::ActiveResult<bool> res = alg->executeAsync();
        res.tryWait(50);
        TS_ASSERT( alg->isRunning() );
        TS_ASSERT( alg->isRunningAsync() );
        res.wait();
        TS_ASSERT( res.data() );
        TS_ASSERT( alg->isExecuted() );
    }
    void testCancel()
    {
        IAlgorithm_sptr alg = AlgorithmManager::Instance().create("ToyAlgorithmProxy");
        TS_ASSERT( dynamic_cast<AlgorithmProxy*>(alg.get()) );
        alg->setProperty("prop1","stuff");
        alg->setProperty("prop2",17);
        Poco::ActiveResult<bool> res = alg->executeAsync();
        res.tryWait(100);
        alg->cancel();
        res.wait();
        TS_ASSERT( !alg->isExecuted() );
        int out = alg->getProperty("out");
        TS_ASSERT_EQUALS(out,8);
    }
    void testAddObserver()
    {
        IAlgorithm_sptr alg = AlgorithmManager::Instance().create("ToyAlgorithmProxy");
        TS_ASSERT( dynamic_cast<AlgorithmProxy*>(alg.get()) );
        alg->setProperty("prop1","stuff");
        alg->setProperty("prop2",17);
        TestProxyObserver obs(alg);
        Poco::ActiveResult<bool> res = alg->executeAsync();
        res.wait();
        TS_ASSERT( obs.start );
        TS_ASSERT( obs.progress );
        TS_ASSERT( obs.finish );
    }
};

#endif /*ALGORITHMPROXYTEST_H_*/
