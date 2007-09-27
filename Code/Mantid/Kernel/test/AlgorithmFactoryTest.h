#ifndef ALGORITHMFACTORYTEST_H_
#define ALGORITHMFACTORYTEST_H_

#include <cxxtest/TestSuite.h>

#include "../inc/AlgorithmFactory.h"
#include "../inc/Algorithm.h"

class AlgorithmFactoryTest : public CxxTest::TestSuite
{
public: 

  void testSubscribe()
  {
    Mantid::AlgorithmFactory *factory = Mantid::AlgorithmFactory::Instance();
    Mantid::StatusCode status = factory->subscribe("myAlg", Mantid::ConcreteAlgorithmCreator<Mantid::Algorithm>::createInstance );   
    TS_ASSERT( ! status.isFailure() );
  }
  
  void testUnsubscribe()
  {
    Mantid::AlgorithmFactory *factory = Mantid::AlgorithmFactory::Instance();
    Mantid::StatusCode status = factory->subscribe("myAlg", Mantid::ConcreteAlgorithmCreator<Mantid::Algorithm>::createInstance );   
    status = factory->unsubscribe("myAlg");
    TS_ASSERT( ! status.isFailure() );
    TS_ASSERT( ! factory->existsAlgorithm("myAlg") );
  }
  
  void testCreateAlgorithm()
  {
    Mantid::AlgorithmFactory *factory = Mantid::AlgorithmFactory::Instance();
    Mantid::StatusCode status = factory->subscribe("myAlg", Mantid::ConcreteAlgorithmCreator<Mantid::Algorithm>::createInstance );   
    Mantid::IAlgorithm *theAlg;
    status = factory->createAlgorithm("myAlg", theAlg);
    TS_ASSERT( ! status.isFailure() );
    std::string theName = theAlg->name();
    TS_ASSERT( ! theName.compare("unknown") );
    status = factory->createAlgorithm("zzzzz", theAlg);
    TS_ASSERT( status.isFailure() );
  }
  
  void testExistsAlgorithm()
  {
    Mantid::AlgorithmFactory *factory = Mantid::AlgorithmFactory::Instance();
    Mantid::StatusCode status = factory->subscribe("myAlg", Mantid::ConcreteAlgorithmCreator<Mantid::Algorithm>::createInstance );   
    TS_ASSERT( factory->existsAlgorithm("myAlg") );
  }
  
};
  
#endif /*ALGORITHMFACTORYTEST_H_*/
