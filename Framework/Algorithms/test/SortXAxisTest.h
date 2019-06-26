// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_SORTXAXISTEST_H_
#define MANTID_ALGORITHMS_SORTXAXISTEST_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/SortXAxis.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Histogram.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;

namespace {
MatrixWorkspace_sptr createWorkspaceE(const std::vector<double> &xData,
                                      const std::vector<double> &yData,
                                      const std::vector<double> &eData,
                                      const int nSpec = 1) {

  Workspace2D_sptr outputWorkspace = create<DataObjects::Workspace2D>(
      nSpec, Mantid::HistogramData::Histogram(
                 Mantid::HistogramData::Points(xData.size())));
  for (int i = 0; i < nSpec; ++i) {
    outputWorkspace->mutableY(i) = yData;
    outputWorkspace->mutableE(i) = eData;
    outputWorkspace->mutableX(i) = xData;
  }
  return outputWorkspace;
}

MatrixWorkspace_sptr createHistoWorkspaceE(const std::vector<double> &xData,
                                           const std::vector<double> &yData,
                                           const std::vector<double> &eData,
                                           const int nSpec = 1) {

  Workspace2D_sptr outputWorkspace = create<DataObjects::Workspace2D>(
      nSpec, Mantid::HistogramData::Histogram(
                 Mantid::HistogramData::BinEdges(xData.size())));
  for (int i = 0; i < nSpec; ++i) {
    outputWorkspace->mutableY(i) = yData;
    outputWorkspace->mutableE(i) = eData;
    outputWorkspace->mutableX(i) = xData;
  }
  return outputWorkspace;
}

MatrixWorkspace_sptr createWorkspaceDx(const std::vector<double> &xData,
                                       const std::vector<double> &yData,
                                       const std::vector<double> &dxData,
                                       const int nSpec = 1) {

  Workspace2D_sptr outputWorkspace = create<DataObjects::Workspace2D>(
      nSpec, Mantid::HistogramData::Histogram(
                 Mantid::HistogramData::Points(xData.size())));
  for (int i = 0; i < nSpec; ++i) {
    outputWorkspace->mutableY(i) = yData;
    outputWorkspace->mutableX(i) = xData;
    outputWorkspace->setPointStandardDeviations(i, dxData);
  }
  return outputWorkspace;
}

MatrixWorkspace_sptr createHistoWorkspaceDx(const std::vector<double> &xData,
                                            const std::vector<double> &yData,
                                            const std::vector<double> &dxData,
                                            const int nSpec = 1) {

  Workspace2D_sptr outputWorkspace = create<DataObjects::Workspace2D>(
      nSpec, Mantid::HistogramData::Histogram(
                 Mantid::HistogramData::BinEdges(xData.size())));
  for (int i = 0; i < nSpec; ++i) {
    outputWorkspace->mutableY(i) = yData;
    outputWorkspace->mutableX(i) = xData;
    outputWorkspace->setPointStandardDeviations(i, dxData);
  }
  return outputWorkspace;
}

MatrixWorkspace_sptr createHistoWorkspace(const std::vector<double> &xData,
                                          const std::vector<double> &yData,
                                          const int nSpec = 1) {

  Workspace2D_sptr outputWorkspace = create<DataObjects::Workspace2D>(
      nSpec, Mantid::HistogramData::Histogram(
                 Mantid::HistogramData::BinEdges(xData.size())));
  for (int i = 0; i < nSpec; ++i) {
    outputWorkspace->mutableY(i) = yData;
    outputWorkspace->mutableX(i) = xData;
  }
  return outputWorkspace;
}
} // namespace

class SortXAxisTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SortXAxisTest *createSuite() { return new SortXAxisTest(); }
  static void destroySuite(SortXAxisTest *suite) { delete suite; }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void testXAscending() {
    std::vector<double> xData = {1, 2, 3};
    std::vector<double> yData = {1, 2, 3};
    std::vector<double> eData = {1, 2, 3};

    MatrixWorkspace_sptr unsortedws = createWorkspaceE(xData, yData, eData);

    SortXAxis alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", unsortedws));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "sortedws"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    MatrixWorkspace_sptr sortedws;
    TS_ASSERT_THROWS_NOTHING(
        sortedws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "sortedws"));
    TS_ASSERT(sortedws);

    TS_ASSERT_EQUALS(sortedws->x(0).rawData(), xData);
    TS_ASSERT_EQUALS(sortedws->y(0).rawData(), yData);
    TS_ASSERT_EQUALS(sortedws->e(0).rawData(), eData);

    TS_ASSERT_THROWS_NOTHING(
        AnalysisDataService::Instance().remove("unsortedws"));
    TS_ASSERT_THROWS_NOTHING(
        AnalysisDataService::Instance().remove("sortedws"));
  }

  void testXDescending() {
    std::vector<double> xData = {3, 2, 1};
    std::vector<double> sortedXData = {1, 2, 3};
    std::vector<double> yData = {1, 2, 3};
    std::vector<double> reverseYData = {3, 2, 1};
    std::vector<double> eData = {1, 2, 3};
    std::vector<double> reverseEData = {3, 2, 1};

    MatrixWorkspace_sptr unsortedws = createWorkspaceE(xData, yData, eData);

    SortXAxis alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", unsortedws));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "sortedws"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    MatrixWorkspace_sptr sortedws;
    TS_ASSERT_THROWS_NOTHING(
        sortedws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "sortedws"));
    TS_ASSERT(sortedws);

    TS_ASSERT_EQUALS(sortedws->x(0).rawData(), sortedXData);
    TS_ASSERT_EQUALS(sortedws->y(0).rawData(), reverseYData);
    TS_ASSERT_EQUALS(sortedws->e(0).rawData(), reverseEData);

    TS_ASSERT_THROWS_NOTHING(
        AnalysisDataService::Instance().remove("unsortedws"));
    TS_ASSERT_THROWS_NOTHING(
        AnalysisDataService::Instance().remove("sortedws"));
  }

  void testOnMultipleSpectrum() {
    std::vector<double> xData = {3, 2, 1};
    std::vector<double> sortedXData = {1, 2, 3};
    std::vector<double> yData = {1, 2, 3};
    std::vector<double> reverseYData = {3, 2, 1};
    std::vector<double> eData = {1, 2, 3};
    std::vector<double> reverseEData = {3, 2, 1};

    MatrixWorkspace_sptr unsortedws = createWorkspaceE(xData, yData, eData, 2);

    SortXAxis alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", unsortedws));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "sortedws"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    MatrixWorkspace_sptr sortedws;
    TS_ASSERT_THROWS_NOTHING(
        sortedws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "sortedws"));
    TS_ASSERT(sortedws);

    TS_ASSERT_EQUALS(sortedws->x(0).rawData(), sortedXData);
    TS_ASSERT_EQUALS(sortedws->y(0).rawData(), reverseYData);
    TS_ASSERT_EQUALS(sortedws->e(0).rawData(), reverseEData);

    TS_ASSERT_EQUALS(sortedws->x(1).rawData(), sortedXData);
    TS_ASSERT_EQUALS(sortedws->y(1).rawData(), reverseYData);
    TS_ASSERT_EQUALS(sortedws->e(1).rawData(), reverseEData);

    TS_ASSERT_THROWS_NOTHING(
        AnalysisDataService::Instance().remove("unsortedws"));
    TS_ASSERT_THROWS_NOTHING(
        AnalysisDataService::Instance().remove("sortedws"));
  }

  void testSortsXHistogramAscending() {
    std::vector<double> xData = {1, 2, 3, 4};
    std::vector<double> yData = {1, 2, 3};
    std::vector<double> eData = {1, 2, 3};

    MatrixWorkspace_sptr unsortedws =
        createHistoWorkspaceE(xData, yData, eData);

    SortXAxis alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", unsortedws));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "sortedws"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    MatrixWorkspace_sptr sortedws;
    TS_ASSERT_THROWS_NOTHING(
        sortedws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "sortedws"));
    TS_ASSERT(sortedws);

    TS_ASSERT_EQUALS(sortedws->x(0).rawData(), xData);
    TS_ASSERT_EQUALS(sortedws->y(0).rawData(), yData);
    TS_ASSERT_EQUALS(sortedws->e(0).rawData(), eData);

    TS_ASSERT_THROWS_NOTHING(
        AnalysisDataService::Instance().remove("unsortedws"));
    TS_ASSERT_THROWS_NOTHING(
        AnalysisDataService::Instance().remove("sortedws"));
  }

  void testSortsXHistogramDescending() {
    std::vector<double> xData = {4, 3, 2, 1};
    std::vector<double> sortedXData = {1, 2, 3, 4};
    std::vector<double> yData = {1, 2, 3};
    std::vector<double> reverseYData = {3, 2, 1};
    std::vector<double> eData = {1, 2, 3};
    std::vector<double> reverseEData = {3, 2, 1};

    MatrixWorkspace_sptr unsortedws =
        createHistoWorkspaceE(xData, yData, eData);

    SortXAxis alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", unsortedws));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "sortedws"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    MatrixWorkspace_sptr sortedws;
    TS_ASSERT_THROWS_NOTHING(
        sortedws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "sortedws"));
    TS_ASSERT(sortedws);

    TS_ASSERT_EQUALS(sortedws->x(0).rawData(), sortedXData);
    TS_ASSERT_EQUALS(sortedws->y(0).rawData(), reverseYData);
    TS_ASSERT_EQUALS(sortedws->e(0).rawData(), reverseEData);

    TS_ASSERT_THROWS_NOTHING(
        AnalysisDataService::Instance().remove("unsortedws"));
    TS_ASSERT_THROWS_NOTHING(
        AnalysisDataService::Instance().remove("sortedws"));
  }

  void testDxMultipleSpectrum() {
    std::vector<double> xData = {3, 2, 1};
    std::vector<double> yData = {1, 2, 3};
    std::vector<double> dxData = {1, 2, 3};
    std::vector<double> reverseDxData = {3, 2, 1};

    MatrixWorkspace_sptr unsortedws =
        createWorkspaceDx(xData, yData, dxData, 2);

    SortXAxis alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", unsortedws));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "sortedws"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    MatrixWorkspace_sptr sortedws;
    TS_ASSERT_THROWS_NOTHING(
        sortedws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "sortedws"));
    TS_ASSERT(sortedws);

    TS_ASSERT_EQUALS(sortedws->dx(0).rawData(), reverseDxData);
    TS_ASSERT_EQUALS(sortedws->dx(1).rawData(), reverseDxData);

    TS_ASSERT_THROWS_NOTHING(
        AnalysisDataService::Instance().remove("unsortedws"));
    TS_ASSERT_THROWS_NOTHING(
        AnalysisDataService::Instance().remove("sortedws"));
  }

  void testDxHistogramAscending() {
    std::vector<double> xData = {1, 2, 3, 4};
    std::vector<double> yData = {1, 2, 3};
    std::vector<double> dxData = {1, 2, 3};

    MatrixWorkspace_sptr unsortedws =
        createHistoWorkspaceDx(xData, yData, dxData, 2);

    SortXAxis alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", unsortedws));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "sortedws"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    MatrixWorkspace_sptr sortedws;
    TS_ASSERT_THROWS_NOTHING(
        sortedws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "sortedws"));
    TS_ASSERT(sortedws);

    TS_ASSERT_EQUALS(sortedws->dx(0).rawData(), dxData);

    TS_ASSERT_THROWS_NOTHING(
        AnalysisDataService::Instance().remove("unsortedws"));
    TS_ASSERT_THROWS_NOTHING(
        AnalysisDataService::Instance().remove("sortedws"));
  }

  void testSortDescending() {
    std::vector<double> xData = {1, 2, 3, 4};
    std::vector<double> reverseXData = {4, 3, 2, 1};
    std::vector<double> yData = {1, 2, 3};
    std::vector<double> reverseYData = {3, 2, 1};

    MatrixWorkspace_sptr unsortedws = createHistoWorkspace(xData, yData, 2);

    SortXAxis alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", unsortedws));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "sortedws"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Ordering", "Descending"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    MatrixWorkspace_sptr sortedws;
    TS_ASSERT_THROWS_NOTHING(
        sortedws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "sortedws"));
    TS_ASSERT(sortedws);

    TS_ASSERT_EQUALS(sortedws->x(0).rawData(), reverseXData);
    TS_ASSERT_EQUALS(sortedws->y(0).rawData(), reverseYData);

    TS_ASSERT_THROWS_NOTHING(
        AnalysisDataService::Instance().remove("unsortedws"));
    TS_ASSERT_THROWS_NOTHING(
        AnalysisDataService::Instance().remove("sortedws"));
  }
};
#endif /*MANTID_ALGORITHMS_SORTXAXISTEST_H_*/
