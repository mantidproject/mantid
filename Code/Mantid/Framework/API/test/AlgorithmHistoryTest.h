#ifndef ALGORITHMHISTORYTEST_H_
#define ALGORITHMHISTORYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmHistory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include <sstream>

using namespace Mantid::API;
using namespace Mantid::Kernel;

// 'Empty' algorithm class for tests
class testalg : public Algorithm
{
public:
  testalg() : Algorithm() {}
  virtual ~testalg() {}
  const std::string name() const { return "testalg";} ///< Algorithm's name for identification
  int version() const  { return 1;} ///< Algorithm's version for identification
  const std::string category() const { return "Cat";} ///< Algorithm's category for identification

  void init()
  {
    declareProperty(new WorkspaceProperty<Workspace>("arg1_param","x",Direction::Input));
    declareProperty("arg2_param",23);
  }
  void exec() {}
};

class AlgorithmHistoryTest : public CxxTest::TestSuite
{
public:
  void testPopulate()
  {
    std::string correctOutput = "Algorithm: testalg ";
    correctOutput = correctOutput + "v1\n";
    correctOutput = correctOutput + "Execution Date: 2008-Feb-29 09:54:49\n";
    correctOutput = correctOutput + "Execution Duration: 14 seconds\n";
    correctOutput = correctOutput + "Parameters:\n";
    correctOutput = correctOutput + "  Name: arg1_param, ";
    correctOutput = correctOutput + "Value: 20, ";
    correctOutput = correctOutput + "Default?: No, ";
    correctOutput = correctOutput + "Direction: Input\n";
    correctOutput = correctOutput + "  Name: arg2_param, ";
    correctOutput = correctOutput + "Value: 23, ";
    correctOutput = correctOutput + "Default?: Yes, ";
    correctOutput = correctOutput + "Direction: Input\n";

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
    //Convert to time_t but assuming the tm is specified in UTC time.
    std::time_t execTime_t =  Mantid::Kernel::DateAndTimeHelpers::utc_mktime ( timeinfo );
    //Create a UTC datetime from it
    Mantid::Kernel::DateAndTime execTime;
    execTime.set_from_time_t( execTime_t );

    // Not really much to test
    Algorithm *alg = new testalg;
    alg->initialize();
    TS_ASSERT_THROWS( alg->setPropertyValue("arg1_param","20"),
      std::invalid_argument );

    AlgorithmHistory AH(alg, execTime, 14.0);
    //dump output to sting
    std::ostringstream output;
    output.exceptions( std::ios::failbit | std::ios::badbit );
    TS_ASSERT_THROWS_NOTHING(output << AH);
    TS_ASSERT_EQUALS(output.str(),correctOutput);

    delete timeinfo;
    delete alg;
  }

  void test_Created_Algorithm_Matches_History()
  {
    Mantid::API::AlgorithmFactory::Instance().subscribe<testalg>();
    Algorithm *testInput = new testalg;
    testInput->initialize();
    testInput->setPropertyValue("arg2_param", "5");
    AlgorithmHistory history(testInput, 10000.,1.5);
    
    IAlgorithm_sptr compareAlg = history.createAlgorithm();
    TS_ASSERT_EQUALS(compareAlg->name(), testInput->name());
    TS_ASSERT_EQUALS(compareAlg->version(), testInput->version());
    TS_ASSERT_EQUALS(compareAlg->category(), testInput->category());
    
    TS_ASSERT_EQUALS(compareAlg->getPropertyValue("arg1_param"), "x");
    TS_ASSERT_EQUALS(compareAlg->getPropertyValue("arg2_param"), "5");
    
    Mantid::API::AlgorithmFactory::Instance().unsubscribe("testalg|1");

  }

};

#endif /* ALGORITHMHISTORYTEST_H_*/
