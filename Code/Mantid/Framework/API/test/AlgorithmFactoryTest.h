#ifndef ALGORITHMFACTORYTEST_H_
#define ALGORITHMFACTORYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/AlgorithmFactory.h"
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
  
  void testCreate()
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