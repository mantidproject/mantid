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
    declareProperty("arg1_param","x",Direction::Input);
    declareProperty("arg2_param",23);
  }
  void exec() {}
};

class AlgorithmHistoryTest : public CxxTest::TestSuite
{
public:
  AlgorithmHistoryTest() : m_correctOutput(), m_execCount(0)
  {
  }

  void testPopulate()
  {
    AlgorithmHistory AH = createTestHistory();
    //dump output to sting
    std::ostringstream output;
    output.exceptions( std::ios::failbit | std::ios::badbit );
    TS_ASSERT_THROWS_NOTHING(output << AH);
    TS_ASSERT_EQUALS(output.str(), m_correctOutput);
    // Does it equal itself
    TS_ASSERT_EQUALS(AH, AH);
  }

  void test_Less_Than_Returns_True_For_If_Execution_Order_Is_Lower()
  {
    AlgorithmHistory first = createTestHistory();
    AlgorithmHistory second = createTestHistory();
    TS_ASSERT_LESS_THAN(first, second);
  }


  void test_Created_Algorithm_Matches_History()
  {
    Mantid::API::AlgorithmFactory::Instance().subscribe<testalg>();
    Algorithm *testInput = new testalg;
    testInput->initialize();
    testInput->setPropertyValue("arg2_param", "5");
    AlgorithmHistory history(testInput);
    
    IAlgorithm_sptr compareAlg = history.createAlgorithm();
    TS_ASSERT_EQUALS(compareAlg->name(), testInput->name());
    TS_ASSERT_EQUALS(compareAlg->version(), testInput->version());
    TS_ASSERT_EQUALS(compareAlg->category(), testInput->category());
    
    TS_ASSERT_EQUALS(compareAlg->getPropertyValue("arg1_param"), "x");
    TS_ASSERT_EQUALS(compareAlg->getPropertyValue("arg2_param"), "5");
    
    Mantid::API::AlgorithmFactory::Instance().unsubscribe("testalg|1");

  }

private:
  AlgorithmHistory createTestHistory()
  {
    m_correctOutput = "Algorithm: testalg ";
    m_correctOutput +=  "v1\n";
    m_correctOutput +=  "Execution Date: 2008-Feb-29 09:54:49\n";
    m_correctOutput +=  "Execution Duration: 14 seconds\n";
    m_correctOutput +=  "Parameters:\n";
    m_correctOutput +=  "  Name: arg1_param, ";
    m_correctOutput +=  "Value: y, ";
    m_correctOutput +=  "Default?: No, ";
    m_correctOutput +=  "Direction: Input\n";
    m_correctOutput +=  "  Name: arg2_param, ";
    m_correctOutput +=  "Value: 23, ";
    m_correctOutput +=  "Default?: Yes, ";
    m_correctOutput +=  "Direction: Input\n";

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
    testalg alg;
    alg.initialize();
    alg.setPropertyValue("arg1_param", "y");
    alg.execute();

    delete timeinfo;

    return AlgorithmHistory(&alg, execTime, 14.0,  m_execCount++);
  }

  std::string m_correctOutput;
  size_t m_execCount;
};

#endif /* ALGORITHMHISTORYTEST_H_*/
