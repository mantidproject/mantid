#ifndef ALGORITHMHISTORYTEST_H_
#define ALGORITHMHISTORYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/AlgorithmHistory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include <sstream>

using namespace Mantid::API;



class AlgorithmHistoryTest : public CxxTest::TestSuite
{
public:

  void testPopulate()
  {
    std::string correctOutput = "Name : testalg\n";
    correctOutput = correctOutput + "Version: 1\n";
    correctOutput = correctOutput + "Execution Date: 2008-Feb-29 09:54:49\n";
    correctOutput = correctOutput + "Execution Duration: 14 seconds\n";
    correctOutput = correctOutput + "Parameters:\n";
    correctOutput = correctOutput + "\n";
    correctOutput = correctOutput + "  Name : arg1_param\n";
    correctOutput = correctOutput + "  Value: 20\n";
    correctOutput = correctOutput + "  Type: argument\n";
    correctOutput = correctOutput + "  isDefault: 1\n";
    correctOutput = correctOutput + "  Direction :Input\n";
    correctOutput = correctOutput + "\n";
    correctOutput = correctOutput + "  Name : arg2_param\n";
    correctOutput = correctOutput + "  Value: 23\n";
    correctOutput = correctOutput + "  Type: argument\n";
    correctOutput = correctOutput + "  isDefault: 1\n";
    correctOutput = correctOutput + "  Direction :Inout\n";

    //set the time
    std::time_t rawtime;
    std::tm * timeinfo = new std::tm;
    timeinfo->tm_isdst = -1;

    /* The datetime must match that in the strng above */
    std::time ( &rawtime );
    timeinfo->tm_year = 108;
    timeinfo->tm_mon = 1;
    timeinfo->tm_mday = 29;
    timeinfo->tm_hour = 9;
    timeinfo->tm_min = 54;
    timeinfo->tm_sec = 49;
    std::time_t execTime = mktime ( timeinfo );

    // Not really much to test
    std::vector<AlgorithmParameter> aps;
    aps.push_back(AlgorithmParameter("arg1_param","20","argument",true,Mantid::Kernel::Direction::Input));
    aps.push_back(AlgorithmParameter("arg2_param","23","argument",true,Mantid::Kernel::Direction::InOut));

    AlgorithmHistory AH("testalg",1,execTime,14.0,aps);
    //dump output to sting
    std::ostringstream output;
    output.exceptions( std::ios::failbit | std::ios::badbit );
    TS_ASSERT_THROWS_NOTHING(output << AH);
    TS_ASSERT_EQUALS(output.str(),correctOutput);

  }


};

#endif /* ALGORITHMHISTORYTEST_H_*/
