#ifndef MANTID_ALGORITHMS_DETECTOREFFICIENCYCORUSERTEST_H_
#define MANTID_ALGORITHMS_DETECTOREFFICIENCYCORUSERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/DetectorEfficiencyCorUser.h"
#include "MantidAPI/Axis.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

using Mantid::Algorithms::DetectorEfficiencyCorUser;

using namespace Mantid;
using namespace Mantid::API;
using namespace Kernel;

class DetectorEfficiencyCorUserTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DetectorEfficiencyCorUserTest *createSuite() {
    return new DetectorEfficiencyCorUserTest();
  }
  static void destroySuite(DetectorEfficiencyCorUserTest *suite) {
    delete suite;
  }

  // contructor
  DetectorEfficiencyCorUserTest()
      : m_inWSName("input_workspace"), m_outWSName("output_workspace") {
    m_Ei = 3.27;

    createInputWorkSpace();
  }

  void test_Init() {
    DetectorEfficiencyCorUser alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {

    DetectorEfficiencyCorUser alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", m_inWSName));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", m_outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the output workspace from data service.
    MatrixWorkspace_sptr outWS;
    TS_ASSERT_THROWS_NOTHING(
        outWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            m_outWSName));
    TS_ASSERT(outWS);
    if (!outWS)
      return;

    // Retrieve the output workspace from data service.
    MatrixWorkspace_sptr inWS;
    TS_ASSERT_THROWS_NOTHING(
        inWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            m_inWSName));
    TS_ASSERT(inWS);
    if (!inWS)
      return;

    TS_ASSERT_DELTA(outWS->readY(0).front(), inWS->readY(0).front(), 1);
    TS_ASSERT_DELTA(outWS->readY(0).back(), inWS->readY(0).back(), 0.5);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(m_outWSName);
  }

private:
  double m_Ei;
  const std::string m_inWSName, m_outWSName;

  void createInputWorkSpace() {
    int numBanks = 1;
    int numPixels = 10;
    int numBins = 20;

    DataObjects::Workspace2D_sptr dataws =
        WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(
            numBanks, numPixels, numBins);

    // WorkspaceCreationHelper::addNoise(dataws,10);
    // np.linspace(2,5,21)
    HistogramData::BinEdges binEdges = {
        -10.,  -9.25, -8.5,  -7.75, -7.,  -6.25, -5.5, -4.75, -4.,  -3.25, -2.5,
        -1.75, -1.,   -0.25, 0.5,   1.25, 2.,    2.75, 3.5,   4.25, 5.};

    for (size_t wi = 0; wi < dataws->getNumberHistograms(); wi++) {
      dataws->setBinEdges(wi, binEdges);
    }
    // WorkspaceCreationHelper::DisplayDataX(dataws);

    dataws->getAxis(0)->setUnit("DeltaE");
    dataws->mutableRun().addProperty("Ei",
                                     boost::lexical_cast<std::string>(m_Ei));
    dataws->instrumentParameters().addString(
        dataws->getInstrument()->getChild(0).get(), "formula_eff",
        "exp(-0.0565/sqrt(e))*(1.0-exp(-3.284/sqrt(e)))"); // IN5

    API::AnalysisDataService::Instance().addOrReplace(m_inWSName, dataws);
  }
};

#endif /* MANTID_ALGORITHMS_DETECTOREFFICIENCYCORUSERTEST_H_ */
