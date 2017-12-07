#ifndef MANTID_ALGORITHMS_POLARIZATIONEFFICIENCYCORTEST_H_
#define MANTID_ALGORITHMS_POLARIZATIONEFFICIENCYCORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/PolarizationEfficiencyCor.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"

using Mantid::Algorithms::PolarizationEfficiencyCor;

class PolarizationEfficiencyCorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PolarizationEfficiencyCorTest *createSuite() { return new PolarizationEfficiencyCorTest(); }
  static void destroySuite( PolarizationEfficiencyCorTest *suite ) { delete suite; }

  void tearDown() override {
    using namespace Mantid::API;
    AnalysisDataService::Instance().clear();
  }

  void test_Init() {
    PolarizationEfficiencyCor alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_exec() {
    using namespace Mantid::API;
    using namespace Mantid::DataObjects;
    using namespace Mantid::HistogramData;
    using namespace Mantid::Kernel;
    constexpr size_t nBins{3};
    constexpr size_t nHist{2};
    BinEdges edges{0.3, 0.6, 0.9, 1.2};
    const double yVal = 2.3;
    Counts counts{yVal, yVal, yVal};
    MatrixWorkspace_sptr ws00 = create<Workspace2D>(nHist, Histogram(edges, counts));
    MatrixWorkspace_sptr ws01 = WorkspaceFactory::Instance().create(ws00);
    MatrixWorkspace_sptr ws10 = WorkspaceFactory::Instance().create(ws00);
    MatrixWorkspace_sptr ws11 = WorkspaceFactory::Instance().create(ws00);
    WorkspaceGroup_sptr inputWS = boost::make_shared<WorkspaceGroup>();
    inputWS->addWorkspace(boost::dynamic_pointer_cast<Workspace>(ws00));
    inputWS->addWorkspace(boost::dynamic_pointer_cast<Workspace>(ws01));
    inputWS->addWorkspace(boost::dynamic_pointer_cast<Workspace>(ws10));
    inputWS->addWorkspace(boost::dynamic_pointer_cast<Workspace>(ws11));
    auto effWS = efficiencies(edges);
    PolarizationEfficiencyCor alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Efficiencies", effWS))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    WorkspaceGroup_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    TS_ASSERT_EQUALS(outputWS->getNumberOfEntries(), 4);
    MatrixWorkspace_sptr out00 = boost::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(0));
    TS_ASSERT(out00)
    TS_ASSERT_EQUALS(out00->getNumberHistograms(), nHist)
    for (size_t i = 0; i != nHist; ++i) {
      const auto &xs = out00->x(i);
      const auto &ys = out00->y(i);
      const auto &es = out00->e(i);
      TS_ASSERT_EQUALS(ys.size(), nBins)
      for (size_t j = 0; j != nBins; ++j) {
        TS_ASSERT_EQUALS(xs[j], edges[j])
        TS_ASSERT_EQUALS(ys[j], yVal)
        TS_ASSERT_EQUALS(es[j], std::sqrt(yVal))
      }
    }
  }

private:
  Mantid::API::MatrixWorkspace_sptr efficiencies(const Mantid::HistogramData::BinEdges &edges) {
    using namespace Mantid::API;
    using namespace Mantid::DataObjects;
    using namespace Mantid::HistogramData;
    using namespace Mantid::Kernel;
    const auto nBins = edges.size() - 1;
    constexpr size_t nHist{4};
    Counts counts(nBins, 0.0);
    MatrixWorkspace_sptr ws = create<Workspace2D>(nHist, Histogram(edges, counts));
    ws->mutableY(0) = 1.;
    ws->mutableY(1) = 1.;
    auto axis = make_unique<TextAxis>(4);
    axis->setLabel(0, "F1");
    axis->setLabel(1, "F2");
    axis->setLabel(2, "P1");
    axis->setLabel(3, "P2");
    ws->replaceAxis(1, axis.release());
    return ws;
  }
};


#endif /* MANTID_ALGORITHMS_POLARIZATIONEFFICIENCYCORTEST_H_ */
