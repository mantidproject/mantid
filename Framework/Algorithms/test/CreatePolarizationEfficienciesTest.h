#ifndef MANTID_ALGORITHMS_CREATEPOLARIZATIONEFFICIENCIES_TEST_H_
#define MANTID_ALGORITHMS_CREATEPOLARIZATIONEFFICIENCIES_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/CreatePolarizationEfficiencies.h"
#include "MantidAPI/Axis.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <boost/make_shared.hpp>

using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;
using namespace WorkspaceCreationHelper;

class CreatePolarizationEfficienciesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CreatePolarizationEfficienciesTest *createSuite() {
    return new CreatePolarizationEfficienciesTest();
  }
  static void destroySuite(CreatePolarizationEfficienciesTest *suite) { delete suite; }

  void test_init() {
    CreatePolarizationEfficiencies alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_no_input() {
    auto inWS = createPointWS();
    CreatePolarizationEfficiencies alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inWS);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    TS_ASSERT_THROWS(alg.execute(), std::runtime_error);
  }

  void test_histo() {
    auto inWS = createHistoWS();

    CreatePolarizationEfficiencies alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inWS);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.setPropertyValue("CPp", "1,0,0,0");
    alg.setPropertyValue("CAp", "0,1,0,0");
    alg.setPropertyValue("CRho", "0,0,1,0");
    alg.setPropertyValue("CAlpha", "0,0,0,1");
    alg.execute();
    MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");

    TS_ASSERT(outWS);
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 4);

    auto axis1 = outWS->getAxis(1);
    TS_ASSERT_EQUALS(axis1->label(0), "CPp");
    TS_ASSERT_EQUALS(axis1->label(1), "CAp");
    TS_ASSERT_EQUALS(axis1->label(2), "CRho");
    TS_ASSERT_EQUALS(axis1->label(3), "CAlpha");

    TS_ASSERT_DELTA(outWS->readY(0)[0], 1.0, 1e-15);
    TS_ASSERT_DELTA(outWS->readY(0)[1], 1.0, 1e-15);
    TS_ASSERT_DELTA(outWS->readY(0)[2], 1.0, 1e-15);
    TS_ASSERT_DELTA(outWS->readY(0)[3], 1.0, 1e-15);
    TS_ASSERT_DELTA(outWS->readY(0)[4], 1.0, 1e-15);
    TS_ASSERT_DELTA(outWS->readY(0)[5], 1.0, 1e-15);
    TS_ASSERT_DELTA(outWS->readY(0)[6], 1.0, 1e-15);
    TS_ASSERT_DELTA(outWS->readY(0)[7], 1.0, 1e-15);

    TS_ASSERT_DELTA(outWS->readY(1)[0], 0.25, 1e-15);
    TS_ASSERT_DELTA(outWS->readY(1)[1], 0.75, 1e-15);
    TS_ASSERT_DELTA(outWS->readY(1)[2], 1.25, 1e-15);
    TS_ASSERT_DELTA(outWS->readY(1)[3], 1.75, 1e-15);
    TS_ASSERT_DELTA(outWS->readY(1)[4], 2.25, 1e-15);
    TS_ASSERT_DELTA(outWS->readY(1)[5], 2.75, 1e-15);
    TS_ASSERT_DELTA(outWS->readY(1)[6], 3.25, 1e-15);
    TS_ASSERT_DELTA(outWS->readY(1)[7], 3.75, 1e-15);

    TS_ASSERT_DELTA(outWS->readY(2)[0], 0.0625 , 1e-15);
    TS_ASSERT_DELTA(outWS->readY(2)[1], 0.5625 , 1e-15);
    TS_ASSERT_DELTA(outWS->readY(2)[2], 1.5625 , 1e-15);
    TS_ASSERT_DELTA(outWS->readY(2)[3], 3.0625 , 1e-15);
    TS_ASSERT_DELTA(outWS->readY(2)[4], 5.0625 , 1e-15);
    TS_ASSERT_DELTA(outWS->readY(2)[5], 7.5625 , 1e-15);
    TS_ASSERT_DELTA(outWS->readY(2)[6], 10.5625, 1e-15);
    TS_ASSERT_DELTA(outWS->readY(2)[7], 14.0625, 1e-15);

    TS_ASSERT_DELTA(outWS->readY(3)[0], 0.015625 , 1e-15);
    TS_ASSERT_DELTA(outWS->readY(3)[1], 0.421875 , 1e-15);
    TS_ASSERT_DELTA(outWS->readY(3)[2], 1.953125 , 1e-15);
    TS_ASSERT_DELTA(outWS->readY(3)[3], 5.359375 , 1e-15);
    TS_ASSERT_DELTA(outWS->readY(3)[4], 11.390625, 1e-15);
    TS_ASSERT_DELTA(outWS->readY(3)[5], 20.796875, 1e-15);
    TS_ASSERT_DELTA(outWS->readY(3)[6], 34.328125, 1e-15);
    TS_ASSERT_DELTA(outWS->readY(3)[7], 52.734375, 1e-15);
  }

  void test_points() {
    auto inWS = createPointWS();

    CreatePolarizationEfficiencies alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inWS);
    alg.setPropertyValue("OutputWorkspace", "dummy");
    alg.setPropertyValue("CPp", "1,0,0,0");
    alg.setPropertyValue("CAp", "0,1,0,0");
    alg.setPropertyValue("CRho", "0,0,1,0");
    alg.setPropertyValue("CAlpha", "0,0,0,1");
    alg.execute();
    MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");

    TS_ASSERT(outWS);
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 4);

    auto axis1 = outWS->getAxis(1);
    TS_ASSERT_EQUALS(axis1->label(0), "CPp");
    TS_ASSERT_EQUALS(axis1->label(1), "CAp");
    TS_ASSERT_EQUALS(axis1->label(2), "CRho");
    TS_ASSERT_EQUALS(axis1->label(3), "CAlpha");

    TS_ASSERT_DELTA(outWS->readY(0)[0], 1.0, 1e-15);
    TS_ASSERT_DELTA(outWS->readY(0)[1], 1.0, 1e-15);
    TS_ASSERT_DELTA(outWS->readY(0)[2], 1.0, 1e-15);
    TS_ASSERT_DELTA(outWS->readY(0)[3], 1.0, 1e-15);
    TS_ASSERT_DELTA(outWS->readY(0)[4], 1.0, 1e-15);
    TS_ASSERT_DELTA(outWS->readY(0)[5], 1.0, 1e-15);
    TS_ASSERT_DELTA(outWS->readY(0)[6], 1.0, 1e-15);
    TS_ASSERT_DELTA(outWS->readY(0)[7], 1.0, 1e-15);
    TS_ASSERT_DELTA(outWS->readY(0)[8], 1.0, 1e-15);

    TS_ASSERT_DELTA(outWS->readY(1)[0], 0.0, 1e-15);
    TS_ASSERT_DELTA(outWS->readY(1)[1], 0.5, 1e-15);
    TS_ASSERT_DELTA(outWS->readY(1)[2], 1.0, 1e-15);
    TS_ASSERT_DELTA(outWS->readY(1)[3], 1.5, 1e-15);
    TS_ASSERT_DELTA(outWS->readY(1)[4], 2.0, 1e-15);
    TS_ASSERT_DELTA(outWS->readY(1)[5], 2.5, 1e-15);
    TS_ASSERT_DELTA(outWS->readY(1)[6], 3.0, 1e-15);
    TS_ASSERT_DELTA(outWS->readY(1)[7], 3.5, 1e-15);
    TS_ASSERT_DELTA(outWS->readY(1)[8], 4.0, 1e-15);

    TS_ASSERT_DELTA(outWS->readY(2)[0], 0.0  , 1e-15);
    TS_ASSERT_DELTA(outWS->readY(2)[1], 0.25 , 1e-15);
    TS_ASSERT_DELTA(outWS->readY(2)[2], 1.0  , 1e-15);
    TS_ASSERT_DELTA(outWS->readY(2)[3], 2.25 , 1e-15);
    TS_ASSERT_DELTA(outWS->readY(2)[4], 4.0  , 1e-15);
    TS_ASSERT_DELTA(outWS->readY(2)[5], 6.25 , 1e-15);
    TS_ASSERT_DELTA(outWS->readY(2)[6], 9.0  , 1e-15);
    TS_ASSERT_DELTA(outWS->readY(2)[7], 12.25, 1e-15);
    TS_ASSERT_DELTA(outWS->readY(2)[8], 16.0 , 1e-15);

    TS_ASSERT_DELTA(outWS->readY(3)[0], 0.0   , 1e-15);
    TS_ASSERT_DELTA(outWS->readY(3)[1], 0.125 , 1e-15);
    TS_ASSERT_DELTA(outWS->readY(3)[2], 1.0   , 1e-15);
    TS_ASSERT_DELTA(outWS->readY(3)[3], 3.375 , 1e-15);
    TS_ASSERT_DELTA(outWS->readY(3)[4], 8.0   , 1e-15);
    TS_ASSERT_DELTA(outWS->readY(3)[5], 15.625, 1e-15);
    TS_ASSERT_DELTA(outWS->readY(3)[6], 27.0  , 1e-15);
    TS_ASSERT_DELTA(outWS->readY(3)[7], 42.875, 1e-15);
    TS_ASSERT_DELTA(outWS->readY(3)[8], 64.0  , 1e-15);
  }

private:

  Workspace2D_sptr createHistoWS() {
    size_t const size = 8;
    BinEdges xVals(size + 1, LinearGenerator(0, 0.5));
    Counts yVals(size, 0);
    auto retVal = boost::make_shared<Workspace2D>();
    retVal->initialize(1, Histogram(xVals, yVals));
    return retVal;
  }

  Workspace2D_sptr createPointWS() {
    size_t const size = 9;
    Points xVals(size, LinearGenerator(0, 0.5));
    Counts yVals(size, 0);
    auto retVal = boost::make_shared<Workspace2D>();
    retVal->initialize(1, Histogram(xVals, yVals));
    return retVal;
  }
};

#endif /* MANTID_ALGORITHMS_CREATEPOLARIZATIONEFFICIENCIES_TEST_H_ */
