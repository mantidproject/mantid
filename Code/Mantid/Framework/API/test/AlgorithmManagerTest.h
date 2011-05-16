#ifndef AlgorithmManagerTest_H_
#define AlgorithmManagerTest_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmProxy.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/ConfigService.h"
#include <stdexcept>
#include <vector>

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

DECLARE_ALGORITHM(AlgTest)
DECLARE_ALGORITHM(AlgTestSecond)

class AlgorithmManagerTest : public CxxTest::TestSuite
{
public:

  AlgorithmManagerTest() 
  {
    // A test fails unless algorithms.retained is big enough
    Mantid::Kernel::ConfigService::Instance().setString("algorithms.retained","100");
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
//    AlgorithmFactory::Instance().subscribe<AlgTest>();
//    AlgorithmFactory::Instance().subscribe<AlgTestSecond>();
    TS_ASSERT_THROWS_NOTHING( AlgorithmManager::Instance().create("AlgTest") );
    TS_ASSERT_THROWS_NOTHING(AlgorithmManager::Instance().create("AlgTestSecond") );
    TS_ASSERT_EQUALS(AlgorithmManager::Instance().size(),2);
    AlgorithmManager::Instance().clear();
    TS_ASSERT_EQUALS(AlgorithmManager::Instance().size(),0);
  }

  void testReturnType()
  {
    AlgorithmManager::Instance().clear();
//    AlgorithmFactory::Instance().subscribe<AlgTest>();
//    AlgorithmFactory::Instance().subscribe<AlgTestSecond>();
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

};

#endif /* AlgorithmManagerTest_H_*/
