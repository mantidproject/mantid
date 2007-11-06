#ifndef ALGORITHMFACTORYTEST_H_
#define ALGORITHMFACTORYTEST_H_

#include <cxxtest/TestSuite.h>

#include "../inc/AlgorithmFactory.h"
#include "../inc/Algorithm.h"

using namespace Mantid::Kernel;

class ToyAlg : public Algorithm
{
public:
  ToyAlg() {}
  virtual ~ToyAlg() {}
  StatusCode init() { return StatusCode::SUCCESS; }
  StatusCode exec() { return StatusCode::SUCCESS; }
  StatusCode final() { return StatusCode::SUCCESS; }
};

DECLARE_ALGORITHM(ToyAlg)

class AlgorithmFactoryTest : public CxxTest::TestSuite
{
public: 

  AlgorithmFactoryTest()
  {
    factory = AlgorithmFactory::Instance();
  }
  
  void testInstance()
  {
    AlgorithmFactory *tester = AlgorithmFactory::Instance();
    TS_ASSERT_EQUALS( factory, tester);
  }
  
  void testReturnType()
  {
    factory->subscribe<ToyAlg>("myAlg");
    IAlgorithm *alg;
    TS_ASSERT_THROWS_NOTHING( alg = factory->create("myAlg") );
    TS_ASSERT_THROWS_NOTHING( dynamic_cast<Algorithm*>(alg) );
  }
  
  void testCast()
  {
    TS_ASSERT_THROWS_NOTHING( dynamic_cast<DynamicFactory<IAlgorithm>*>(factory) );
  }
  
private:
  AlgorithmFactory *factory;
  
};
  
#endif /*ALGORITHMFACTORYTEST_H_*/
