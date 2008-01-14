#ifndef ALGORITHMMANAGERTEST_H_
#define ALGORITHMMANAGERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Algorithm.h"
#include <stdexcept>

using namespace Mantid::API;

class algmantest : public Algorithm
{
public:

  algmantest() : Algorithm() {}
  virtual ~algmantest() {}
  void init() { }
  void exec() {  }
};

class algmantestSecond : public Algorithm
{
public:

  algmantestSecond() : Algorithm() {}
  virtual ~algmantestSecond() {}
  void init() { }
  void exec() { }
};

DECLARE_ALGORITHM(algmantest)
DECLARE_ALGORITHM(algmantestSecond)

class AlgorithmManagerTest : public CxxTest::TestSuite
{
public:

  AlgorithmManagerTest() 
  {
    manager = AlgorithmManager::Instance();
  }

  void testInstance()
  {
    // Not really much to test
    AlgorithmManager *tester = AlgorithmManager::Instance();
    TS_ASSERT_EQUALS( manager, tester);
    TS_ASSERT_THROWS_NOTHING( manager->create("algmantest") )
    TS_ASSERT_THROWS( manager->create("aaaaaa"), std::runtime_error )
  }

  void testClear()
  {
    manager->clear();
    manager->subscribe<algmantest>("AlgorithmManager::myAlgclear");
    manager->subscribe<algmantestSecond>("AlgorithmManager::myAlgBclear");
    TS_ASSERT_THROWS_NOTHING( manager->create("AlgorithmManager::myAlgBclear") );
    TS_ASSERT_THROWS_NOTHING( manager->create("AlgorithmManager::myAlgBclear") );
    TS_ASSERT_EQUALS(manager->size(),2);
    manager->clear();
    TS_ASSERT_EQUALS(manager->size(),0);
  }

  void testReturnType()
  {
    manager->clear();
    manager->subscribe<algmantest>("AlgorithmManager::myAlg");
    manager->subscribe<algmantestSecond>("AlgorithmManager::myAlgB");
    Algorithm_sptr alg;
    TS_ASSERT_THROWS_NOTHING( alg = manager->create("AlgorithmManager::myAlg") );
    TS_ASSERT_DIFFERS(dynamic_cast<algmantest*>(alg.get()),static_cast<algmantest*>(0));
    TS_ASSERT_THROWS_NOTHING( alg = manager->create("AlgorithmManager::myAlgB") );
    TS_ASSERT_DIFFERS(dynamic_cast<algmantestSecond*>(alg.get()),static_cast<algmantestSecond*>(0));
    TS_ASSERT_DIFFERS(dynamic_cast<Algorithm*>(alg.get()),static_cast<Algorithm*>(0));
    TS_ASSERT_EQUALS(manager->size(),2);   // To check that crea is called on local objects
  }

  void testManagedType()
  {
    manager->clear();
    Algorithm_sptr Aptr, Bptr;
    Aptr=manager->create("algmantest");
    Bptr=manager->createUnmanaged("algmantest");
    TS_ASSERT_DIFFERS(Aptr,Bptr);
    TS_ASSERT_EQUALS(manager->size(),1);
    TS_ASSERT_DIFFERS(Aptr.get(),static_cast<Algorithm*>(0));
    TS_ASSERT_DIFFERS(Bptr.get(),static_cast<Algorithm*>(0));
  }


private:

  AlgorithmManager *manager;

};

#endif /* ALGORITHMMANAGERTEST_H_*/
