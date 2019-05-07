// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CreateFloodWorkspace.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MultipleFileProperty.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/ListValidator.h"

#include <limits>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace {
namespace Prop {
std::string const FILENAME("Filename");
std::string const OUTPUT_WORKSPACE("OutputWorkspace");
std::string const START_X("StartSpectrum");
std::string const END_X("EndSpectrum");
std::string const EXCLUDE("ExcludeSpectra");
std::string const BACKGROUND("Background");
std::string const CENTRAL_PIXEL("CentralPixelSpectrum");
std::string const RANGE_LOWER("RangeLower");
std::string const RANGE_UPPER("RangeUpper");
} // namespace Prop

// Too large number makes mantid crash when trying to open a plot
double const VERY_BIG_VALUE = 1.0e200;
std::map<std::string, std::string> const funMap{
    {"Linear", "name=LinearBackground"}, {"Quadratic", "name=Quadratic"}};

} // namespace

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CreateFloodWorkspace)

const std::string CreateFloodWorkspace::name() const {
  return "CreateFloodWorkspace";
}

const std::string CreateFloodWorkspace::summary() const {
  return "Algorithm to create a flood correction workspace for reflectometry "
         "data reduction.";
}

int CreateFloodWorkspace::version() const { return 1; }

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
                  "The name of the flood run file(s) to read. Multiple runs "
                  "can be loaded and added together, e.g. INST10+11+12+13.ext");

  declareProperty(Prop::START_X, EMPTY_DBL(),
                  "Start value of the fitting interval");
  declareProperty(Prop::END_X, EMPTY_DBL(),
                  "End value of the fitting interval");

  declareProperty(Kernel::make_unique<ArrayProperty<double>>(Prop::EXCLUDE),
                  "Spectra to exclude");

  declareProperty(Prop::RANGE_LOWER, EMPTY_DBL(),
                  "The lower integration limit (an X value).");

  declareProperty(Prop::RANGE_UPPER, EMPTY_DBL(),
                  "The upper integration limit (an X value).");

  declareProperty(Prop::CENTRAL_PIXEL, EMPTY_INT(),
                  "A spectrum number of the central pixel.");

  std::vector<std::string> allowedValues;
  allowedValues.reserve(funMap.size());
  for (const auto &i : funMap)
    allowedValues.emplace_back(i.first);
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
  auto alg = createChildAlgorithm("Load", 0, 0.8);
  alg->setProperty("Filename", fileName);
  if (alg->existsProperty("LoadMonitors")) {
    alg->setProperty("LoadMonitors", false);
  }
  alg->setProperty("OutputWorkspace", "dummy");
  alg->execute();
  Workspace_sptr ws = alg->getProperty("OutputWorkspace");
  auto input = boost::dynamic_pointer_cast<MatrixWorkspace>(ws);
  if (!input) {
    throw std::invalid_argument(
        "Loaded files do not produce a single MatrixWorkspace as expected.");
  }
  return input;
}

std::string CreateFloodWorkspace::getBackgroundFunction() {
  return funMap.at(getPropertyValue(Prop::BACKGROUND));
}

MatrixWorkspace_sptr CreateFloodWorkspace::integrate(MatrixWorkspace_sptr ws) {
  auto alg = createChildAlgorithm("Integration");
  alg->setProperty("InputWorkspace", ws);
  alg->setProperty("OutputWorkspace", "dummy");
  if (!isDefault(Prop::RANGE_LOWER)) {
    alg->setProperty("RangeLower",
                     static_cast<double>(getProperty(Prop::RANGE_LOWER)));
  }
  if (!isDefault(Prop::RANGE_UPPER)) {
    alg->setProperty("RangeUpper",
                     static_cast<double>(getProperty(Prop::RANGE_UPPER)));
  }
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

bool CreateFloodWorkspace::shouldRemoveBackground() {
  return isDefault(Prop::CENTRAL_PIXEL);
}

void CreateFloodWorkspace::collectExcludedSpectra() {
  m_excludedSpectra = getProperty(Prop::EXCLUDE);
}

bool CreateFloodWorkspace::isExcludedSpectrum(double spec) const {
  return std::find(m_excludedSpectra.begin(), m_excludedSpectra.end(), spec) !=
         m_excludedSpectra.end();
}

API::MatrixWorkspace_sptr
CreateFloodWorkspace::removeBackground(API::MatrixWorkspace_sptr ws) {
  g_log.information() << "Remove background "
                      << getPropertyValue(Prop::BACKGROUND) << '\n';
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
  for (auto i : m_excludedSpectra) {
    excludeFromFit.push_back(i);
    excludeFromFit.push_back(i);
  }

  std::string const function = getBackgroundFunction();

  // Fit the data to determine unwanted background
  auto alg = createChildAlgorithm("Fit", 0.9, 0.99);
  alg->setProperty("Function", function);
  alg->setProperty("InputWorkspace", fitWS);
  alg->setProperty("WorkspaceIndex", 0);
  if (!excludeFromFit.empty()) {
    alg->setProperty("Exclude", excludeFromFit);
  }
  alg->setProperty("Output", "fit");
  alg->execute();

  IFunction_sptr func = alg->getProperty("Function");
  g_log.information() << "Background function parameters:\n";
  for (size_t i = 0; i < func->nParams(); ++i) {
    g_log.information() << "    " << func->parameterName(i) << ": "
                        << func->getParameter(i) << '\n';
  }

  // Divide the workspace by the fitted curve to remove the background
  // and scale to values around 1
  MatrixWorkspace_sptr bkgWS = alg->getProperty("OutputWorkspace");
  auto const &bkg = bkgWS->y(1);
  auto const nHisto = static_cast<int>(ws->getNumberHistograms());
  PARALLEL_FOR_IF(Kernel::threadSafe(*ws, *bkgWS))
  for (int i = 0; i < nHisto; ++i) {
    PARALLEL_START_INTERUPT_REGION
    auto const xVal = x[i];
    if (isExcludedSpectrum(xVal)) {
      ws->mutableY(i)[0] = VERY_BIG_VALUE;
      ws->mutableE(i)[0] = 0.0;
    } else if (xVal >= startX && xVal <= endX) {
      auto const background = bkg[i];
      if (background <= 0.0) {
        throw std::runtime_error(
            "Background is expected to be positive, found value " +
            std::to_string(background) + " at spectrum with workspace index " +
            std::to_string(i));
      }
      ws->mutableY(i)[0] /= background;
      ws->mutableE(i)[0] /= background;
    } else {
      ws->mutableY(i)[0] = 1.0;
      ws->mutableE(i)[0] = 0.0;
    }
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // Remove the logs
  ws->setSharedRun(make_cow<Run>());

  return ws;
}

MatrixWorkspace_sptr
CreateFloodWorkspace::scaleToCentralPixel(MatrixWorkspace_sptr ws) {
  int const centralSpectrum = getProperty(Prop::CENTRAL_PIXEL);
  auto const nHisto = static_cast<int>(ws->getNumberHistograms());
  if (centralSpectrum >= nHisto) {
    throw std::invalid_argument(
        "Spectrum index " + std::to_string(centralSpectrum) +
        " passed to property " + Prop::CENTRAL_PIXEL +
        " is outside the range 0-" + std::to_string(nHisto - 1));
  }
  auto const spectraMap = ws->getSpectrumToWorkspaceIndexMap();
  auto const centralIndex = spectraMap.at(centralSpectrum);
  auto const scaleFactor = ws->y(centralIndex).front();
  g_log.information() << "Scale to central pixel, factor = " << scaleFactor
                      << '\n';
  if (scaleFactor <= 0.0) {
    throw std::runtime_error("Scale factor muhst be > 0, found " +
                             std::to_string(scaleFactor));
  }
  auto const axis = ws->getAxis(1);
  auto const sa = dynamic_cast<const SpectraAxis *>(axis);
  double const startX =
      isDefault(Prop::START_X) ? sa->getMin() : getProperty(Prop::START_X);
  double const endX =
      isDefault(Prop::END_X) ? sa->getMax() : getProperty(Prop::END_X);
  PARALLEL_FOR_IF(Kernel::threadSafe(*ws))
  for (int i = 0; i < nHisto; ++i) {
    PARALLEL_START_INTERUPT_REGION
    auto const spec = ws->getSpectrum(i).getSpectrumNo();
    if (isExcludedSpectrum(spec)) {
      ws->mutableY(i)[0] = VERY_BIG_VALUE;
      ws->mutableE(i)[0] = 0.0;
    } else if (spec >= startX && spec <= endX) {
      ws->mutableY(i)[0] /= scaleFactor;
      ws->mutableE(i)[0] /= scaleFactor;
    } else {
      ws->mutableY(i)[0] = 1.0;
      ws->mutableE(i)[0] = 0.0;
    }
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  return ws;
}

/** Execute the algorithm.
 */
void CreateFloodWorkspace::exec() {
  progress(0.0);
  auto ws = getInputWorkspace();
  ws = integrate(ws);
  progress(0.9);
  collectExcludedSpectra();
  if (shouldRemoveBackground()) {
    ws = removeBackground(ws);
  } else {
    ws = scaleToCentralPixel(ws);
  }
  progress(1.0);
  setProperty(Prop::OUTPUT_WORKSPACE, ws);
}

} // namespace Algorithms
} // namespace Mantid
