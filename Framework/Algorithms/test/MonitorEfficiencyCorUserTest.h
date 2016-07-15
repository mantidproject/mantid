#ifndef MANTID_ALGORITHMS_MONITOREFFICIENCYCORUSERTEST_H_
#define MANTID_ALGORITHMS_MONITOREFFICIENCYCORUSERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/MonitorEfficiencyCorUser.h"
#include "MantidAPI/Axis.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

using Mantid::Algorithms::MonitorEfficiencyCorUser;

using namespace Mantid;
using namespace Mantid::API;
using namespace Kernel;

class MonitorEfficiencyCorUserTest : public CxxTest::TestSuite {
public:
  static MonitorEfficiencyCorUserTest *createSuite() {
    return new MonitorEfficiencyCorUserTest();
  }
  static void destroySuite(MonitorEfficiencyCorUserTest *suite) {
    delete suite;
  }

  // constructor
  MonitorEfficiencyCorUserTest()
      : m_inWSName("input_workspace"), m_outWSName("output_workspace") {
    m_Ei = 3.27;
    m_monitor_counts = 1000;
    createInputWorkSpace();
  }

  // test init
  void test_Init() {
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void testName() { TS_ASSERT_EQUALS(alg.name(), "MonitorEfficiencyCorUser"); }

  void testVersion() { TS_ASSERT_EQUALS(alg.version(), 1); }

  void test_exec() {

    if (!alg.isInitialized())
      alg.initialize();
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

    // Retrieve the input workspace from data service.
    MatrixWorkspace_sptr inWS;
    TS_ASSERT_THROWS_NOTHING(
        inWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            m_inWSName));
    TS_ASSERT(inWS);
    if (!inWS)
      return;

    // Test equality or proportionality between input and output workspaces
    const size_t xsize = outWS->blocksize();
    double proportionality_coeff = m_monitor_counts * sqrt(m_Ei / 25.3);
    for (size_t i = 0; i < outWS->getNumberHistograms(); ++i) {
      for (size_t j = 0; j < xsize; ++j) { // Same x-values
        TS_ASSERT_DELTA(outWS->readX(i)[j], inWS->readX(i)[j], 1e-12);
        // Output Y-values proportional to input
        TS_ASSERT_DELTA(proportionality_coeff * outWS->readY(i)[j],
                        inWS->readY(i)[j], 1e-12);

        // Output Err-values proportional to input
        TS_ASSERT_DELTA(proportionality_coeff * outWS->readE(i)[j],
                        inWS->readE(i)[j], 1e-12);
      }
    }

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(m_outWSName);
    AnalysisDataService::Instance().remove(m_inWSName);
  }

private:
  double m_Ei;
  int m_monitor_counts;
  const std::string m_inWSName, m_outWSName;
  MonitorEfficiencyCorUser alg;

  void createInputWorkSpace() {
    int numHist = 1;
    int numBins = 20;

    DataObjects::Workspace2D_sptr dataws =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
            numHist, numBins, false, false, true, "TOFTOF");

    HistogramData::BinEdges binEdges = {
        -10.,  -9.25, -8.5,  -7.75, -7.,  -6.25, -5.5, -4.75, -4.,  -3.25, -2.5,
        -1.75, -1.,   -0.25, 0.5,   1.25, 2.,    2.75, 3.5,   4.25, 5.};

    for (size_t wi = 0; wi < dataws->getNumberHistograms(); wi++) {
      dataws->setBinEdges(wi, binEdges);
    }

    dataws->getAxis(0)->setUnit("TOF");
    dataws->mutableRun().addProperty("Ei",
                                     boost::lexical_cast<std::string>(m_Ei));
    dataws->mutableRun().addProperty(
        "monitor_counts", boost::lexical_cast<std::string>(m_monitor_counts));

    dataws->instrumentParameters().addString(
        dataws->getInstrument()->getChild(0).get(), "formula_mon_eff",
        "sqrt(e/25.3)"); // TOFTOF

    API::AnalysisDataService::Instance().addOrReplace(m_inWSName, dataws);
  }
};

#endif /* MANTID_ALGORITHMS_MONITOREFFICIENCYCORUSERTEST_H_ */
