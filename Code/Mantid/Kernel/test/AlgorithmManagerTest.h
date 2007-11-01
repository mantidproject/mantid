#ifndef ALGORITHMMANAGERTEST_H_
#define ALGORITHMMANAGERTEST_H_

#include <cxxtest/TestSuite.h>

#include "../inc/Algorithm.h"
#include <stdexcept>


namespace Mantid
{
	class algmantest : public Algorithm
	{
	public:
  
		algmantest() {}
		virtual ~algmantest() {}
		StatusCode init() { return StatusCode::SUCCESS; }
		StatusCode exec() { return StatusCode::SUCCESS; }
		StatusCode final() { return StatusCode::SUCCESS; }		
	};
}

DECLARE_ALGORITHM(algmantest)


using namespace Mantid;

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
	TS_ASSERT_THROWS_NOTHING( manager->createAlgorithm("algmantest") )
	TS_ASSERT_THROWS( manager->createAlgorithm("aaaaaa"), std::runtime_error )

	}


private:
  AlgorithmManager *manager;
	
};

#endif /* ALGORITHMMANAGERTEST_H_*/