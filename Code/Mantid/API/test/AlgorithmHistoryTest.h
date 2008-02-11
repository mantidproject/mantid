#ifndef ALGORITHMHISTORYTEST_H_
#define ALGORITHMHISTORYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/AlgorithmHistory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include <ostream>

using namespace Mantid::API;



class AlgorithmHistoryTest : public CxxTest::TestSuite
{
public:

  void testPopulate()
  {
    // Not really much to test
    std::vector<AlgorithmParameter> aps;
    aps.push_back(AlgorithmParameter("arg1_param","20","argument",true,Mantid::Kernel::Direction::Input));
     aps.push_back(AlgorithmParameter("arg2_param","23","argument",true,Mantid::Kernel::Direction::InOut));
   
    AlgorithmHistory AH("testalg","version 1",time(NULL),14.0,aps);
    TS_ASSERT_THROWS_NOTHING(std::cout<<std::endl << AH <<std::endl;)

  }


};

#endif /* ALGORITHMHISTORYTEST_H_*/
