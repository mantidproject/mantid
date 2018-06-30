#ifndef MANTID_API_MULTIPLEEXPERIMENTINFOSTEST_H_
#define MANTID_API_MULTIPLEEXPERIMENTINFOSTEST_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/MultipleExperimentInfos.h"
#include "MantidAPI/Sample.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Geometry;

class MultipleExperimentInfosTest : public CxxTest::TestSuite {
public:
  void test_setExperimentInfos() {
    MultipleExperimentInfos mei;
    TS_ASSERT_EQUALS(mei.getNumExperimentInfo(), 0);
    ExperimentInfo_sptr ei(new ExperimentInfo);
    TS_ASSERT_EQUALS(mei.addExperimentInfo(ei), 0);
    TS_ASSERT_EQUALS(mei.getNumExperimentInfo(), 1);
    TS_ASSERT_EQUALS(mei.getExperimentInfo(0), ei);
    TS_ASSERT_THROWS_ANYTHING(mei.getExperimentInfo(1));
    ExperimentInfo_sptr ei2(new ExperimentInfo);
    mei.setExperimentInfo(0, ei2);
    TS_ASSERT_EQUALS(mei.getExperimentInfo(0), ei2);
  }

  void test_copy_constructor() {
    MultipleExperimentInfos mei;
    ExperimentInfo_sptr ei(new ExperimentInfo);
    TS_ASSERT_EQUALS(mei.addExperimentInfo(ei), 0);
    MultipleExperimentInfos copy(mei);
    TS_ASSERT_EQUALS(copy.getNumExperimentInfo(), 1);
    TSM_ASSERT_DIFFERS("ExperimentInfo's were deep-copied",
                       copy.getExperimentInfo(0), mei.getExperimentInfo(0));
  }

  void testHasOrientedLattice() {
    constexpr uint16_t nExperimentInfosToAdd = 3;

    MultipleExperimentInfos mei;
    TS_ASSERT_EQUALS(mei.hasOrientedLattice(), false);

    OrientedLattice *latt = new OrientedLattice(1.0, 2.0, 3.0, 90, 90, 90);

    // add some oriented lattices to the multiple experiment info
    for (uint16_t i = 0; i < nExperimentInfosToAdd; ++i) {
      ExperimentInfo_sptr experimentInfo = boost::make_shared<ExperimentInfo>();
      mei.addExperimentInfo(experimentInfo);
      mei.getExperimentInfo(i)->mutableSample().setOrientedLattice(latt);
      TS_ASSERT_EQUALS(mei.hasOrientedLattice(), true);
    }

    // take them away one by one starting with the first, leave the last one
    for (uint16_t i = 0; i < nExperimentInfosToAdd - 1; ++i) {
      mei.getExperimentInfo(i)->mutableSample().clearOrientedLattice();
      TS_ASSERT_EQUALS(mei.hasOrientedLattice(), true);
    }

    // remove the last one
    mei.getExperimentInfo((nExperimentInfosToAdd - 1))
        ->mutableSample()
        .clearOrientedLattice();
    TS_ASSERT_EQUALS(mei.hasOrientedLattice(), false);

    delete latt;
  }
};

#endif /* MANTID_API_MULTIPLEEXPERIMENTINFOSTEST_H_ */
