#ifndef MANTID_ALGORITHMS_DETECTOREFFICIENCYCORUSERTEST_H_
#define MANTID_ALGORITHMS_DETECTOREFFICIENCYCORUSERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Axis.h"
#include "MantidAlgorithms/DetectorEfficiencyCorUser.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

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

  // constructor
  DetectorEfficiencyCorUserTest()
      : m_Efs(m_numBins), m_inWSName("input_workspace"),
        m_outWSName("output_workspace") {
    for (size_t i = 0; i != m_Efs.size(); ++i) {
      m_Efs[i] = 0.1 + 0.2 * static_cast<double>(i);
    }
    std::reverse(m_Efs.begin(), m_Efs.end());

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

    const auto nHisto = outWS->getNumberHistograms();
    for (size_t i = 0; i != nHisto; ++i) {
      auto eff0 =
          i < nHisto / 2 ? efficiencyBank1(m_Ei) : efficiencyBank2(m_Ei);
      const auto &ys = outWS->counts(i);
      const auto &es = outWS->countStandardDeviations(i);
      for (size_t j = 0; j != ys.size(); ++j) {
        const auto eff = i < nHisto / 2 ? efficiencyBank1(m_Efs[j])
                                        : efficiencyBank2(m_Efs[j]);
        // By default, input workspace has y = 2, e = sqrt(2).
        TS_ASSERT_DELTA(ys[j], 2 * eff0 / eff, 1e-6);
        TS_ASSERT_DELTA(es[j], std::sqrt(2) * eff0 / eff, 1e-6);
      }
    }

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(m_outWSName);
  }

private:
  // Initial energy.
  static constexpr double m_Ei = 3.27;
  static const int m_numBins = 20;

  static double efficiencyBank1(const double e) {
    return std::exp(-1.0 / std::sqrt(e)) *
           (1.0 - std::exp(-1.0 / std::sqrt(e)));
  }

  static double efficiencyBank2(const double e) {
    return std::exp(-2.0 / std::sqrt(e)) *
           (1.0 - std::exp(-2.0 / std::sqrt(e)));
  }

  // Final energies.
  std::vector<double> m_Efs;
  const std::string m_inWSName, m_outWSName;

  void createInputWorkSpace() const {
    int numBanks = 2;
    int numPixels = 10;

    DataObjects::Workspace2D_sptr dataws =
        WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(
            numBanks, numPixels, m_numBins);

    // Prepare energy bins.
    std::vector<double> xs(m_Efs.size());
    for (size_t i = 0; i != xs.size(); ++i) {
      xs[i] = m_Ei - m_Efs[i];
    }

    for (size_t wi = 0; wi < dataws->getNumberHistograms(); wi++) {
      dataws->setBinEdges(wi,
                          HistogramData::BinEdges(HistogramData::Points(xs)));
    }
    dataws->getAxis(0)->setUnit("DeltaE");
    dataws->mutableRun().addProperty("Ei", double(m_Ei));
    // Efficiency formula should be the same as in efficiency().
    const auto instrument = dataws->getInstrument();
    auto bank = instrument->getComponentByName("bank1");
    dataws->instrumentParameters().addString(
        bank.get(), "formula_eff", "exp(-1/sqrt(e))*(1-exp(-1/sqrt(e)))");
    bank = instrument->getComponentByName("bank2");
    dataws->instrumentParameters().addString(
        bank.get(), "formula_eff", "exp(-2/sqrt(e))*(1-exp(-2/sqrt(e)))");
    API::AnalysisDataService::Instance().addOrReplace(m_inWSName, dataws);
  }
};

#endif /* MANTID_ALGORITHMS_DETECTOREFFICIENCYCORUSERTEST_H_ */
