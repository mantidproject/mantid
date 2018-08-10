#include "MantidAlgorithms/CreateFloodWorkspace.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MultipleFileProperty.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/ListValidator.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace {
namespace Prop {
std::string const FILENAME("Filename");
std::string const OUTPUT_WORKSPACE("OutputWorkspace");
std::string const START_X("StartSpectrumIndex");
std::string const END_X("EndSpectrumIndex");
std::string const EXCLUDE("Exclude");
std::string const BACKGROUND("Background");
} // namespace Prop

double const VERY_BIG_VALUE = 10000.0;
std::map<std::string, std::string> const funMap{
    {"Linear", "name=LinearBackground"}, {"Quadratic", "name=Quadratic"}};

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

  declareProperty(make_unique<MultipleFileProperty>(Prop::FILENAME, exts),
                  "The name of the fllod run file(s) to read. Multiple runs "
                  "can be loaded and added together, e.g. INST10+11+12+13.ext");

  declareProperty(Prop::START_X, EMPTY_DBL(),
                  "Start value of the fitting interval");
  declareProperty(Prop::END_X, EMPTY_DBL(),
                  "End value of the fitting interval");

  declareProperty(Kernel::make_unique<ArrayProperty<double>>(Prop::EXCLUDE),
                  "Spectra to exclude");

  std::vector<std::string> allowedValues;
  for (auto i : funMap)
    allowedValues.push_back(i.first);
  auto backgroundValidator =
      boost::make_shared<ListValidator<std::string>>(allowedValues);
  declareProperty(Prop::BACKGROUND, "Linear", backgroundValidator,
                  "Background function.");

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

std::string CreateFloodWorkspace::getBackgroundFunction() {
  return funMap.at(getPropertyValue(Prop::BACKGROUND));
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

API::MatrixWorkspace_sptr
CreateFloodWorkspace::removeBackground(API::MatrixWorkspace_sptr ws) {
  auto fitWS = transpose(ws);
  auto const &x = fitWS->x(0);

  // Define the fitting interval
  double startX = getProperty(Prop::START_X);
  double endX = getProperty(Prop::END_X);
  std::vector<double> excludeFromFit;
  if (isDefault(Prop::START_X)) {
    startX = x.front();
  } else {
    excludeFromFit.push_back(x.front());
    excludeFromFit.push_back(startX);
  }
  if (isDefault(Prop::END_X)) {
    endX = x.back();
  } else {
    excludeFromFit.push_back(endX);
    excludeFromFit.push_back(x.back());
  }

  // Exclude any bad detectors.
  std::vector<double> const exclude = getProperty(Prop::EXCLUDE);
  for (auto i : exclude) {
    excludeFromFit.push_back(i);
    excludeFromFit.push_back(i);
  }
  auto isExcluded = [&exclude](double xVal) {
    return std::find(exclude.begin(), exclude.end(), xVal) != exclude.end();
  };

  std::string const function = getBackgroundFunction();

  // Fit the data to determine unwanted background
  auto alg = createChildAlgorithm("Fit");
  alg->setProperty("Function", function);
  alg->setProperty("InputWorkspace", fitWS);
  alg->setProperty("WorkspaceIndex", 0);
  if (!excludeFromFit.empty()) {
    alg->setProperty("Exclude", excludeFromFit);
  }
  alg->setProperty("Output", "fit");
  alg->execute();

  // Divide the workspace by the fitted curve to remove the background
  // and scale to values around 1
  MatrixWorkspace_sptr bkgWS = alg->getProperty("OutputWorkspace");
  auto const &bkg = bkgWS->y(1);
  auto const nHisto = ws->getNumberHistograms();
  for (size_t i = 0; i < nHisto; ++i) {
    auto const xVal = x[i];
    if (isExcluded(xVal)) {
      ws->mutableY(i)[0] = VERY_BIG_VALUE;
      ws->mutableE(i)[0] = 0.0;
    } else if (xVal >= startX && xVal <= endX) {
      auto const background = bkg[i];
      if (background <= 0.0) {
        throw std::runtime_error(
            "Background is expected to be positive, found value " +
            std::to_string(background));
      }
      ws->mutableY(i)[0] /= background;
      ws->mutableE(i)[0] /= background;
    } else {
      ws->mutableY(i)[0] = 1.0;
      ws->mutableE(i)[0] = 0.0;
    }
  }

  // Remove the logs
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
