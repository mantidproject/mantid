#ifndef ENVIRONMENTHISTORYTEST_H_
#define ENVIRONMENTHISTORYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/EnvironmentHistory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/MantidVersion.h"
#include <sstream>

using namespace Mantid::Kernel;

class EnvironmentHistoryTest : public CxxTest::TestSuite
{
public:

  void testframeworkVersion()
  {
    EnvironmentHistory EH;
    TS_ASSERT_EQUALS(EH.frameworkVersion(),MantidVersion::version());
  }

  void testosName()
 {
    EnvironmentHistory EH;
    TS_ASSERT_EQUALS(EH.osName(),ConfigService::Instance().getOSName());
 }

  void testosVersion()
  {
    EnvironmentHistory EH;
    TS_ASSERT_EQUALS(EH.osVersion(),ConfigService::Instance().getOSVersion());
  }
  
  void testPopulate()
  {
    std::string correctOutput = "Framework Version: " + std::string(MantidVersion::version()) + "\n";
    correctOutput += "OS name: " + ConfigService::Instance().getOSName() + "\n";
    correctOutput += "OS version: " + ConfigService::Instance().getOSVersion() + "\n";

    // Not really much to test
    EnvironmentHistory EH;

    //dump output to sting
    std::ostringstream output;
    output.exceptions( std::ios::failbit | std::ios::badbit );
    TS_ASSERT_THROWS_NOTHING(output << EH);
    TS_ASSERT_EQUALS(output.str(),correctOutput);
  }
};

#endif /* ALGORITHMPARAMETERTEST_H_*/
