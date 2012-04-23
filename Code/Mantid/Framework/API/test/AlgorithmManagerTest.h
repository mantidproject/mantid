#ifndef AlgorithmManagerTest_H_
#define AlgorithmManagerTest_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmProxy.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/ConfigService.h"
#include <stdexcept>
#include <vector>
#include <Poco/Thread.h>

using namespace Mantid::API;

class AlgTest : public Algorithm
{
public:

  AlgTest() : Algorithm() {}
  virtual ~AlgTest() {}
  void init() { }
  void exec() { }
  virtual const std::string name() const {return "AlgTest";}
  virtual int version() const {return(1);}
  virtual const std::string category() const {return("Cat1");}
};

class AlgTestFail : public Algorithm
{
public:

  AlgTestFail() : Algorithm() {}
  virtual ~AlgTestFail() {}
  void init() { }
  void exec() { }
  virtual const std::string name() const {return "AlgTest";}
  virtual int version() const {return(1);}
  virtual const std::string category() const {return("Cat2");}

};

class AlgTestPass : public Algorithm
{
public:

  AlgTestPass() : Algorithm() {}
  virtual ~AlgTestPass() {}
  void init() { }
  void exec() { }
  virtual const std::string name() const {return "AlgTest";}
  virtual int version() const {return(2);}
  virtual const std::string category() const {return("Cat4");}

};

class AlgTestSecond : public Algorithm
{
public:

  AlgTestSecond() : Algorithm() {}
  virtual ~AlgTestSecond() {}
  void init() { }
  void exec() { }
  virtual const std::string name() const {return "AlgTestSecond";}
  virtual int version() const {return(1);}
  virtual const std::string category() const {return("Cat3");}
};


/** Algorithm that runs until cancelled */
class AlgRunsForever : public Algorithm
{
public:
  AlgRunsForever() : Algorithm() {}
  virtual ~AlgRunsForever() {}
  void init() { }
  void exec()
  {
    while (!this->m_cancel)
      Poco::Thread::sleep(10);
  }
  virtual const std::string name() const {return "AlgRunsForever";}
  virtual int version() const {return(1);}
  virtual const std::string category() const {return("Cat1");}
};


DECLARE_ALGORITHM(AlgTest)
DECLARE_ALGORITHM(AlgRunsForever)
DECLARE_ALGORITHM(AlgTestSecond)

class AlgorithmManagerTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AlgorithmManagerTest *createSuite() { return new AlgorithmManagerTest(); }
  static void destroySuite( AlgorithmManagerTest *suite ) { delete suite; }

  AlgorithmManagerTest() 
  {
    // A test fails unless algorithms.retained is big enough
    Mantid::Kernel::ConfigService::Instance().setString("algorithms.retained","5");
  }

  void testVersionFail()
  {
    const size_t nalgs = AlgorithmFactory::Instance().getKeys().size();
    TS_ASSERT_THROWS_NOTHING(AlgorithmFactory::Instance().subscribe<AlgTestFail>());
    // Size should be the same
    TS_ASSERT_EQUALS(AlgorithmFactory::Instance().getKeys().size(), nalgs);
    
  }

 void testVersionPass()
  {
	TS_ASSERT_THROWS_NOTHING(AlgorithmFactory::Instance().subscribe<AlgTestPass>());
  }

  void testInstance()
  {
    // Not really much to test
    //AlgorithmManager *tester = AlgorithmManager::Instance();
    //TS_ASSERT_EQUALS( manager, tester);

    TS_ASSERT_THROWS_NOTHING( AlgorithmManager::Instance().create("AlgTest") );
    TS_ASSERT_THROWS(
    AlgorithmManager::Instance().create("AlgTest",3)
    , std::runtime_error );
    TS_ASSERT_THROWS(AlgorithmManager::Instance().create("aaaaaa"), std::runtime_error );
  }

  void testGetNamesAndCategories()
  {
	  AlgorithmManager::Instance().clear();
	  TS_ASSERT_THROWS_NOTHING( AlgorithmManager::Instance().create("AlgTest") );
	  TS_ASSERT_THROWS_NOTHING(AlgorithmManager::Instance().create("AlgTestSecond") );
    std::vector<std::pair<std::string,std::string> > names = AlgorithmManager::Instance().getNamesAndCategories();
	  TS_ASSERT_EQUALS(names.size(), 2);
    if (names.size() > 0)
	  {
      TS_ASSERT_EQUALS(names[0].first, "AlgTest");
      TS_ASSERT_EQUALS(names[0].second, "Cat4");
	  }
    if (names.size() > 1)
    {
      TS_ASSERT_EQUALS(names[1].first, "AlgTestSecond");
      TS_ASSERT_EQUALS(names[1].second, "Cat3");
    }

  }

  void testClear()
  {
    AlgorithmManager::Instance().clear();
    TS_ASSERT_THROWS_NOTHING( AlgorithmManager::Instance().create("AlgTest") );
    TS_ASSERT_THROWS_NOTHING(AlgorithmManager::Instance().create("AlgTestSecond") );
    TS_ASSERT_EQUALS(AlgorithmManager::Instance().size(),2);
    AlgorithmManager::Instance().clear();
    TS_ASSERT_EQUALS(AlgorithmManager::Instance().size(),0);
  }

  void testReturnType()
  {
    AlgorithmManager::Instance().clear();
    IAlgorithm_sptr alg;
    TS_ASSERT_THROWS_NOTHING( alg = AlgorithmManager::Instance().create("AlgTest",1) );
    TS_ASSERT_DIFFERS(dynamic_cast<AlgorithmProxy*>(alg.get()),static_cast<AlgorithmProxy*>(0));
    TS_ASSERT_THROWS_NOTHING( alg = AlgorithmManager::Instance().create("AlgTestSecond",1) );
    TS_ASSERT_DIFFERS(dynamic_cast<AlgorithmProxy*>(alg.get()),static_cast<AlgorithmProxy*>(0));
    TS_ASSERT_DIFFERS(dynamic_cast<IAlgorithm*>(alg.get()),static_cast<IAlgorithm*>(0));
    TS_ASSERT_EQUALS(AlgorithmManager::Instance().size(),2);   // To check that crea is called on local objects
  }

  void testManagedType()
  {
    AlgorithmManager::Instance().clear();
    IAlgorithm_sptr Aptr, Bptr;
    Aptr=AlgorithmManager::Instance().create("AlgTest");
    Bptr=AlgorithmManager::Instance().createUnmanaged("AlgTest");
    TS_ASSERT_DIFFERS(Aptr,Bptr);
    TS_ASSERT_EQUALS(AlgorithmManager::Instance().size(),1);
    TS_ASSERT_DIFFERS(Aptr.get(),static_cast<Algorithm*>(0));
    TS_ASSERT_DIFFERS(Bptr.get(),static_cast<Algorithm*>(0));
  }

  void testCreateNoProxy()
  {
    AlgorithmManager::Instance().clear();
    IAlgorithm_sptr Aptr, Bptr;
    Aptr=AlgorithmManager::Instance().create("AlgTest", -1, true);
    Bptr=AlgorithmManager::Instance().create("AlgTest", -1, false);
    TSM_ASSERT("Was created as a AlgorithmProxy", dynamic_cast<AlgorithmProxy*>(Aptr.get()) );
    TSM_ASSERT("Was NOT created as a AlgorithmProxy", dynamic_cast<AlgorithmProxy*>(Bptr.get())==NULL );
  }

  // This will be called back when an algo starts
  void handleAlgorithmStartingNotification(const Poco::AutoPtr<AlgorithmStartingNotification>& /*pNf*/)
  {
    m_notificationValue = 12345;
  }


  /** When running an algorithm in async mode, the
   * AlgorithmManager needs to send out a notification
   */
  void testStartingNotification()
  {
    AlgorithmManager::Instance().clear();
    Poco::NObserver<AlgorithmManagerTest, Mantid::API::AlgorithmStartingNotification>
      my_observer(*this, &AlgorithmManagerTest::handleAlgorithmStartingNotification);
    AlgorithmManager::Instance().notificationCenter.addObserver(my_observer);

    IAlgorithm_sptr Aptr, Bptr;
    Aptr=AlgorithmManager::Instance().create("AlgTest", -1, true);
    Bptr=AlgorithmManager::Instance().create("AlgTest", -1, false);

    m_notificationValue = 0;
    Poco::ActiveResult<bool> resB = Bptr->executeAsync();
    resB.wait();
    TSM_ASSERT_EQUALS( "Notification was received.", m_notificationValue, 12345 );

    m_notificationValue = 0;
    Poco::ActiveResult<bool> resA = Aptr->executeAsync();
    resA.wait();
    TSM_ASSERT_EQUALS( "Notification was received (proxy).", m_notificationValue, 12345 );

  }

  /** Keep the right number of algorithms in the list */
  void testDroppingOldOnes()
  {
    AlgorithmManager::Instance().clear();
    TS_ASSERT_EQUALS(AlgorithmManager::Instance().size(),0);

    IAlgorithm_sptr first = AlgorithmManager::Instance().create("AlgTest");
    // Fill up the list
    for (size_t i=1; i<5; i++)
      AlgorithmManager::Instance().create("AlgTest");
    TS_ASSERT_EQUALS(AlgorithmManager::Instance().size(),5);

    // The first one is at the front
    TS_ASSERT(AlgorithmManager::Instance().algorithms().front() == first);

    // Add one more, drops the oldest one
    AlgorithmManager::Instance().create("AlgTest");
    TS_ASSERT_EQUALS(AlgorithmManager::Instance().size(),5);
    TSM_ASSERT("The first(oldest) algorithm is gone",
        AlgorithmManager::Instance().algorithms().front() != first);
  }


  /** Keep one algorithm running, drop the second-oldest one etc. */
  void testDroppingOldOnes_whenAnAlgorithmIsStillRunning()
  {
    AlgorithmManager::Instance().clear();
    TS_ASSERT_EQUALS(AlgorithmManager::Instance().size(),0);

    // Start one algorithm that never stops
    IAlgorithm_sptr first = AlgorithmManager::Instance().create("AlgRunsForever");
    Poco::ActiveResult<bool> res1 = first->executeAsync();

    IAlgorithm_sptr second = AlgorithmManager::Instance().create("AlgTest");

    // Another long-running algo
    IAlgorithm_sptr third = AlgorithmManager::Instance().create("AlgRunsForever");
    Poco::ActiveResult<bool> res3 = third->executeAsync();

    // give it some time to start
    Poco::Thread::sleep(100);

    for (size_t i=3; i<5; i++)
      AlgorithmManager::Instance().create("AlgTest");
    TS_ASSERT_EQUALS(AlgorithmManager::Instance().size(),5);

    // The right ones are at the front
    TS_ASSERT( *(AlgorithmManager::Instance().algorithms().begin()+0) == first);
    TS_ASSERT( *(AlgorithmManager::Instance().algorithms().begin()+1) == second);
    TS_ASSERT( *(AlgorithmManager::Instance().algorithms().begin()+2) == third);

    // Add one more, drops the SECOND oldest one
    AlgorithmManager::Instance().create("AlgTest");
    TS_ASSERT_EQUALS(AlgorithmManager::Instance().size(), 5);

    TSM_ASSERT("The oldest algorithm (is still running) so it is still there",
        *(AlgorithmManager::Instance().algorithms().begin()+0) == first);
    TSM_ASSERT("The second oldest was popped, replaced with the 3rd",
        *(AlgorithmManager::Instance().algorithms().begin()+1) == third);

    // One more time
    AlgorithmManager::Instance().create("AlgTest");
    TS_ASSERT_EQUALS(AlgorithmManager::Instance().size(), 5);

    // The right ones are at the front
    TSM_ASSERT("The oldest algorithm (is still running) so it is still there",
        *(AlgorithmManager::Instance().algorithms().begin()+0) == first);
    TSM_ASSERT("The third algorithm (is still running) so it is still there",
        *(AlgorithmManager::Instance().algorithms().begin()+1) == third);

    // Cancel the long-running ones
    first->cancel();
    third->cancel();
    res1.wait();
    res3.wait();
  }

  /** Extreme case where your queue fills up and all algos are running */
  void testDroppingOldOnes_extremeCase()
  {
    AlgorithmManager::Instance().clear();
    std::vector<Poco::ActiveResult<bool>> results;
    std::vector<IAlgorithm_sptr> algs;
    for (size_t i=0; i<5; i++)
    {
      IAlgorithm_sptr alg = AlgorithmManager::Instance().create("AlgRunsForever");
      algs.push_back(alg);
      results.push_back(alg->executeAsync());
    }
    // give it some time to start
    Poco::Thread::sleep(100);

    TS_ASSERT_EQUALS(AlgorithmManager::Instance().size(), 5);
    AlgorithmManager::Instance().create("AlgTest");
    TS_ASSERT_EQUALS(AlgorithmManager::Instance().size(), 6);

    for (size_t i=0; i<5; i++)
    {
      algs[i]->cancel();
      results[i].wait();
    }
  }

  void testThreadSafety()
  {
    PARALLEL_FOR_NO_WSP_CHECK()
    for (int i=0; i < 5000; i++)
    {
      AlgorithmManager::Instance().create("AlgTest");
    }
  }


int m_notificationValue;
};

#endif /* AlgorithmManagerTest_H_*/
