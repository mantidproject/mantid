#ifndef ALGORITHMMANAGERTEST_H_
#define ALGORITHMMANAGERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Algorithm.h"
#include <stdexcept>

using namespace Mantid::Kernel;

class algmantest : public Algorithm
{
   public:
  
	algmantest() : Algorithm() {}
	virtual ~algmantest() {}
	StatusCode init() { return StatusCode::SUCCESS; }
	StatusCode exec() { return StatusCode::SUCCESS; }
	StatusCode final() { return StatusCode::SUCCESS; }		
};

class algmantestSecond : public Algorithm
{
   public:
  
	algmantestSecond() : Algorithm() {}
	virtual ~algmantestSecond() {}
	StatusCode init() { return StatusCode::SUCCESS; }
	StatusCode exec() { return StatusCode::SUCCESS; }
	StatusCode final() { return StatusCode::SUCCESS; }		
};

 DECLARE_ALGORITHM(algmantest)
 DECLARE_ALGORITHM(algmantestSecond)

using namespace Mantid::Kernel;

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
               IAlgorithm *alg;
               TS_ASSERT_THROWS_NOTHING( alg = manager->create("AlgorithmManager::myAlg") );
               TS_ASSERT_DIFFERS(dynamic_cast<algmantest*>(alg),static_cast<algmantest*>(0));
		TS_ASSERT_THROWS_NOTHING( alg = manager->create("AlgorithmManager::myAlgB") );
                TS_ASSERT_DIFFERS(dynamic_cast<algmantestSecond*>(alg),static_cast<algmantestSecond*>(0));
	        TS_ASSERT_DIFFERS(dynamic_cast<Algorithm*>(alg),static_cast<Algorithm*>(0));
		TS_ASSERT_EQUALS(manager->size(),2);   // To check that crea is called on local objects
	   }
	   
	   void testManagedType()
	   {
		 manager->clear();
		 IAlgorithm *Aptr, *Bptr;
		 Aptr=manager->create("algmantest");
		 Bptr=manager->createUnmanaged("algmantest");
		 TS_ASSERT_DIFFERS(Aptr,Bptr);
		 TS_ASSERT_EQUALS(manager->size(),1);
		TS_ASSERT_DIFFERS(Aptr,static_cast<IAlgorithm*>(0));
		TS_ASSERT_DIFFERS(Bptr,static_cast<IAlgorithm*>(0));
		delete Bptr;
	   }
  
  
private:
	
  AlgorithmManager *manager;
	
};

#endif /* ALGORITHMMANAGERTEST_H_*/
