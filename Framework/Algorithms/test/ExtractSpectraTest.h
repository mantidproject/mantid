#ifndef MANTID_ALGORITHMS_EXTRACTSPECTRATEST_H_
#define MANTID_ALGORITHMS_EXTRACTSPECTRATEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Axis.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAlgorithms/ExtractSpectra.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/UnitFactory.h"

#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/ParallelAlgorithmCreation.h"
#include "MantidTestHelpers/ParallelRunner.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::ExtractSpectra;
using namespace Mantid;
using namespace API;
using namespace Kernel;
using namespace DataObjects;
using namespace HistogramData;

namespace {
void run_parallel_DetectorList_fails(const Parallel::Communicator &comm) {
  Indexing::IndexInfo indexInfo(1000, Parallel::StorageMode::Distributed, comm);
  auto alg = ParallelTestHelpers::create<ExtractSpectra>(comm);
  alg->setProperty("InputWorkspace", create<Workspace2D>(indexInfo, Points(1)));
  alg->setProperty("DetectorList", "1");
  if (comm.size() == 1) {
    TS_ASSERT_THROWS_NOTHING(alg->execute());
  } else {
    TS_ASSERT_THROWS_EQUALS(
        alg->execute(), const std::runtime_error &e, std::string(e.what()),
        "MatrixWorkspace: Using getIndicesFromDetectorIDs in "
        "a parallel run is most likely incorrect. Aborting.");
  }
}

void run_parallel_WorkspaceIndexList(const Parallel::Communicator &comm) {
  Indexing::IndexInfo indexInfo(1000, Parallel::StorageMode::Distributed, comm);
  auto alg = ParallelTestHelpers::create<ExtractSpectra>(comm);
  alg->setProperty("InputWorkspace", create<Workspace2D>(indexInfo, Points(1)));
  alg->setProperty("WorkspaceIndexList", "0-" + std::to_string(comm.size()));
  TS_ASSERT_THROWS_NOTHING(alg->execute());
  MatrixWorkspace_const_sptr out = alg->getProperty("OutputWorkspace");
  TS_ASSERT_EQUALS(out->storageMode(), Parallel::StorageMode::Distributed);
  if (comm.rank() == 0) {
    TS_ASSERT_EQUALS(out->getNumberHistograms(), 2);
  } else {
    TS_ASSERT_EQUALS(out->getNumberHistograms(), 1);
  }
}

void run_parallel_WorkspaceIndexRange(const Parallel::Communicator &comm) {
  Indexing::IndexInfo indexInfo(3 * comm.size(),
                                Parallel::StorageMode::Distributed, comm);
  auto alg = ParallelTestHelpers::create<ExtractSpectra>(comm);
  alg->setProperty("InputWorkspace", create<Workspace2D>(indexInfo, Points(1)));
  alg->setProperty("StartWorkspaceIndex", std::to_string(comm.size() + 1));
  TS_ASSERT_THROWS_NOTHING(alg->execute());
  MatrixWorkspace_const_sptr out = alg->getProperty("OutputWorkspace");
  TS_ASSERT_EQUALS(out->storageMode(), Parallel::StorageMode::Distributed);
  if (comm.rank() == 0) {
    TS_ASSERT_EQUALS(out->getNumberHistograms(), 1);
  } else {
    TS_ASSERT_EQUALS(out->getNumberHistograms(), 2);
  }
}
} // namespace

class ExtractSpectraTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ExtractSpectraTest *createSuite() { return new ExtractSpectraTest(); }
  static void destroySuite(ExtractSpectraTest *suite) {
    AnalysisDataService::Instance().clear();
    delete suite;
  }

  ExtractSpectraTest()
      : nSpec(5), nBins(6), outWSName("ExtractSpectraTest_OutputWS") {}

  void test_Init() {
    ExtractSpectra alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_defaults() {
    Parameters params;
    auto ws = runAlgorithm(params);
    if (!ws)
      return;
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), nSpec);
    TS_ASSERT_EQUALS(ws->blocksize(), nBins);

    TS_ASSERT_EQUALS(ws->x(0)[0], 0.0);
    TS_ASSERT_EQUALS(ws->x(0)[1], 1.0);
    TS_ASSERT_EQUALS(ws->x(0)[2], 2.0);
    TS_ASSERT_EQUALS(ws->x(0)[3], 3.0);
    TS_ASSERT_EQUALS(ws->x(0)[4], 4.0);
    TS_ASSERT_EQUALS(ws->x(0)[5], 5.0);
    TS_ASSERT_EQUALS(ws->x(0)[6], 6.0);
  }

  // ---- test histo ----

  void test_x_range() {
    Parameters params;
    params.setXRange();

    auto ws = runAlgorithm(params);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(ws->getNumberHistograms(), nSpec);
    params.testXRange(*ws);
  }

  void test_index_range() {
    Parameters params;
    params.setIndexRange();

    auto ws = runAlgorithm(params);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(ws->blocksize(), nBins);
    params.testIndexRange(*ws);
  }

  void test_spectrum_list() {
    Parameters params;
    params.setWorkspaceIndexList();

    auto ws = runAlgorithm(params);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(ws->blocksize(), nBins);
    params.testWorkspaceIndexList(*ws);
  }

  void test_index_and_spectrum_list() {
    Parameters params;
    params.setWorkspaceIndexList().setIndexRange();

    auto ws = runAlgorithm(params);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(ws->blocksize(), nBins);
    params.testWorkspaceIndexList(*ws);
  }

  void test_x_range_and_spectrum_list() {
    Parameters params;
    params.setWorkspaceIndexList().setXRange();

    auto ws = runAlgorithm(params);
    if (!ws)
      return;

    params.testXRange(*ws);
    params.testWorkspaceIndexList(*ws);
  }

  void test_invalid_x_range() {
    Parameters params;
    params.setInvalidXRange();

    auto ws = runAlgorithm(params, false);
  }

  void test_invalid_index_range() {
    {
      Parameters params;
      params.setInvalidIndexRange();
      auto ws = runAlgorithm(params, false);
    }
    {
      Parameters params;
      params.setInvalidIndexRange1();
      auto ws = runAlgorithm(params, false);
    }
  }

  void test_detector_list() {
    Parameters params("histo-detector");
    params.setDetectorList();

    auto ws = runAlgorithm(params);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(ws->blocksize(), nBins);
    params.testDetectorList(*ws);
  }

  void test_index_and_detector_list() {
    Parameters params("histo-detector");
    params.setDetectorList().setIndexRange();

    auto ws = runAlgorithm(params);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(ws->blocksize(), nBins);
    params.testDetectorList(*ws);
  }

  void test_x_range_and_detector_list() {
    Parameters params("histo-detector");
    params.setDetectorList().setXRange();

    auto ws = runAlgorithm(params);
    if (!ws)
      return;

    params.testXRange(*ws);
    params.testDetectorList(*ws);
  }

  void test_spectrum_list_and_detector_list() {
    Parameters params("histo-detector");
    params.setWorkspaceIndexList().setDetectorList();

    auto ws = runAlgorithm(params);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(ws->blocksize(), nBins);
    params.testDetectorList(*ws);
  }

  void test_with_dx_data() {
    // Arrange
    Parameters params("histo-dx");

    // Act
    auto ws = runAlgorithm(params);
    if (!ws)
      return;

    // Assert
    TS_ASSERT_EQUALS(ws->blocksize(), nBins);
    params.testDx(*ws);
  }

  // ---- test event ----

  void test_x_range_event() {
    Parameters params("event");
    params.setXRange();

    auto ws = runAlgorithm(params);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(ws->getNumberHistograms(), nSpec);
    params.testXRange(*ws);
  }

  void test_index_range_event() {
    Parameters params("event");
    params.setIndexRange();

    auto ws = runAlgorithm(params);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(ws->blocksize(), nBins);
    params.testIndexRange(*ws);
  }

  void test_spectrum_list_event() {
    Parameters params("event");
    params.setWorkspaceIndexList();

    auto ws = runAlgorithm(params);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(ws->blocksize(), nBins);
    params.testWorkspaceIndexList(*ws);
  }

  void test_index_and_spectrum_list_event() {
    Parameters params("event");
    params.setWorkspaceIndexList().setIndexRange();

    auto ws = runAlgorithm(params);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(ws->blocksize(), nBins);
    params.testWorkspaceIndexList(*ws);
  }

  void test_x_range_and_spectrum_list_event() {
    Parameters params("event");
    params.setWorkspaceIndexList().setXRange();

    auto ws = runAlgorithm(params);
    if (!ws)
      return;

    params.testXRange(*ws);
    params.testWorkspaceIndexList(*ws);
  }

  void test_invalid_x_range_event() {
    Parameters params("event");
    params.setInvalidXRange();
    auto ws = runAlgorithm(params, false);
  }

  void test_invalid_index_range_event() {
    {
      Parameters params("event");
      params.setInvalidIndexRange();
      auto ws = runAlgorithm(params, false);
    }
    {
      Parameters params("event");
      params.setInvalidIndexRange1();
      auto ws = runAlgorithm(params, false);
    }
  }

  void test_detector_list_event() {
    Parameters params("event-detector");
    params.setDetectorList();

    auto ws = runAlgorithm(params);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(ws->blocksize(), nBins);
    params.testDetectorList(*ws);
  }

  void test_index_and_detector_list_event() {
    Parameters params("event-detector");
    params.setDetectorList().setIndexRange();

    auto ws = runAlgorithm(params);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(ws->blocksize(), nBins);
    params.testDetectorList(*ws);
  }

  void test_x_range_and_detector_list_event() {
    Parameters params("event-detector");
    params.setDetectorList().setXRange();

    auto ws = runAlgorithm(params);
    if (!ws)
      return;

    params.testXRange(*ws);
    params.testDetectorList(*ws);
  }

  void test_spectrum_list_and_detector_list_event() {
    Parameters params("event-detector");
    params.setWorkspaceIndexList().setDetectorList();

    auto ws = runAlgorithm(params);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(ws->blocksize(), nBins);
    params.testDetectorList(*ws);
  }

  void test_with_dx_data_event() {
    Parameters params("event-dx");
    auto ws = runAlgorithm(params);

    if (!ws)
      return;
    params.testDx(*ws);
  }

  // ---- test histo-ragged ----

  void test_x_range_ragged() {
    Parameters params("histo-ragged");
    params.setXRange();

    auto ws = runAlgorithm(params);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(ws->getNumberHistograms(), nSpec);
    params.testXRange(*ws);
  }

  void test_index_range_ragged() {
    Parameters params("histo-ragged");
    params.setIndexRange();

    auto ws = runAlgorithm(params);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(ws->blocksize(), nBins);
    params.testIndexRange(*ws);
  }

  void test_spectrum_list_ragged() {
    Parameters params("histo-ragged");
    params.setWorkspaceIndexList();

    auto ws = runAlgorithm(params);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(ws->blocksize(), nBins);
    params.testWorkspaceIndexList(*ws);
  }

  void xtest_invalid_x_range_ragged() {
    Parameters params("histo-ragged");
    params.setInvalidXRange();

    auto ws = runAlgorithm(params, false);
  }

  void test_parallel_DetectorList_fails() {
    ParallelTestHelpers::runParallel(run_parallel_DetectorList_fails);
  }
  void test_parallel_WorkspaceIndexList() {
    ParallelTestHelpers::runParallel(run_parallel_WorkspaceIndexList);
  }
  void test_parallel_WorkspaceIndexRange() {
    ParallelTestHelpers::runParallel(run_parallel_WorkspaceIndexRange);
  }

private:
  // -----------------------  helper methods ------------------------

  const size_t nSpec;
  const size_t nBins;
  const std::string outWSName;

  MatrixWorkspace_sptr
  createInputWorkspace(const std::string &workspaceType) const {
    if (workspaceType == "histo")
      return createInputWorkspaceHisto();
    else if (workspaceType == "event")
      return createInputWorkspaceEvent();
    else if (workspaceType == "histo-ragged")
      return createInputWorkspaceHistoRagged();
    else if (workspaceType == "histo-detector")
      return createInputWithDetectors("histo");
    else if (workspaceType == "event-detector")
      return createInputWithDetectors("event");
    else if (workspaceType == "histo-dx")
      return createInputWorkspaceHistWithDx();
    else if (workspaceType == "event-dx")
      return createInputWorkspaceEventWithDx();
    throw std::runtime_error("Undefined workspace type");
  }

  MatrixWorkspace_sptr createInputWorkspaceHisto() const {
    // Set up a small workspace for testing
    MatrixWorkspace_sptr space = WorkspaceFactory::Instance().create(
        "Workspace2D", nSpec, nBins + 1, nBins);
    for (size_t j = 0; j < nSpec; ++j) {
      for (size_t k = 0; k <= nBins; ++k) {
        space->mutableX(j)[k] = double(k);
      }
      space->mutableY(j) = HistogramData::HistogramY(nBins, double(j));
      space->mutableE(j) = HistogramData::HistogramE(nBins, sqrt(double(j)));
    }
    return space;
  }

  MatrixWorkspace_sptr createInputWorkspaceHistWithDx() const {
    auto ws = createInputWorkspaceHisto();
    // Add the delta x values
    for (size_t j = 0; j < nSpec; ++j) {
      ws->setPointStandardDeviations(j, nBins);
      for (size_t k = 0; k < nBins; ++k) {
        // Add a constant error to all spectra
        ws->mutableDx(j)[k] = sqrt(double(k));
      }
    }
    return ws;
  }

  MatrixWorkspace_sptr createInputWorkspaceHistoRagged() const {
    // Set up a small workspace for testing
    MatrixWorkspace_sptr space = WorkspaceFactory::Instance().create(
        "Workspace2D", nSpec, nBins + 1, nBins);
    for (size_t j = 0; j < nSpec; ++j) {
      auto &X = space->mutableX(j);
      for (size_t k = 0; k <= nBins; ++k) {
        X[k] = double(j + k);
      }
      space->mutableY(j).assign(nBins, double(j + 1));
      space->mutableE(j).assign(nBins, sqrt(double(j + 1)));
    }
    return space;
  }

  MatrixWorkspace_sptr createInputWorkspaceEvent() const {
    EventWorkspace_sptr ws = WorkspaceCreationHelper::createEventWorkspace(
        int(nSpec), int(nBins), 50, 0.0, 1., 2);
    ws->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
    ws->setInstrument(
        ComponentCreationHelper::createTestInstrumentCylindrical(1));
    for (size_t i = 0; i < ws->getNumberHistograms(); ++i) {
      ws->getSpectrum(i).setDetectorID(detid_t(i + 1));
    }
    return ws;
  }

  MatrixWorkspace_sptr createInputWorkspaceEventWithDx() const {
    auto ws = createInputWorkspaceEvent();
    // Add the delta x values
    auto dXvals = HistogramData::PointStandardDeviations(nBins, 0.0);
    auto &dX = dXvals.mutableData();
    for (size_t k = 0; k < nBins; ++k) {
      dX[k] = sqrt(double(k)) + 1;
    }
    for (size_t j = 0; j < nSpec; ++j) {
      ws->setPointStandardDeviations(j, dXvals);
    }
    return ws;
  }

  MatrixWorkspace_sptr
  createInputWithDetectors(std::string workspaceType) const {
    MatrixWorkspace_sptr ws;

    // Set the type of underlying workspace
    if (workspaceType == "histo") {
      ws = createInputWorkspaceHisto();
      for (size_t i = 0; i < ws->getNumberHistograms(); ++i) {
        // Create a detector for each spectra
        ws->getSpectrum(i).setDetectorID(static_cast<detid_t>(i + 1));
      }
    } else if (workspaceType == "event") {
      ws = createInputWorkspaceEvent();
    } else {
      throw std::runtime_error("Undefined workspace type (with detector ids)");
    }
    return ws;
  }

  struct Parameters {
    Parameters(const std::string &workspaceType = "histo")
        : XMin(EMPTY_DBL()), XMax(EMPTY_DBL()), StartWorkspaceIndex(0),
          EndWorkspaceIndex(EMPTY_INT()), WorkspaceIndexList(),
          wsType(workspaceType) {}
    double XMin;
    double XMax;
    int StartWorkspaceIndex;
    int EndWorkspaceIndex;
    std::vector<size_t> WorkspaceIndexList;
    std::vector<detid_t> DetectorList;
    std::string wsType;

    // ---- x range ----
    Parameters &setXRange() {
      XMin = 2.0;
      XMax = 3.1;
      return *this;
    }
    void testXRange(const MatrixWorkspace &ws) const {
      if (wsType == "histo-ragged") {
        TS_ASSERT_EQUALS(ws.blocksize(), 6);
        TS_ASSERT_EQUALS(ws.y(0)[0], 0.0);
        TS_ASSERT_EQUALS(ws.y(0)[1], 0.0);
        TS_ASSERT_EQUALS(ws.y(0)[2], 1.0);
        TS_ASSERT_EQUALS(ws.y(0)[3], 0.0);
        TS_ASSERT_EQUALS(ws.y(0)[4], 0.0);
        TS_ASSERT_EQUALS(ws.y(0)[5], 0.0);

        TS_ASSERT_EQUALS(ws.y(1)[0], 0.0);
        TS_ASSERT_EQUALS(ws.y(1)[1], 2.0);
        TS_ASSERT_EQUALS(ws.y(1)[2], 0.0);
        TS_ASSERT_EQUALS(ws.y(1)[3], 0.0);
        TS_ASSERT_EQUALS(ws.y(1)[4], 0.0);
        TS_ASSERT_EQUALS(ws.y(1)[5], 0.0);

        TS_ASSERT_EQUALS(ws.y(2)[0], 3.0);
        TS_ASSERT_EQUALS(ws.y(2)[1], 0.0);
        TS_ASSERT_EQUALS(ws.y(2)[2], 0.0);
        TS_ASSERT_EQUALS(ws.y(2)[3], 0.0);
        TS_ASSERT_EQUALS(ws.y(2)[4], 0.0);
        TS_ASSERT_EQUALS(ws.y(2)[5], 0.0);

        TS_ASSERT_EQUALS(ws.y(3)[0], 0.0);
        TS_ASSERT_EQUALS(ws.y(3)[1], 0.0);
        TS_ASSERT_EQUALS(ws.y(3)[2], 0.0);
        TS_ASSERT_EQUALS(ws.y(3)[3], 0.0);
        TS_ASSERT_EQUALS(ws.y(3)[4], 0.0);
        TS_ASSERT_EQUALS(ws.y(3)[5], 0.0);
      } else {
        TS_ASSERT_EQUALS(ws.blocksize(), 1);
        TS_ASSERT_EQUALS(ws.x(0)[0], 2.0);
      }
    }

    // ---- index range ----
    Parameters &setIndexRange() {
      StartWorkspaceIndex = 1;
      EndWorkspaceIndex = 3;
      return *this;
    }
    void testIndexRange(const MatrixWorkspace &ws) const {
      TS_ASSERT_EQUALS(ws.getNumberHistograms(), 3);
      if (wsType == "histo") {
        TS_ASSERT_EQUALS(ws.y(0)[0], 1.0);
        TS_ASSERT_EQUALS(ws.y(1)[0], 2.0);
        TS_ASSERT_EQUALS(ws.y(2)[0], 3.0);
      } else if (wsType == "event") {
        const auto &specrumInfo = ws.spectrumInfo();
        TS_ASSERT_EQUALS(specrumInfo.detector(0).getID(), 2);
        TS_ASSERT_EQUALS(specrumInfo.detector(1).getID(), 3);
        TS_ASSERT_EQUALS(specrumInfo.detector(2).getID(), 4);
      }
    }

    // ---- spectrum list ----
    Parameters &setWorkspaceIndexList() {
      WorkspaceIndexList.resize(3);
      WorkspaceIndexList[0] = 0;
      WorkspaceIndexList[1] = 2;
      WorkspaceIndexList[2] = 4;
      return *this;
    }
    void testWorkspaceIndexList(const MatrixWorkspace &ws) const {
      TS_ASSERT_EQUALS(ws.getNumberHistograms(), 3);
      if (wsType == "histo") {
        TS_ASSERT_EQUALS(ws.y(0)[0], 0.0);
        TS_ASSERT_EQUALS(ws.y(1)[0], 2.0);
        TS_ASSERT_EQUALS(ws.y(2)[0], 4.0);
      } else if (wsType == "event") {
        const auto &specrumInfo = ws.spectrumInfo();
        TS_ASSERT_EQUALS(specrumInfo.detector(0).getID(), 1);
        TS_ASSERT_EQUALS(specrumInfo.detector(1).getID(), 3);
        TS_ASSERT_EQUALS(specrumInfo.detector(2).getID(), 5);
      }
    }

    // ---- detector list ----
    Parameters &setDetectorList() {
      DetectorList.resize(3);
      DetectorList[0] = 1; // Translates into WSindex = 0
      DetectorList[1] = 3; // Translates into WSindex = 2
      DetectorList[2] = 5; // Translates into WSindex = 4
      return *this;
    }
    void testDetectorList(const MatrixWorkspace &ws) const {
      TS_ASSERT_EQUALS(ws.getNumberHistograms(), 3);
      if (wsType == "histo-detector") {
        TS_ASSERT_EQUALS(ws.y(0)[0], 0.0);
        TS_ASSERT_EQUALS(ws.y(1)[0], 2.0);
        TS_ASSERT_EQUALS(ws.y(2)[0], 4.0);
      } else if (wsType == "event-detector") {
        const auto &specrumInfo = ws.spectrumInfo();
        TS_ASSERT_EQUALS(specrumInfo.detector(0).getID(), 1);
        TS_ASSERT_EQUALS(specrumInfo.detector(1).getID(), 3);
        TS_ASSERT_EQUALS(specrumInfo.detector(2).getID(), 5);
      }
    }

    // ---- invalid inputs ----
    void setInvalidXRange() {
      XMin = 2.0;
      XMax = 1.0;
    }
    void setInvalidIndexRange() {
      StartWorkspaceIndex = 3;
      EndWorkspaceIndex = 1;
    }
    void setInvalidIndexRange1() {
      StartWorkspaceIndex = 1000;
      EndWorkspaceIndex = 1002;
    }

    // ---- test Dx -------
    void testDx(const MatrixWorkspace &ws) const {
      if (wsType == "histo-dx") {
        TS_ASSERT(ws.hasDx(0));
        TS_ASSERT_EQUALS(ws.dx(0)[0], 0.0);
        TS_ASSERT_EQUALS(ws.dx(0)[1], 1.0);
        TS_ASSERT_EQUALS(ws.dx(0)[2], M_SQRT2);
        TS_ASSERT_EQUALS(ws.dx(0)[3], sqrt(3.0));
        // Check that the length of x and dx differs by 1
        TS_ASSERT_EQUALS(ws.x(0).size() - 1, ws.dx(0).size());
      } else if (wsType == "event-dx") {
        TS_ASSERT(ws.hasDx(0));
        TS_ASSERT_EQUALS(ws.dx(0)[0], 0.0 + 1.0);
        TS_ASSERT_EQUALS(ws.dx(0)[1], 1.0 + 1.0);
        TS_ASSERT_EQUALS(ws.dx(0)[2], M_SQRT2 + 1.0);
        TS_ASSERT_EQUALS(ws.dx(0)[3], sqrt(3.0) + 1.0);
      } else {
        TSM_ASSERT("Should never reach here", false);
      }
    }
  };

  MatrixWorkspace_sptr runAlgorithm(const Parameters &params,
                                    bool expectSuccess = true) const {
    auto ws = createInputWorkspace(params.wsType);
    ExtractSpectra alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));

    if (params.XMin != EMPTY_DBL()) {
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("XMin", params.XMin));
    }
    if (params.XMax != EMPTY_DBL()) {
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("XMax", params.XMax));
    }
    if (params.StartWorkspaceIndex != 0) {
      TS_ASSERT_THROWS_NOTHING(
          alg.setProperty("StartWorkspaceIndex", params.StartWorkspaceIndex));
    }
    if (params.EndWorkspaceIndex != EMPTY_INT()) {
      TS_ASSERT_THROWS_NOTHING(
          alg.setProperty("EndWorkspaceIndex", params.EndWorkspaceIndex));
    }
    if (!params.WorkspaceIndexList.empty()) {
      TS_ASSERT_THROWS_NOTHING(
          alg.setProperty("WorkspaceIndexList", params.WorkspaceIndexList));
    }
    if (!params.DetectorList.empty()) {
      TS_ASSERT_THROWS_NOTHING(
          alg.setProperty("DetectorList", params.DetectorList));
    }

    TS_ASSERT_THROWS_NOTHING(alg.execute(););

    if (expectSuccess) {
      TS_ASSERT(alg.isExecuted());

      // Retrieve the workspace from data service. TODO: Change to your desired
      // type
      MatrixWorkspace_sptr ws;
      TS_ASSERT_THROWS_NOTHING(
          ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
              outWSName));
      return ws;
    } else {
      TS_ASSERT(!alg.isExecuted());
    }

    return MatrixWorkspace_sptr();
  }
};

class ExtractSpectraTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ExtractSpectraTestPerformance *createSuite() {
    return new ExtractSpectraTestPerformance();
  }
  static void destroySuite(ExtractSpectraTestPerformance *suite) {
    AnalysisDataService::Instance().clear();
    delete suite;
  }

  ExtractSpectraTestPerformance() {
    input = WorkspaceCreationHelper::create2DWorkspaceBinned(40000, 10000);
    inputEvent =
        WorkspaceCreationHelper::createEventWorkspace(40000, 10000, 2000);
  }

  void testExec2D() {
    Algorithms::ExtractSpectra alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", input);
    alg.setProperty("EndWorkspaceIndex", 30000);
    alg.setPropertyValue("OutputWorkspace", "ExtractSpectra2DOut");
    alg.execute();
  }

  void testExecEvent() {
    Algorithms::ExtractSpectra alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", inputEvent);
    alg.setProperty("EndWorkspaceIndex", 30000);
    alg.setPropertyValue("OutputWorkspace", "ExtractSpectraEventOut");
    alg.execute();
  }

private:
  MatrixWorkspace_sptr input;
  EventWorkspace_sptr inputEvent;
};

#endif /* MANTID_ALGORITHMS_EXTRACTSPECTRATEST_H_ */
