#ifndef ALGORITHMPARAMETERTEST_H_
#define ALGORITHMPARAMETERTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/AlgorithmParameter.h"
#include "MantidAPI/WorkspaceProperty.h"
#include <ostream>

using namespace Mantid::API;



class AlgorithmParameterTest : public CxxTest::TestSuite
{
public:

  void testPopulate()
  {
    // Not really much to test 
    AlgorithmParameter AP("arg1_param","20","argument",true,Mantid::Kernel::Direction::Input);
    TS_ASSERT_THROWS_NOTHING(std::cout<<std::endl << AP <<std::endl;)
  }


};

#endif /* ALGORITHMPARAMETERTEST_H_*/
