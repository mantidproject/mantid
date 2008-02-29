#ifndef ENVIRONMENTHISTORYTEST_H_
#define ENVIRONMENTHISTORYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/EnvironmentHistory.h"
#include <sstream>

using namespace Mantid::API;



class EnvironmentHistoryTest : public CxxTest::TestSuite
{
public:

  void testPopulate()
  {
    std::string correctOutput = "Framework Version : 1\n";
    correctOutput = correctOutput + "OS name: MS Windows\n";
    correctOutput = correctOutput + "OS version: XP Pro\n";
    correctOutput = correctOutput + "username: isiscnc\\jdmc43\n";

    // Not really much to test 
    EnvironmentHistory EH("1","MS Windows","XP Pro","isiscnc\\jdmc43");

    //dump output to sting
    std::ostringstream output;
    TS_ASSERT_THROWS_NOTHING(output << EH);
    TS_ASSERT_EQUALS(output.str(),correctOutput);
  }
};

#endif /* ALGORITHMPARAMETERTEST_H_*/
