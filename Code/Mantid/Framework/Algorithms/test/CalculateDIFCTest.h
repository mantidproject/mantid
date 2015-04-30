#ifndef MANTID_ALGORITHMS_CALCULATEDIFCTEST_H_
#define MANTID_ALGORITHMS_CALCULATEDIFCTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CalculateDIFC.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::CalculateDIFC;
using Mantid::DataObjects::OffsetsWorkspace;
using Mantid::DataObjects::OffsetsWorkspace_sptr;
using Mantid::DataObjects::Workspace2D_sptr;
using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace {
  const double OFFSET = .1;
  const int NUM_SPEC = 3;
}

class CalculateDIFCTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CalculateDIFCTest *createSuite() { return new CalculateDIFCTest(); }
  static void destroySuite(CalculateDIFCTest *suite) { delete suite; }

  void test_Init() {
    CalculateDIFC alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void runTest(Workspace2D_sptr inputWS, OffsetsWorkspace_sptr offsetsWS,
               std::string &outWSName) {

    CalculateDIFC alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OffsetsWorkspace", offsetsWS));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service. TODO: Change to your desired
    // type
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(outWSName)));
    TS_ASSERT(ws);
    if (!ws)
      return;

    // should only be NUM_SPEC
    double factor = 1.;
    if (offsetsWS)
      factor = 1. / (1. + OFFSET);
    TS_ASSERT_DELTA(ws->readY(0)[0], factor * 0., 1.);
    TS_ASSERT_DELTA(ws->readY(1)[0], factor * 126., 1.);
    TS_ASSERT_DELTA(ws->readY(2)[0], factor * 252., 1.);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_withoutOffsets() {
    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
        NUM_SPEC, 1);
    std::string outWSName("CalculateDIFCTest_withoutOffsets_OutputWS");

    runTest(inputWS, OffsetsWorkspace_sptr(), outWSName);
  }

  void test_withOffsets() {
    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
        NUM_SPEC, 1);
    std::string outWSName("CalculateDIFCTest_withOffsets_OutputWS");

    auto offsetsWS =
        OffsetsWorkspace_sptr(new OffsetsWorkspace(inputWS->getInstrument()));
    for (int i = 0; i < NUM_SPEC; ++i) {
      IDetector_const_sptr det = inputWS->getDetector(i);
      offsetsWS->setValue(det->getID(), OFFSET);
    }

    runTest(inputWS, offsetsWS, outWSName);
  }
};

#endif /* MANTID_ALGORITHMS_CALCULATEDIFCTEST_H_ */
