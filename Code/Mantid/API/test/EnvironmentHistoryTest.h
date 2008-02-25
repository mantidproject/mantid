#ifndef ENVIRONMENTHISTORYTEST_H_
#define ENVIRONMENTHISTORYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/EnvironmentHistory.h"
#include <ostream>

using namespace Mantid::API;



class EnvironmentHistoryTest : public CxxTest::TestSuite
{
public:

  void testPopulate()
  {
    // Not really much to test 
    EnvironmentHistory EH("1","MS Windows","XP Pro","isiscnc\\jdmc43");
    TS_ASSERT_THROWS_NOTHING(std::cout<<std::endl << EH <<std::endl;)
  }
};

#endif /* ALGORITHMPARAMETERTEST_H_*/
