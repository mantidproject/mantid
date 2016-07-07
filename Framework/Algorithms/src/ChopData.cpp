#include "MantidAlgorithms/ChopData.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/SpectraAxisValidator.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/MultiThreaded.h"

namespace Mantid {
namespace Algorithms {
using namespace Kernel;
using namespace API;
using namespace Geometry;

DECLARE_ALGORITHM(ChopData)

void ChopData::init() {
  auto wsVal = boost::make_shared<CompositeValidator>();
  wsVal->add<WorkspaceUnitValidator>("TOF");
  wsVal->add<HistogramValidator>();
  wsVal->add<SpectraAxisValidator>();
  declareProperty(make_unique<WorkspaceProperty<>>("InputWorkspace", "",
                                                   Direction::Input, wsVal),
                  "Name of the input workspace to be split.");
  declareProperty(make_unique<WorkspaceProperty<WorkspaceGroup>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Name for the WorkspaceGroup that will be created.");

  declareProperty("Step", 20000.0);
  declareProperty("NChops", 5);
  declareProperty("IntegrationRangeLower", EMPTY_DBL());
  declareProperty("IntegrationRangeUpper", EMPTY_DBL());
  declareProperty("MonitorWorkspaceIndex", EMPTY_INT());
}

void ChopData::exec() {
  // Get the input workspace
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  const std::string output = getPropertyValue("OutputWorkspace");
  const double step = getProperty("Step");
  const int chops = getProperty("NChops");
  const double rLower = getProperty("IntegrationRangeLower");
  const double rUpper = getProperty("IntegrationRangeUpper");
  const int monitorWi = getProperty("MonitorWorkspaceIndex");
  const int nHist = static_cast<int>(inputWS->getNumberHistograms());
  const size_t nBins = inputWS->blocksize();
  const double maxX = inputWS->readX(0)[nBins];
  std::map<int, double> intMap;
  int prelow = -1;
  std::vector<MatrixWorkspace_sptr> workspaces;

  boost::shared_ptr<Progress> progress;

  if (maxX < step) {
    throw std::invalid_argument(
        "Step value provided larger than size of workspace.");
  }

  if (rLower != EMPTY_DBL() && rUpper != EMPTY_DBL() &&
      monitorWi != EMPTY_INT()) {

    progress = boost::make_shared<Progress>(this, 0, 1, chops * 2);

    // Select the spectrum that is to be used to compare the sections of the
    // workspace
    // This will generally be the monitor spectrum.
    MatrixWorkspace_sptr monitorWS;
    monitorWS = WorkspaceFactory::Instance().create(inputWS, 1);
    monitorWS->setHistogram(0, inputWS->histogram(monitorWi));

    int lowest = 0;

    // Get ranges
    for (int i = 0; i < chops; i++) {
      Mantid::API::IAlgorithm_sptr integ =
          Mantid::API::Algorithm::createChildAlgorithm("Integration");
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
  } else
    progress = boost::make_shared<Progress>(this, 0, 1, chops);

  int wsCounter(1);

  for (int i = 0; i < chops; i++) {
    const double stepDiff = (i * step);

    size_t indexLow, indexHigh;

    try {
      indexLow = inputWS->binIndexOf(stepDiff);
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
      indexHigh = inputWS->binIndexOf((i + 1) * step);
    } catch (std::out_of_range &) {
      indexHigh = nBins;
    }

    size_t nbins = indexHigh - indexLow;

    MatrixWorkspace_sptr workspace =
        Mantid::API::WorkspaceFactory::Instance().create(inputWS, nHist,
                                                         nbins + 1, nbins);

    // Copy over X, Y and E data
    PARALLEL_FOR2(inputWS, workspace)
    for (int j = 0; j < nHist; j++) {
      PARALLEL_START_INTERUPT_REGION;
      for (size_t k = 0; k < nbins; k++) {
        size_t oldbin = indexLow + k;
        workspace->mutableY(j)[k] = inputWS->y(j)[oldbin];
        workspace->mutableE(j)[k] = inputWS->e(j)[oldbin];
        workspace->mutableX(j)[k] = inputWS->x(j)[oldbin] - stepDiff;
      }
      workspace->mutableX(j)[nbins] =
          inputWS->x(j)[indexLow + nbins] - stepDiff;

      PARALLEL_END_INTERUPT_REGION;
    }
    PARALLEL_CHECK_INTERUPT_REGION;

    // add the workspace to the AnalysisDataService
    std::stringstream name;
    name << output << "_" << wsCounter;
    std::string wsname = name.str();

    declareProperty(Kernel::make_unique<WorkspaceProperty<>>(
        wsname, wsname, Direction::Output));
    setProperty(wsname, workspace);
    ++wsCounter;

    workspaces.push_back(workspace);

    progress->report();
  }

  // Create workspace group that holds output workspaces
  auto wsgroup = boost::make_shared<WorkspaceGroup>();

  for (auto &workspace : workspaces) {
    wsgroup->addWorkspace(workspace);
  }
  // set the output property
  setProperty("OutputWorkspace", wsgroup);
}
} // namespace Algorithms
} // namespace Mantid
