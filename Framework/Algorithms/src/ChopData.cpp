// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ChopData.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectraAxisValidator.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/MultiThreaded.h"

namespace Mantid::Algorithms {
using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;
using namespace HistogramData;

DECLARE_ALGORITHM(ChopData)

void ChopData::init() {
  auto wsVal = std::make_shared<CompositeValidator>();
  wsVal->add<WorkspaceUnitValidator>("TOF");
  wsVal->add<HistogramValidator>();
  wsVal->add<SpectraAxisValidator>();
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input, wsVal),
                  "Name of the input workspace to be split.");
  declareProperty(std::make_unique<WorkspaceProperty<WorkspaceGroup>>("OutputWorkspace", "", Direction::Output),
                  "Name for the WorkspaceGroup that will be created.");

  declareProperty("Step", 20000.0);
  declareProperty("NChops", 5);
  declareProperty("IntegrationRangeLower", EMPTY_DBL());
  declareProperty("IntegrationRangeUpper", EMPTY_DBL());
  declareProperty("MonitorWorkspaceIndex", EMPTY_INT());
}

void ChopData::exec() {
  // Get the input workspace
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  const std::string output = getPropertyValue("OutputWorkspace");
  const double step = getProperty("Step");
  const int chops = getProperty("NChops");
  const double rLower = getProperty("IntegrationRangeLower");
  const double rUpper = getProperty("IntegrationRangeUpper");
  const int monitorWi = getProperty("MonitorWorkspaceIndex");
  const auto nHist = static_cast<int>(inputWS->getNumberHistograms());
  const size_t nBins = inputWS->blocksize();
  const double maxX = inputWS->readX(0)[nBins];
  std::map<int, double> intMap;
  int prelow = -1;
  std::vector<MatrixWorkspace_sptr> workspaces;
  std::unique_ptr<Progress> progress;

  if (maxX < step) {
    throw std::invalid_argument("Step value provided larger than size of workspace.");
  }

  if (rLower != EMPTY_DBL() && rUpper != EMPTY_DBL() && monitorWi != EMPTY_INT()) {

    progress = std::make_unique<Progress>(this, 0.0, 1.0, chops * 2);

    // Select the spectrum that is to be used to compare the sections of the
    // workspace
    // This will generally be the monitor spectrum.
    MatrixWorkspace_sptr monitorWS = create<MatrixWorkspace>(*inputWS, 1, inputWS->histogram(monitorWi));

    int lowest = 0;

    // Get ranges
    for (int i = 0; i < chops; i++) {
      auto integ = createChildAlgorithm("Integration");
      integ->initialize();
      integ->setProperty<MatrixWorkspace_sptr>("InputWorkspace", monitorWS);
      integ->setProperty<double>("RangeLower", i * step + rLower);
      integ->setProperty<double>("RangeUpper", i * step + rUpper);
      integ->execute();
      MatrixWorkspace_sptr integR = integ->getProperty("OutputWorkspace");
      intMap[i] = integR->y(0)[0];

      if (intMap[i] < intMap[lowest]) {
        lowest = i;
      }

      progress->report();
    }

    auto nlow = intMap.find(lowest - 1);
    if (nlow != intMap.end() && intMap[lowest] < (0.1 * nlow->second)) {
      prelow = nlow->first;
    }
  } else {
    progress = std::make_unique<Progress>(this, 0.0, 1.0, chops);
  }

  int wsCounter{1};

  for (int i = 0; i < chops; i++) {
    const double stepDiff = (i * step);

    size_t indexLow, indexHigh;

    try {
      indexLow = inputWS->yIndexOfX(stepDiff);
      if (indexLow < (nBins + 1)) {
        indexLow++;
      }
    } catch (std::out_of_range &) {
      indexLow = 0;
    }

    if (i == prelow) {
      i++;
    }

    try {
      indexHigh = inputWS->yIndexOfX((i + 1) * step);
    } catch (std::out_of_range &) {
      indexHigh = nBins;
    }

    size_t nbins = indexHigh - indexLow;

    MatrixWorkspace_sptr workspace = create<MatrixWorkspace>(*inputWS, BinEdges(nbins + 1));

    // Copy over X, Y and E data
    PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS, *workspace))
    for (int j = 0; j < nHist; j++) {

      auto edges = inputWS->binEdges(j);

      PARALLEL_START_INTERRUPT_REGION;

      workspace->mutableX(j).assign(edges.cbegin() + indexLow, edges.cbegin() + indexLow + nbins + 1);

      workspace->mutableX(j) += -stepDiff;

      workspace->mutableY(j).assign(inputWS->y(j).cbegin() + indexLow, inputWS->y(j).cbegin() + indexLow + nbins);

      workspace->mutableE(j).assign(inputWS->e(j).cbegin() + indexLow, inputWS->e(j).cbegin() + indexLow + nbins);
      PARALLEL_END_INTERRUPT_REGION;
    }
    PARALLEL_CHECK_INTERRUPT_REGION;

    // add the workspace to the AnalysisDataService
    std::stringstream nameStream;
    nameStream << output << "_" << wsCounter;
    std::string wsname = nameStream.str();

    declareProperty(std::make_unique<WorkspaceProperty<>>(wsname, wsname, Direction::Output));
    setProperty(wsname, workspace);
    ++wsCounter;

    workspaces.emplace_back(workspace);

    progress->report();
  }

  // Create workspace group that holds output workspaces
  auto wsgroup = std::make_shared<WorkspaceGroup>();

  for (auto &workspace : workspaces) {
    wsgroup->addWorkspace(workspace);
  }
  // set the output property
  setProperty("OutputWorkspace", wsgroup);
}
} // namespace Mantid::Algorithms
