#ifndef ALGORITHMPROXYTEST_H_
#define ALGORITHMPROXYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmProxy.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmObserver.h"


using namespace Mantid::API;
using namespace Mantid::Kernel;

class ToyAlgorithmProxy : public Algorithm
{
public:
  ToyAlgorithmProxy() : Algorithm() {}
  virtual ~ToyAlgorithmProxy() {}
  const std::string name() const { return "ToyAlgorithmProxy";} ///< Algorithm's name for identification
  int version() const  { return 1;}                        ///< Algorithm's version for identification
  const std::string category() const { return "ProxyCat";}           ///< Algorithm's category for identification
  const std::string alias() const { return "Dog";}            ///< Algorithm's alias
  const std::string workspaceMethodName() const { return "toyalgorithm"; }
  const std::string workspaceMethodOnTypes() const { return "MatrixWorkspace;ITableWorkspace"; }
  const std::string workspaceMethodInputProperty() const { return "InputWorkspace"; }

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

class ToyAlgorithmProxyMultipleCategory : public Algorithm
{
public:
  ToyAlgorithmProxyMultipleCategory() : Algorithm() {}
  virtual ~ToyAlgorithmProxyMultipleCategory() {}
  const std::string name() const { return "ToyAlgorithmProxyMultipleCategory";} ///< Algorithm's name for identification
  int version() const  { return 1;}                        ///< Algorithm's version for identification
  const std::string category() const { return "ProxyCat;ProxyLeopard";}           ///< Algorithm's category for identification
  const std::string alias() const { return "Dog";}            ///< Algorithm's alias

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
DECLARE_ALGORITHM(ToyAlgorithmProxyMultipleCategory)

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
        TS_ASSERT_EQUALS( alg->category() , "ProxyCat" );
        TS_ASSERT_EQUALS( alg->alias(), "Dog");
        TS_ASSERT( alg->isInitialized() );
        TS_ASSERT( alg->existsProperty("prop1") );
        TS_ASSERT( alg->existsProperty("prop2") );
        TS_ASSERT( !alg->isRunning() );
        alg->setProperty("prop1","stuff");
        alg->setProperty("prop2",17);
        TS_ASSERT_THROWS_NOTHING( alg->execute() );
        TS_ASSERT( alg->isExecuted() );
        int out = alg->getProperty("out");
        TS_ASSERT_EQUALS(out,28);
    }

    void testMultipleCategory()
    {
        IAlgorithm_sptr alg = AlgorithmManager::Instance().create("ToyAlgorithmProxyMultipleCategory");
        TS_ASSERT( dynamic_cast<AlgorithmProxy*>(alg.get()) );
        TS_ASSERT_EQUALS( alg->name() , "ToyAlgorithmProxyMultipleCategory" );
        TS_ASSERT_EQUALS( alg->version() , 1 );
        TS_ASSERT_EQUALS( alg->category() , "ProxyCat;ProxyLeopard" );
        std::vector<std::string> result;
        result.push_back("ProxyCat");
        result.push_back("ProxyLeopard");
        TS_ASSERT_EQUALS( alg->categories() , result );
        TS_ASSERT_EQUALS( alg->alias(), "Dog");
        TS_ASSERT( alg->isInitialized() );
    }

    /**
     * Disabled due to random failures that cannot be pinned down and are most likely timing issues.
     * This test has never failed legitimately and only serves to cause confusion when it fails
     * due to completely unrelated changes.
     */
    void xtestRunning()
    {
        IAlgorithm_sptr alg = AlgorithmManager::Instance().create("ToyAlgorithmProxy");
        TS_ASSERT( dynamic_cast<AlgorithmProxy*>(alg.get()) );
        alg->setProperty("prop1","stuff");
        alg->setProperty("prop2",17);
        Poco::ActiveResult<bool> res = alg->executeAsync();
        res.tryWait(60);
        TS_ASSERT( alg->isRunning() );

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

    void test_WorkspaceMethodFunctionsReturnProxiedContent()
    {
      IAlgorithm_sptr alg = AlgorithmManager::Instance().create("ToyAlgorithmProxy");

      TS_ASSERT_EQUALS("toyalgorithm", alg->workspaceMethodName());
      
      auto types = alg->workspaceMethodOn();
      TS_ASSERT_EQUALS(2, types.size());
      if(types.size() == 2)
      {
        TS_ASSERT_EQUALS("MatrixWorkspace", types[0]);
        TS_ASSERT_EQUALS("ITableWorkspace", types[1]);
      }
      TS_ASSERT_EQUALS("InputWorkspace", alg->workspaceMethodInputProperty());
    }

};

#endif /*ALGORITHMPROXYTEST_H_*/
