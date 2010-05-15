#ifndef PROPERTYHISTORYTEST_H_
#define PROPERTYHISTORYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/PropertyHistory.h"
#include "MantidKernel/Property.h"
#include <sstream>

using namespace Mantid::Kernel;

class PropertyHistoryTest : public CxxTest::TestSuite
{
public:

  void testPopulate()
  {
    std::string correctOutput = "Name: arg1_param, ";
    correctOutput = correctOutput + "Value: 20, ";
    correctOutput = correctOutput + "Default?: Yes, ";
    correctOutput = correctOutput + "Direction: Input\n";

    // Not really much to test
    PropertyHistory AP("arg1_param","20","argument",true,Direction::Input);

    //dump output to sting
    std::ostringstream output;
    TS_ASSERT_THROWS_NOTHING(output << AP);
    TS_ASSERT_EQUALS(output.str(),correctOutput);
  }


};

#endif /* PROPERTYHISTORYTEST_H_*/
