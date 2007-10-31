#ifndef ALGORITHMMANAGERTEST_H_
#define ALGORITHMMANAGERTEST_H_

#include <cxxtest/TestSuite.h>

#include "../inc/FrameworkManager.h"
#include "../inc/Algorithm.h"
#include <stdexcept>

using namespace Mantid;

namespace Mantid
{

class ToyAlgorithm : public Algorithm
{
public:
  ToyAlgorithm() 
  {
	int a=1;
  }
  virtual ~ToyAlgorithm() {}

  StatusCode ToyAlgorithm::exec()
  {
	  int b=100;
	  std::cout<< b << std::endl;
	  return StatusCode::SUCCESS;
  }


};

}
DECLARE_ALGORITHM(ToyAlgorithm)

class AlgorithmManagerTest : public CxxTest::TestSuite
{
public:

	void testInitialize()
	{
	  // Not really much to test
		TS_ASSERT_THROWS_NOTHING( Algmanager.Instance() )
	}

	void test_global_Mantid_AlgorithmManager_createAlgorithm()
	{
	  TS_ASSERT_THROWS_NOTHING( Algmanager.createAlgorithm("ToyAlgorithm") )
	  TS_ASSERT_THROWS( Algmanager.createAlgorithm("aaaaaa"), std::runtime_error )
      TS_ASSERT_THROWS( Algmanager.createAlgorithm("ToyAlgorithm","p1:p2:p3"), std::runtime_error )
	}

	void test_global_Mantid_AlgorithmManager_createAlgorithmWithProps()
	{
	  IAlgorithm *alg = Algmanager.createAlgorithm("ToyAlgorithm","Prop:Val,P2:V2");
	  std::string prop;
	  StatusCode status = alg->getProperty("Prop",prop);
	  TS_ASSERT ( ! status.isFailure() )
	  TS_ASSERT ( ! prop.compare("Val") )
    status = alg->getProperty("P2",prop);
    TS_ASSERT ( ! status.isFailure() )
    TS_ASSERT ( ! prop.compare("V2") )
	  
	}




private:
  AlgorithmManager Algmanager;
	
};

#endif /* ALGORITHMMANAGERTEST_H_*/