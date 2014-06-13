#ifndef MANTID_API_MULTIPLEEXPERIMENTINFOSTEST_H_
#define MANTID_API_MULTIPLEEXPERIMENTINFOSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidAPI/MultipleExperimentInfos.h"
#include "MantidAPI/ExperimentInfo.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::API;

class MultipleExperimentInfosTest : public CxxTest::TestSuite
{
public:

  void test_setExperimentInfos()
  {
    MultipleExperimentInfos mei;
    TS_ASSERT_EQUALS( mei.getNumExperimentInfo(), 0);
    ExperimentInfo_sptr ei(new ExperimentInfo);
    TS_ASSERT_EQUALS( mei.addExperimentInfo(ei), 0);
    TS_ASSERT_EQUALS( mei.getNumExperimentInfo(), 1);
    TS_ASSERT_EQUALS( mei.getExperimentInfo(0), ei);
    TS_ASSERT_THROWS_ANYTHING( mei.getExperimentInfo(1) );
    ExperimentInfo_sptr ei2(new ExperimentInfo);
    mei.setExperimentInfo(0, ei2);
    TS_ASSERT_EQUALS( mei.getExperimentInfo(0), ei2);
  }

  void test_copy_constructor()
  {
    MultipleExperimentInfos mei;
    ExperimentInfo_sptr ei(new ExperimentInfo);
    TS_ASSERT_EQUALS( mei.addExperimentInfo(ei), 0);
    MultipleExperimentInfos copy(mei);
    TS_ASSERT_EQUALS( copy.getNumExperimentInfo(), 1);
    TSM_ASSERT_DIFFERS( "ExperimentInfo's were deep-copied", copy.getExperimentInfo(0), mei.getExperimentInfo(0));
  }
};


#endif /* MANTID_API_MULTIPLEEXPERIMENTINFOSTEST_H_ */
