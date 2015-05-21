#ifndef GENERALISEDSECONDDIFFERENCETEST_H_
#define GENERALISEDSECONDDIFFERENCETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include <boost/assign.hpp>

using namespace Mantid::API;

class GeneralisedSecondDifferenceTest : public CxxTest::TestSuite {

public:
  void testInit() {

    IAlgorithm_sptr gsd = Mantid::API::AlgorithmManager::Instance().create(
        "GeneralisedSecondDifference", 1);

    TS_ASSERT_EQUALS(gsd->name(), "GeneralisedSecondDifference");
    TS_ASSERT_EQUALS(gsd->category(), "Arithmetic");
    TS_ASSERT_THROWS_NOTHING(gsd->initialize());
    TS_ASSERT(gsd->isInitialized());
  }

  void testExec() {

    IAlgorithm_sptr gsd = Mantid::API::AlgorithmManager::Instance().create(
      "GeneralisedSecondDifference", 1);

    std::vector<double> x =
        boost::assign::list_of(0)(1)(2)(3)(4)(5)(6)(7)(8)(9);
    std::vector<double> y = boost::assign::list_of(0.3)(0.3)(0.3)(0.47)(3.9)
        (10.3)(3.9)(0.47)(0.3)(0.3);
    MatrixWorkspace_sptr inputWs = WorkspaceFactory::Instance().create(
        "Workspace2D", 1, y.size(), y.size());
    inputWs->dataY(0) = y;
    inputWs->dataX(0) = x;

    gsd->setProperty("InputWorkspace", inputWs);
    gsd->setProperty("M", "1");
    gsd->setProperty("Z", "2");
    gsd->setPropertyValue("OutputWorkspace", "secondDiff");

    gsd->execute();
    TS_ASSERT(gsd->isExecuted());

    MatrixWorkspace_sptr outWs = Mantid::API::AnalysisDataService::Instance()
                                     .retrieveWS<MatrixWorkspace>("secondDiff");
    TS_ASSERT(outWs);

    TS_ASSERT_EQUALS(outWs->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outWs->blocksize(), 4);

    x = outWs->readX(0);
    TS_ASSERT_EQUALS(x[0], 3);
    TS_ASSERT_EQUALS(x[3], 6);

    y = outWs->readY(0);
    TS_ASSERT_DELTA(y[1], -7.0300, 0.0001);
    TS_ASSERT_DELTA(y[2], -20.0000, 0.0001);

    Mantid::API::AnalysisDataService::Instance().remove("secondDiff");
  }

};

#endif /* GENERALISEDSECONDDIFERENCETEST_H_ */
