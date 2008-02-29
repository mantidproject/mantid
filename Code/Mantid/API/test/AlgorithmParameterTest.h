#ifndef ALGORITHMPARAMETERTEST_H_
#define ALGORITHMPARAMETERTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/AlgorithmParameter.h"
#include "MantidAPI/WorkspaceProperty.h"
#include <sstream>

using namespace Mantid::API;



class AlgorithmParameterTest : public CxxTest::TestSuite
{
public:

  void testPopulate()
  {
    std::string correctOutput = "Name : arg1_param\n";
    correctOutput = correctOutput + "Value: 20\n";
    correctOutput = correctOutput + "Type: argument\n";
    correctOutput = correctOutput + "isDefault: 1\n";
    correctOutput = correctOutput + "Direction :Input\n";

    // Not really much to test 
    AlgorithmParameter AP("arg1_param","20","argument",true,Mantid::Kernel::Direction::Input);

    //dump output to sting
    std::ostringstream output;
    TS_ASSERT_THROWS_NOTHING(output << AP);
    TS_ASSERT_EQUALS(output.str(),correctOutput);
  }


};

#endif /* ALGORITHMPARAMETERTEST_H_*/
