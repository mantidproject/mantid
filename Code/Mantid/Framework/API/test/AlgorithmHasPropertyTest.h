#ifndef MANTID_API_ALGORITHMHASPROPERTYTEST_H_
#define MANTID_API_ALGORITHMHASPROPERTYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/AlgorithmHasProperty.h"
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/BoundedValidator.h"

using namespace Mantid::API;

class AlgorithmHasPropertyTest : public CxxTest::TestSuite
{
private:
  
  class AlgorithmWithWorkspace : public Algorithm
  {
  public:
    const std::string name() const { return "AlgorithmWithWorkspace";}
    int version() const  { return 1;}
    const std::string category() const { return "Cat";} 
    
    void init()
    { 
      declareProperty("OutputWorkspace","");
    }
    void exec() {}
    
  };

  class AlgorithmWithNoWorkspace : public Algorithm
  {
  public:
    const std::string name() const { return "AlgorithmWithNoWorkspace";}
    int version() const  { return 1;}
    const std::string category() const { return "Cat";} 
    
    void init()
    { 
      declareProperty("NotOutputWorkspace","");
    }
    void exec() {}
  };
  
  class AlgorithmWithInvalidProperty : public Algorithm
  {
  public:
    const std::string name() const { return "AlgorithmWithInvalidProperty";}
    int version() const  { return 1; }
    const std::string category() const { return "Cat";} 
    
    void init()
    { 
      Mantid::Kernel::BoundedValidator<int> * lower = new Mantid::Kernel::BoundedValidator<int>();
      lower->setLower(0);
      declareProperty("OutputValue",-1, lower);
    }
    void exec() {}
  };


public:

  void test_Algorithm_With_Correct_Property_Is_Valid()
  {
    AlgorithmHasProperty *check = new AlgorithmHasProperty("OutputWorkspace");
    IAlgorithm_sptr tester(new AlgorithmWithWorkspace);
    tester->initialize();
    tester->execute();

    TS_ASSERT_EQUALS(check->isValid(tester), "");
  }

  void test_Algorithm_Without_Property_Is_Invalid()
  {
    AlgorithmHasProperty *check = new AlgorithmHasProperty("OutputWorkspace");
    IAlgorithm_sptr tester(new AlgorithmWithNoWorkspace);
    tester->initialize();
    tester->execute();

    TS_ASSERT_EQUALS(check->isValid(tester), 
		     "Algorithm object does not have the required property \"OutputWorkspace\"");
  }

  void test_Algorithm_With_Invalid_Property_Is_Invalid()
  {
    AlgorithmHasProperty *check = new AlgorithmHasProperty("OutputValue");
    IAlgorithm_sptr tester(new AlgorithmWithInvalidProperty);
    tester->initialize();

    TS_ASSERT_EQUALS(check->isValid(tester),
		     "Algorithm object contains the required property \"OutputValue\" but "
		     "it has an invalid value: -1");
  }
  

};


#endif /* MANTID_API_ALGORITHMHASPROPERTY_H_ */

