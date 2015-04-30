#ifndef MANTID_ALGORITHMS_ESTIMATERESOLUTIONDIFFRACTIONTEST_H_
#define MANTID_ALGORITHMS_ESTIMATERESOLUTIONDIFFRACTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/EstimateResolutionDiffraction.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/TimeSeriesProperty.h"

using Mantid::Algorithms::EstimateResolutionDiffraction;
using Mantid::DataHandling::LoadEmptyInstrument;

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;

class EstimateResolutionDiffractionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EstimateResolutionDiffractionTest *createSuite() {
    return new EstimateResolutionDiffractionTest();
  }
  static void destroySuite(EstimateResolutionDiffractionTest *suite) {
    delete suite;
  }

  /** Test init
    */
  void test_Init() {
    EstimateResolutionDiffraction alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  /** Test POWGEN
    */
  void test_EmptyPG3() {
    // Create an empty PG3 workspace
    MatrixWorkspace_sptr ws = createInstrument();

    // Set up and run
    EstimateResolutionDiffraction alg;
    alg.initialize();

    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", ws->name()));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "PG3_Resolution"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DeltaTOF", 40.0));

    alg.execute();
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr outputws =
        boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("PG3_Resolution"));
    TS_ASSERT(outputws);
    if (!outputws)
      return;

    size_t numspec = outputws->getNumberHistograms();
    TS_ASSERT_EQUALS(numspec, 25873);

    for (size_t i = 0; i < numspec; ++i)
      TS_ASSERT(outputws->readY(i)[0] < 0.03);
  }

  /** Create an instrument
    */
  API::MatrixWorkspace_sptr createInstrument() {
    // Create empty workspace
    LoadEmptyInstrument loader;
    loader.initialize();

    loader.setProperty("Filename", "POWGEN_Definition_2013-06-01.xml");
    loader.setProperty("OutputWorkspace", "PG3_Sctrach");

    loader.execute();
    TS_ASSERT(loader.isExecuted());

    // Time series property
    TimeSeriesProperty<double> *lambda =
        new TimeSeriesProperty<double>("LambdaRequest");
    lambda->setUnits("Angstrom");
    DateAndTime time0(0);
    lambda->addValue(time0, 1.066);

    // Add log to workspace
    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("PG3_Sctrach"));
    ws->mutableRun().addProperty(lambda);

    return ws;
  }
};

#endif /* MANTID_ALGORITHMS_ESTIMATERESOLUTIONDIFFRACTIONTEST_H_ */
