#include "MantidAlgorithms/CreateFloodWorkspace.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MultipleFileProperty.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace {
namespace Prop {
static std::string const FILENAME("Filename");
static std::string const OUTPUT_WORKSPACE("OutputWorkspace");
static std::string const START_X("StartSpectrumIndex");
static std::string const END_X("EndSpectrumIndex");
} // namespace Prop

} // namespace

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CreateFloodWorkspace)

const std::string CreateFloodWorkspace::name() const {
  return "CreateFloodWorkspace";
};

const std::string CreateFloodWorkspace::summary() const {
  return "Algorithm to create a flood correction workspace for reflectometry "
         "data reduction.";
}

int CreateFloodWorkspace::version() const { return 1; };

const std::vector<std::string> CreateFloodWorkspace::seeAlso() const {
  return {"ReflectometryReductionOneAuto"};
}

const std::string CreateFloodWorkspace::category() const {
  return "Reflectometry\\ISIS";
}

/** Initialize the algorithm's properties.
 */
void CreateFloodWorkspace::init() {
  const FacilityInfo &defaultFacility = ConfigService::Instance().getFacility();
  std::vector<std::string> exts = defaultFacility.extensions();

  declareProperty(make_unique<MultipleFileProperty>(
                      Prop::FILENAME, exts),
                  "The name of the fllod run file(s) to read. Multiple runs "
                  "can be loaded and added together, e.g. INST10+11+12+13.ext");

  declareProperty(Prop::START_X, EMPTY_DBL(), "Start value of the fitting interval");
  declareProperty(Prop::END_X, EMPTY_DBL(), "End value of the fitting interval");

  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      Prop::OUTPUT_WORKSPACE, "", Direction::Output),
                  "The flood correction workspace.");
}

MatrixWorkspace_sptr CreateFloodWorkspace::getInputWorkspace() {
  std::string fileName = getProperty(Prop::FILENAME);
  auto alg = createChildAlgorithm("Load");
  alg->setProperty("Filename", fileName);
  alg->setProperty("LoadMonitors", false);
  alg->setProperty("OutputWorkspace", "dummy");
  alg->execute();
  Workspace_sptr ws = alg->getProperty("OutputWorkspace");
  return boost::dynamic_pointer_cast<MatrixWorkspace>(ws);
}

MatrixWorkspace_sptr CreateFloodWorkspace::integrate(MatrixWorkspace_sptr ws) {
  auto alg = createChildAlgorithm("Integration");
  alg->setProperty("InputWorkspace", ws);
  alg->setProperty("OutputWorkspace", "dummy");
  alg->execute();
  return alg->getProperty("OutputWorkspace");
}

MatrixWorkspace_sptr CreateFloodWorkspace::transpose(MatrixWorkspace_sptr ws) {
  auto alg = createChildAlgorithm("Transpose");
  alg->setProperty("InputWorkspace", ws);
  alg->setProperty("OutputWorkspace", "dummy");
  alg->execute();
  return alg->getProperty("OutputWorkspace");
}

API::MatrixWorkspace_sptr CreateFloodWorkspace::removeBackground(API::MatrixWorkspace_sptr ws){
  double startX = getProperty(Prop::START_X);
  double endX = getProperty(Prop::END_X);

  auto fitWS = transpose(ws);
  auto const &x = fitWS->x(0);
  if (isDefault(Prop::START_X)) {
    startX = x.front();
  }
  if (isDefault(Prop::END_X)) {
    endX = x.back();
  }

  std::string const functionStr("name=LinearBackground");

  auto alg = createChildAlgorithm("Fit");
  alg->setProperty("Function", functionStr);
  alg->setProperty("InputWorkspace", fitWS);
  alg->setProperty("WorkspaceIndex", 0);
  alg->setProperty("StartX", startX);
  alg->setProperty("EndX", endX);
  alg->execute();

  IFunction_sptr fun = alg->getProperty("Function");

  alg = createChildAlgorithm("EvaluateFunction");
  alg->setProperty("Function", fun);
  alg->setProperty("InputWorkspace", fitWS);
  alg->setProperty("WorkspaceIndex", 0);
  alg->setProperty("OutputWorkspace", "dummy");
  alg->execute();

  Workspace_sptr tmpWS = alg->getProperty("OutputWorkspace");
  auto bkgWS = boost::dynamic_pointer_cast<MatrixWorkspace>(tmpWS);
  auto const &bkg = bkgWS->y(1);

  auto const nHisto = ws->getNumberHistograms();
  for(size_t i = 0; i < nHisto; ++i) {
    auto const xVal = x[i];
    if (xVal >= startX && xVal <= endX) {
      auto const background = bkg[i];
      if (background <= 0.0) {
        throw std::runtime_error("Background is expected to be positive, found value " + std::to_string(background));
      }
      ws->mutableY(i)[0] /= background;
      ws->mutableE(i)[0] /= background;
    } else {
      ws->mutableY(i)[0] = 1.0;
      ws->mutableE(i)[0] = 0.0;
    }
  }

  ws->setSharedRun(make_cow<Run>());

  return ws;
}


/** Execute the algorithm.
 */
void CreateFloodWorkspace::exec() {
  auto ws = getInputWorkspace();
  ws = integrate(ws);
  ws = removeBackground(ws);
  setProperty(Prop::OUTPUT_WORKSPACE, ws);
}

} // namespace Algorithms
} // namespace Mantid
