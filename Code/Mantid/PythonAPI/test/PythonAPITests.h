#ifndef MANTID_PYTHONAPITESTS_H_
#define MANTID_PYTHONAPITESTS_H_

#include <vector>

#include <cxxtest/TestSuite.h>

#include "MantidPythonAPI/PythonInterface.h"


using namespace Mantid::PythonAPI;

class PythonAPITest : public CxxTest::TestSuite
{

private:
	PythonInterface* inter;	

public: 

  PythonAPITest()
  {
	inter = new PythonInterface();
  }
  
  void testFrameworkInitialise()
  {
	TS_ASSERT_THROWS_NOTHING(inter->InitialiseFrameworkManager());
  }
  
  void xtestCreateAlgorithm()
  {
	TS_ASSERT(inter->CreateAlgorithm("HelloWorldAlgorithm"));
  }
  
  void xtestCreateAlgorithmNotFoundThrows()
  {
	TS_ASSERT_THROWS_ANYTHING(inter->CreateAlgorithm("Rubbish!"));
  }
  
  void xtestExecuteAlgorithm()
  {
	TS_ASSERT(inter->ExecuteAlgorithm("HelloWorldAlgorithm"));
  }
  
  void xtestExecuteAlgorithmNotFound()
  {
	TS_ASSERT(!inter->ExecuteAlgorithm("Rubbish!"));
  }
  
  void testLoadIsisRaw()
  {
	  int hists = inter->LoadIsisRawFile("../../../../Test/Data/HET15869.RAW", "TestWorkspace");
	  
	  TS_ASSERT_EQUALS(hists, 2584);
  }
  
  void testGetXdata()
  {
	  int hists = inter->LoadIsisRawFile("../../../../Test/Data/HET15869.RAW", "TestWorkspace");
	  
	  std::vector<double> data = inter->GetXData("TestWorkspace", 0);
	  
	  TS_ASSERT(!data.empty());	  
  }
  
  void testGetYdata()
  {
	  int hists = inter->LoadIsisRawFile("../../../../Test/Data/HET15869.RAW", "TestWorkspace");
	  
	  std::vector<double> data = inter->GetYData("TestWorkspace", 0);
	  
	  TS_ASSERT(!data.empty());	  
  }
  
};


#endif /*MANTID_PYTHONAPITESTS_H_*/
