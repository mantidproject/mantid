#ifndef ALGORITHMFACTORYTEST_H_
#define ALGORITHMFACTORYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidKernel/Instantiator.h"
#include "FakeAlgorithms.h"


class AlgorithmFactoryTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AlgorithmFactoryTest *createSuite() { return new AlgorithmFactoryTest(); }
  static void destroySuite( AlgorithmFactoryTest *suite ) { delete suite; }

  AlgorithmFactoryTest()
  {}

  ~AlgorithmFactoryTest()
  {}

  void testSubscribe()
  {
    TS_ASSERT_THROWS_NOTHING(AlgorithmFactory::Instance().subscribe<ToyAlgorithm>())
    Mantid::Kernel::Instantiator<ToyAlgorithmTwo, Algorithm>* newTwo = new Mantid::Kernel::Instantiator<ToyAlgorithmTwo, Algorithm>;
    TS_ASSERT_THROWS_NOTHING(AlgorithmFactory::Instance().subscribe(newTwo))

    TS_ASSERT_THROWS_ANYTHING(AlgorithmFactory::Instance().subscribe<ToyAlgorithm>())
    AlgorithmFactory::Instance().unsubscribe("ToyAlgorithm",1);
    AlgorithmFactory::Instance().unsubscribe("ToyAlgorithm",2);
  }

  void testUnsubscribe()
  {
  }
  
  void testCreate()
  {
  }
  
  void testExists()
  {
  }
  
  void testGetKeys()
  {
  }
  
  void testGetCategories()
  {
  }
  
  void testGetCategoriesWithState()
  {
  }
  
  void testGetDescriptors()
  {
  }
  
  void testDecodeName()
  {
  }


};

#endif