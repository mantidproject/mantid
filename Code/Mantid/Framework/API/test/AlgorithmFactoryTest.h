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
    Mantid::Kernel::Instantiator<ToyAlgorithmTwo, Algorithm>* newTwo = new Mantid::Kernel::Instantiator<ToyAlgorithmTwo, Algorithm>;
    
    //get the nubmer of algorithms it already has
    std::vector<std::string> keys = AlgorithmFactory::Instance().getKeys();
    int noOfAlgs = keys.size();

    TS_ASSERT_THROWS_NOTHING(AlgorithmFactory::Instance().subscribe<ToyAlgorithm>());
    TS_ASSERT_THROWS_NOTHING(AlgorithmFactory::Instance().subscribe(newTwo));

    TS_ASSERT_THROWS_ANYTHING(AlgorithmFactory::Instance().subscribe<ToyAlgorithm>());
   
    //get the nubmer of algorithms it has now
    keys = AlgorithmFactory::Instance().getKeys();
    int noOfAlgsAfter = keys.size();
    TS_ASSERT_EQUALS(noOfAlgsAfter, noOfAlgs + 2);

    AlgorithmFactory::Instance().unsubscribe("ToyAlgorithm",1);
    AlgorithmFactory::Instance().unsubscribe("ToyAlgorithm",2);
  }

  void testUnsubscribe()
  {
    Mantid::Kernel::Instantiator<ToyAlgorithmTwo, Algorithm>* newTwo = new Mantid::Kernel::Instantiator<ToyAlgorithmTwo, Algorithm>;

    //get the nubmer of algorithms it already has
    std::vector<std::string> keys = AlgorithmFactory::Instance().getKeys();
    int noOfAlgs = keys.size();

    AlgorithmFactory::Instance().subscribe<ToyAlgorithm>();
    AlgorithmFactory::Instance().subscribe(newTwo);
   
    TS_ASSERT_THROWS_NOTHING(AlgorithmFactory::Instance().unsubscribe("ToyAlgorithm",1));
    TS_ASSERT_THROWS_NOTHING(AlgorithmFactory::Instance().unsubscribe("ToyAlgorithm",2));
    
    //get the nubmer of algorithms it has now
    keys = AlgorithmFactory::Instance().getKeys();
    int noOfAlgsAfter = keys.size();

    TS_ASSERT_EQUALS(noOfAlgsAfter, noOfAlgs)

    //try unsubscribing them again
    TS_ASSERT_THROWS_NOTHING(AlgorithmFactory::Instance().unsubscribe("ToyAlgorithm",1));
    TS_ASSERT_THROWS_NOTHING(AlgorithmFactory::Instance().unsubscribe("ToyAlgorithm",2));
    
    //make sure the number hasn't changed
    keys = AlgorithmFactory::Instance().getKeys();
    int noOfAlgsAgain = keys.size();

    TS_ASSERT_EQUALS(noOfAlgsAfter, noOfAlgsAgain);
  }
  
  void testExists()
  {
    Mantid::Kernel::Instantiator<ToyAlgorithmTwo, Algorithm>* newTwo = new Mantid::Kernel::Instantiator<ToyAlgorithmTwo, Algorithm>;
    
    AlgorithmFactory::Instance().subscribe<ToyAlgorithm>();
    AlgorithmFactory::Instance().subscribe(newTwo);
    
    TS_ASSERT(AlgorithmFactory::Instance().exists("ToyAlgorithm",1));
    TS_ASSERT(AlgorithmFactory::Instance().exists("ToyAlgorithm",2));
    TS_ASSERT(!AlgorithmFactory::Instance().exists("ToyAlgorithm",3));
    TS_ASSERT(!AlgorithmFactory::Instance().exists("ToyAlgorithm",4));
    TS_ASSERT(AlgorithmFactory::Instance().exists("ToyAlgorithm",-1));

    AlgorithmFactory::Instance().unsubscribe("ToyAlgorithm",1);
    AlgorithmFactory::Instance().unsubscribe("ToyAlgorithm",2);
  }
  
  void testGetKeys()
  {
    std::vector<std::string> keys;

    TS_ASSERT_EQUALS(0, keys.size());
    AlgorithmFactory::Instance().subscribe<ToyAlgorithm>();

    TS_ASSERT_THROWS_NOTHING(keys = AlgorithmFactory::Instance().getKeys());
    int noOfAlgs = keys.size();
    TS_ASSERT(noOfAlgs > 1);

    AlgorithmFactory::Instance().unsubscribe("ToyAlgorithm",1);

    TS_ASSERT_THROWS_NOTHING(keys = AlgorithmFactory::Instance().getKeys());
    TS_ASSERT_EQUALS(noOfAlgs - 1, keys.size());
  }
  
  void testCreate()
  {
    Mantid::Kernel::Instantiator<ToyAlgorithmTwo, Algorithm>* newTwo = new Mantid::Kernel::Instantiator<ToyAlgorithmTwo, Algorithm>;
    
    AlgorithmFactory::Instance().subscribe<ToyAlgorithm>();
    AlgorithmFactory::Instance().subscribe(newTwo);
    
    TS_ASSERT_THROWS_NOTHING(AlgorithmFactory::Instance().create("ToyAlgorithm",-1));
    TS_ASSERT_THROWS_ANYTHING(AlgorithmFactory::Instance().create("AlgorithmDoesntExist",-1));

    TS_ASSERT_THROWS_NOTHING(AlgorithmFactory::Instance().create("ToyAlgorithm",1));
    TS_ASSERT_THROWS_NOTHING(AlgorithmFactory::Instance().create("ToyAlgorithm",2));
    TS_ASSERT_THROWS_ANYTHING(AlgorithmFactory::Instance().create("AlgorithmDoesntExist",1));
    TS_ASSERT_THROWS_ANYTHING(AlgorithmFactory::Instance().create("AlgorithmDoesntExist",2));

    TS_ASSERT_THROWS_ANYTHING(AlgorithmFactory::Instance().create("",1));
    TS_ASSERT_THROWS_ANYTHING(AlgorithmFactory::Instance().create("",-1));

    TS_ASSERT_THROWS_ANYTHING(AlgorithmFactory::Instance().create("ToyAlgorithm",3));
    TS_ASSERT_THROWS_ANYTHING(AlgorithmFactory::Instance().create("ToyAlgorithm",4));

    AlgorithmFactory::Instance().unsubscribe("ToyAlgorithm",1);
    AlgorithmFactory::Instance().unsubscribe("ToyAlgorithm",2);
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