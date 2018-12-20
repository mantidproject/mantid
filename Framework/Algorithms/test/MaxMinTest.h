#ifndef MAXMINTEST_H_
#define MAXMINTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/CreateWorkspace.h"
#include "MantidAlgorithms/Max.h"
#include "MantidAlgorithms/MaxMin.h"
#include "MantidAlgorithms/Min.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;

class MaxMinTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MaxMinTest *createSuite() { return new MaxMinTest(); }
  static void destroySuite(MaxMinTest *suite) { delete suite; }

  MaxMinTest() : inputWSname("testInput"), outputWSname("testOutput") {}

  void testName() {
    TS_ASSERT_EQUALS(maxa.name(), "Max");
    TS_ASSERT_EQUALS(mina.name(), "Min");
    TS_ASSERT_EQUALS(maxmina.name(), "MaxMin");
  }

  void testVersion() {
    TS_ASSERT_EQUALS(maxa.version(), 1);
    TS_ASSERT_EQUALS(mina.version(), 1);
    TS_ASSERT_EQUALS(maxmina.version(), 1);
  }

  void testMaxMinInit() {
    TS_ASSERT_THROWS_NOTHING(maxmina.initialize())
    TS_ASSERT(maxmina.isInitialized())
  }

  void testMaxInit() {
    TS_ASSERT_THROWS_NOTHING(maxa.initialize())
    TS_ASSERT(maxa.isInitialized())
  }

  void testMinInit() {
    TS_ASSERT_THROWS_NOTHING(mina.initialize())
    TS_ASSERT(mina.isInitialized())
  }

  void testMaxMin1() {
    setupWorkspace();

    if (!maxmina.isInitialized())
      maxmina.initialize();

    // AnalysisDataService::Instance().add("tomultiply",WorkspaceCreationHelper::Create2DWorkspace123(10,10));
    TS_ASSERT_THROWS_NOTHING(
        maxmina.setPropertyValue("InputWorkspace", inputWSname))
    TS_ASSERT_THROWS_NOTHING(
        maxmina.setPropertyValue("OutputWorkspace", outputWSname))
    TS_ASSERT_THROWS_NOTHING(maxmina.setPropertyValue("Showmin", "1"))

    TS_ASSERT_THROWS_NOTHING(maxmina.execute())
    TS_ASSERT(maxmina.isExecuted())

    MatrixWorkspace_const_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(outputWSname)))

    TS_ASSERT_EQUALS(result->x(0)[0], 4)
    TS_ASSERT_EQUALS(result->y(0)[0], 0)
    TS_ASSERT_EQUALS(result->e(0)[0], 0)
    TS_ASSERT_EQUALS(result->x(1)[0], 1)
    TS_ASSERT_EQUALS(result->y(1)[0], 1)
    TS_ASSERT_EQUALS(result->e(1)[0], 0)

    AnalysisDataService::Instance().remove(outputWSname);
    AnalysisDataService::Instance().remove(inputWSname);
  }

  void testMaxMin2() {
    setupWorkspace();

    if (!maxmina.isInitialized())
      maxmina.initialize();

    // AnalysisDataService::Instance().add("tomultiply",WorkspaceCreationHelper::Create2DWorkspace123(10,10));
    TS_ASSERT_THROWS_NOTHING(
        maxmina.setPropertyValue("InputWorkspace", inputWSname))
    TS_ASSERT_THROWS_NOTHING(
        maxmina.setPropertyValue("OutputWorkspace", outputWSname))
    TS_ASSERT_THROWS_NOTHING(
        maxmina.setPropertyValue("Showmin", "0")) // should find max

    TS_ASSERT_THROWS_NOTHING(maxmina.execute())
    TS_ASSERT(maxmina.isExecuted())

    MatrixWorkspace_const_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(outputWSname)))

    TS_ASSERT_EQUALS(result->x(0)[0], 3)
    TS_ASSERT_EQUALS(result->y(0)[0], 3)
    TS_ASSERT_EQUALS(result->e(0)[0], 0)
    TS_ASSERT_EQUALS(result->x(1)[0], 5)
    TS_ASSERT_EQUALS(result->y(1)[0], 5)
    TS_ASSERT_EQUALS(result->e(1)[0], 0)

    AnalysisDataService::Instance().remove(outputWSname);
    AnalysisDataService::Instance().remove(inputWSname);
  }

  void testMax() {
    setupWorkspace();

    if (!maxa.isInitialized())
      maxa.initialize();

    // AnalysisDataService::Instance().add("tomultiply",WorkspaceCreationHelper::Create2DWorkspace123(10,10));
    TS_ASSERT_THROWS_NOTHING(
        maxa.setPropertyValue("InputWorkspace", inputWSname))
    TS_ASSERT_THROWS_NOTHING(
        maxa.setPropertyValue("OutputWorkspace", outputWSname))

    TS_ASSERT_THROWS_NOTHING(maxa.execute())
    TS_ASSERT(maxa.isExecuted())

    MatrixWorkspace_const_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(outputWSname)))

    TS_ASSERT_EQUALS(result->x(0)[0], 3)
    TS_ASSERT_EQUALS(result->y(0)[0], 3)
    TS_ASSERT_EQUALS(result->e(0)[0], 0)
    TS_ASSERT_EQUALS(result->x(1)[0], 5)
    TS_ASSERT_EQUALS(result->y(1)[0], 5)
    TS_ASSERT_EQUALS(result->e(1)[0], 0)

    AnalysisDataService::Instance().remove(outputWSname);
    AnalysisDataService::Instance().remove(inputWSname);
  }

  void testMin() {
    setupWorkspace();

    if (!mina.isInitialized())
      mina.initialize();

    // AnalysisDataService::Instance().add("tomultiply",WorkspaceCreationHelper::Create2DWorkspace123(10,10));
    TS_ASSERT_THROWS_NOTHING(
        mina.setPropertyValue("InputWorkspace", inputWSname))
    TS_ASSERT_THROWS_NOTHING(
        mina.setPropertyValue("OutputWorkspace", outputWSname))

    TS_ASSERT_THROWS_NOTHING(mina.execute())
    TS_ASSERT(mina.isExecuted())

    MatrixWorkspace_const_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(outputWSname)))

    TS_ASSERT_EQUALS(result->x(0)[0], 4)
    TS_ASSERT_EQUALS(result->y(0)[0], 0)
    TS_ASSERT_EQUALS(result->e(0)[0], 0)
    TS_ASSERT_EQUALS(result->x(1)[0], 1)
    TS_ASSERT_EQUALS(result->y(1)[0], 1)
    TS_ASSERT_EQUALS(result->e(1)[0], 0)

    AnalysisDataService::Instance().remove(outputWSname);
    AnalysisDataService::Instance().remove(inputWSname);
  }

private:
  Mantid::Algorithms::MaxMin maxmina;
  Mantid::Algorithms::Max maxa;
  Mantid::Algorithms::Min mina;
  std::string inputWSname;
  std::string outputWSname;

  void setupWorkspace() {
    Mantid::Algorithms::CreateWorkspace creator;
    creator.initialize();
    creator.setPropertyValue("OutputWorkspace", inputWSname);
    creator.setProperty("DataX", "1,2,3,4,5,6,1,2,3,4,5,6");
    creator.setProperty("DataY", "1,2,3,0,1,1,2,3,4,5");
    creator.setProperty("DataE", "0,0,0,0,0,0,0,0,0,0");
    creator.setProperty("NSpec", 2);
    creator.execute();
  }
};

#endif /*MAXMINTEST_H_*/
