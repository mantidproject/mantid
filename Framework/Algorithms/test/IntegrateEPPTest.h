// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_INTEGRATEEPPTEST_H_
#define MANTID_ALGORITHMS_INTEGRATEEPPTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/IntegrateEPP.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::IntegrateEPP;

class IntegrateEPPTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IntegrateEPPTest *createSuite() { return new IntegrateEPPTest(); }
  static void destroySuite(IntegrateEPPTest *suite) { delete suite; }

  void test_Init() {
    IntegrateEPP alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_normal_operation() {
    using namespace Mantid::API;
    using namespace WorkspaceCreationHelper;
    const size_t nHist = 3;
    const size_t nBins = 6;
    auto inputWS = create2DWorkspaceWhereYIsWorkspaceIndex(nHist, nBins + 1);
    std::vector<EPPTableRow> eppRows(nHist);
    for (auto &row : eppRows) {
      row.peakCentre = static_cast<double>(nBins + 1) / 2.0;
      row.sigma = 1;
    }
    auto eppWS = createEPPTableWorkspace(eppRows);
    IntegrateEPP alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("EPPWorkspace", eppWS))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("HalfWidthInSigmas", 1.0))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), nHist)
    TS_ASSERT_EQUALS(outputWS->blocksize(), 1)
    for (size_t i = 0; i < outputWS->getNumberHistograms(); ++i) {
      const auto &ys = outputWS->y(i);
      const auto &xs = outputWS->x(i);
      TS_ASSERT_EQUALS(ys[0], 2.0 * static_cast<double>(i))
      TS_ASSERT_EQUALS(xs[0], 2.5)
      TS_ASSERT_EQUALS(xs[1], 4.5)
    }
  }

  void test_WorkspaceIndex_column_is_respected() {
    using namespace Mantid::API;
    using namespace WorkspaceCreationHelper;
    const size_t nHist = 3;
    const size_t nBins = 6;
    auto inputWS = create2DWorkspaceWhereYIsWorkspaceIndex(nHist, nBins + 1);
    std::vector<EPPTableRow> eppRows;
    const double centre = static_cast<double>(nBins + 1) / 2.0;
    const double sigma = 1.0;
    eppRows.emplace_back(2, centre, sigma, 0.0,
                         EPPTableRow::FitStatus::SUCCESS);
    eppRows.emplace_back(0, centre, sigma, 0.0,
                         EPPTableRow::FitStatus::SUCCESS);
    auto eppWS = createEPPTableWorkspace(eppRows);
    IntegrateEPP alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("EPPWorkspace", eppWS))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("HalfWidthInSigmas", 1.0))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), nHist)
    TS_ASSERT_EQUALS(outputWS->blocksize(), 1)
    TS_ASSERT_EQUALS(outputWS->y(0)[0], 2.0 * 0.0)
    TS_ASSERT_EQUALS(outputWS->x(0)[0], 2.5)
    TS_ASSERT_EQUALS(outputWS->x(0)[1], 4.5)
    TS_ASSERT_EQUALS(outputWS->y(1)[0], 0.0)
    TS_ASSERT_EQUALS(outputWS->x(1)[0], 0.0)
    TS_ASSERT_EQUALS(outputWS->x(1)[1], 0.0)
    TS_ASSERT_EQUALS(outputWS->y(2)[0], 2.0 * 2.0)
    TS_ASSERT_EQUALS(outputWS->x(2)[0], 2.5)
    TS_ASSERT_EQUALS(outputWS->x(2)[1], 4.5)
  }

  void test_failure_too_many_epp_rows() {
    using namespace Mantid::API;
    using namespace WorkspaceCreationHelper;
    const size_t nHist = 3;
    const size_t nBins = 6;
    auto inputWS = create2DWorkspaceWhereYIsWorkspaceIndex(nHist, nBins + 1);
    std::vector<EPPTableRow> eppRows(nHist + 1);
    for (auto &row : eppRows) {
      row.peakCentre = static_cast<double>(nBins + 1) / 2.0;
      row.sigma = 1;
    }
    auto eppWS = createEPPTableWorkspace(eppRows);
    IntegrateEPP alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("EPPWorkspace", eppWS))
    TS_ASSERT_THROWS_ANYTHING(alg.execute())
    TS_ASSERT(!alg.isExecuted())
  }

  void test_failure_invalid_index_in_epp_workspace() {
    using namespace Mantid::API;
    using namespace WorkspaceCreationHelper;
    const size_t nHist = 3;
    const size_t nBins = 6;
    auto inputWS = create2DWorkspaceWhereYIsWorkspaceIndex(nHist, nBins + 1);
    std::vector<EPPTableRow> eppRows;
    const double centre = static_cast<double>(nBins + 1) / 2.0;
    const double sigma = 1.0;
    eppRows.emplace_back(3, centre, sigma, 0.0,
                         EPPTableRow::FitStatus::SUCCESS);
    auto eppWS = createEPPTableWorkspace(eppRows);
    IntegrateEPP alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("EPPWorkspace", eppWS))
    TS_ASSERT_THROWS_ANYTHING(alg.execute())
    TS_ASSERT(!alg.isExecuted())
  }
};

#endif /* MANTID_ALGORITHMS_INTEGRATEEPPTEST_H_ */
