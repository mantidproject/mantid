#ifndef FRAMEWORKMANAGERTEST_H_
#define FRAMEWORKMANAGERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Algorithm.h"
#include <stdexcept>

using namespace Mantid::Kernel;
using namespace Mantid::API;

class ToyAlgorithm2 : public Algorithm
{
public:
  ToyAlgorithm2() {}
  virtual ~ToyAlgorithm2() {}
  StatusCode init()
  { declareProperty("Prop","");
    declareProperty("P2","");
    declareProperty("Filename","");
    return StatusCode::SUCCESS; 
  }
  StatusCode exec() { return StatusCode::SUCCESS; }
  StatusCode final() { return StatusCode::SUCCESS; }
};

DECLARE_ALGORITHM(ToyAlgorithm2)

using namespace Mantid;

class FrameworkManagerTest : public CxxTest::TestSuite
{
public:

  void testInitialize()
  {
    // Not really much to test
    TS_ASSERT_THROWS_NOTHING( manager.initialize() )
  }

  void testcreateAlgorithm()
  {
    TS_ASSERT_THROWS_NOTHING( manager.createAlgorithm("ToyAlgorithm2") )
    TS_ASSERT_THROWS( manager.createAlgorithm("aaaaaa"), std::runtime_error )
  }

  void testcreateAlgorithmWithProps()
  {
    IAlgorithm *alg = manager.createAlgorithm("ToyAlgorithm2","Prop:Val,P2:V2");
    std::string prop;
    TS_ASSERT_THROWS_NOTHING( prop = alg->getPropertyValue("Prop") )
	  TS_ASSERT( ! prop.compare("Val") )
	  TS_ASSERT_THROWS_NOTHING( prop = alg->getPropertyValue("P2") )
	  TS_ASSERT( ! prop.compare("V2") )
	  
    TS_ASSERT_THROWS_NOTHING( manager.createAlgorithm("ToyAlgorithm2","") )
//    TS_ASSERT_THROWS_NOTHING( manager.createAlgorithm("ToyAlgorithm2","noValProp") )
    TS_ASSERT_THROWS( manager.createAlgorithm("ToyAlgorithm2","p1:p2:p3"), std::invalid_argument )
  }

  void testExec()
  {
    IAlgorithm *alg = manager.exec("ToyAlgorithm2","Prop:Val,P2:V2");
    TS_ASSERT( alg->isExecuted() )
  }

  void testGetWorkspace()
  {
    TS_ASSERT_THROWS( manager.getWorkspace("wrongname"), std::runtime_error )
  }

private:
  FrameworkManager manager;
	
};

#endif /*FRAMEWORKMANAGERTEST_H_*/
