#ifndef LOADSAMPLEDETAILSFROMRAWTEST_H_
#define LOADSAMPLEDETAILSFROMRAWTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadSampleDetailsFromRaw.h"
#include "MantidAPI/Sample.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::API;

class LoadSampleDetailsFromRawTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadSampleDetailsFromRawTest *createSuite() {
    return new LoadSampleDetailsFromRawTest();
  }
  static void destroySuite(LoadSampleDetailsFromRawTest *suite) {
    delete suite;
  }

  void testExec() {
    // setup and run the algorithm (includes basic checks)
    LoadSampleDetailsFromRaw alg;
    const MatrixWorkspace_const_sptr inWS = setupAlgorithm(alg, "HRP39180.RAW");
    runAlgorithm(alg);

    // specific checks
    TS_ASSERT_EQUALS(inWS->sample().getGeometryFlag(), 2);
    TS_ASSERT_DELTA(inWS->sample().getHeight(), 20.0, 1e-6);
    TS_ASSERT_DELTA(inWS->sample().getWidth(), 15.0, 1e-6);
    TS_ASSERT_DELTA(inWS->sample().getThickness(), 11.0, 1e-6);
  }

private:
  const MatrixWorkspace_sptr makeFakeWorkspace() {
    // create the workspace
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::createEventWorkspace();
    return ws;
  }

  // Initialise the algorithm and set the properties. Creates a fake
  // workspace for the input and returns it.
  const MatrixWorkspace_sptr setupAlgorithm(LoadSampleDetailsFromRaw &alg,
                                            const std::string &filename) {
    // create the workspace
    const MatrixWorkspace_sptr inWS = makeFakeWorkspace();

    // set up the algorithm
    if (!alg.isInitialized())
      alg.initialize();
    alg.setProperty("InputWorkspace", inWS);
    alg.setProperty("Filename", filename);

    return inWS;
  }

  // Run the algorithm and do some basic checks
  void runAlgorithm(LoadSampleDetailsFromRaw &alg) {
    // run the algorithm
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
  }
};

#endif /*LOADSAMPLEDETAILSFROMRAWTEST_H_*/
