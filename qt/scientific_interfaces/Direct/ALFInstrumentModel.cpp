// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFInstrumentModel.h"

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Workspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/Unit.h"
#include "MantidQtWidgets/InstrumentView/InstrumentActor.h"

#include <memory>
#include <utility>

using namespace Mantid::API;

namespace {
auto &ADS = AnalysisDataService::Instance();

std::string const CURVES = "Curves";
std::string const NOT_IN_ADS = "not_stored_in_ads";

bool isALFData(MatrixWorkspace_const_sptr const &workspace) { return workspace->getInstrument()->getName() == "ALF"; }

bool isAxisDSpacing(MatrixWorkspace_const_sptr const &workspace) {
  return workspace->getAxis(0)->unit()->unitID() == "dSpacing";
}

std::optional<double> xConversionFactor(MatrixWorkspace_const_sptr const &workspace) {
  if (!workspace)
    return std::nullopt;

  if (const auto axis = workspace->getAxis(0)) {
    const auto unit = axis->unit()->unitID();
    const auto label = std::string(axis->unit()->label());
    return unit == "Phi" || label == "Out of plane angle" ? 180.0 / M_PI : 1.0;
  }
  return std::nullopt;
}

std::optional<double> getTwoTheta(Mantid::Geometry::Instrument_const_sptr instrument,
                                  Mantid::Geometry::IDetector_const_sptr detector) {
  if (!instrument || !detector) {
    return std::nullopt;
  }
  auto const sample = instrument->getSample()->getPos();
  auto const source = instrument->getSource()->getPos();

  return detector->getTwoTheta(sample, sample - source) * 180.0 / M_PI;
}

void loadEmptyInstrument(std::string const &instrumentName, std::string const &outputName) {
  auto alg = AlgorithmManager::Instance().create("LoadEmptyInstrument");
  alg->initialize();
  alg->setProperty("InstrumentName", instrumentName);
  alg->setProperty("OutputWorkspace", outputName);
  alg->execute();
}

MatrixWorkspace_sptr load(std::string const &filename) {
  auto alg = AlgorithmManager::Instance().create("Load");
  alg->initialize();
  alg->setAlwaysStoreInADS(false);
  alg->setProperty("Filename", filename);
  alg->setProperty("OutputWorkspace", NOT_IN_ADS);
  alg->execute();
  Workspace_sptr outputWorkspace = alg->getProperty("OutputWorkspace");
  return std::dynamic_pointer_cast<MatrixWorkspace>(outputWorkspace);
}

MatrixWorkspace_sptr createWorkspace(std::string const &parentName, std::vector<double> const &x,
                                     std::vector<double> const &y, std::vector<double> const &e,
                                     int const numberOfSpectra, std::string const &unitX) {
  auto alg = AlgorithmManager::Instance().create("CreateWorkspace");
  alg->initialize();
  alg->setAlwaysStoreInADS(false);
  alg->setProperty("ParentWorkspace", parentName);
  alg->setProperty("DataX", x);
  alg->setProperty("DataY", y);
  alg->setProperty("DataE", e);
  alg->setProperty("NSpec", numberOfSpectra);
  alg->setProperty("UnitX", unitX);
  alg->setPropertyValue("OutputWorkspace", NOT_IN_ADS);
  alg->execute();
  MatrixWorkspace_sptr outputWorkspace = alg->getProperty("OutputWorkspace");
  return outputWorkspace;
}

MatrixWorkspace_sptr rebunch(MatrixWorkspace_sptr const &inputWorkspace, int const nBunch) {
  auto alg = Mantid::API::AlgorithmManager::Instance().create("Rebunch");
  alg->initialize();
  alg->setAlwaysStoreInADS(false);
  alg->setProperty("InputWorkspace", inputWorkspace);
  alg->setProperty("NBunch", nBunch);
  alg->setProperty("OutputWorkspace", NOT_IN_ADS);
  alg->execute();
  MatrixWorkspace_sptr outputWorkspace = alg->getProperty("OutputWorkspace");
  return outputWorkspace;
}

MatrixWorkspace_sptr rebinToWorkspace(MatrixWorkspace_sptr const &workspaceToRebin,
                                      MatrixWorkspace_sptr const &workspaceToMatch) {
  auto alg = AlgorithmManager::Instance().create("RebinToWorkspace");
  alg->initialize();
  alg->setAlwaysStoreInADS(false);
  alg->setProperty("WorkspaceToRebin", workspaceToRebin);
  alg->setProperty("WorkspaceToMatch", workspaceToMatch);
  alg->setProperty("OutputWorkspace", NOT_IN_ADS);
  alg->execute();
  MatrixWorkspace_sptr outputWorkspace = alg->getProperty("OutputWorkspace");
  return outputWorkspace;
}

MatrixWorkspace_sptr plus(MatrixWorkspace_sptr const &lhsWorkspace, MatrixWorkspace_sptr const &rhsWorkspace) {
  auto alg = AlgorithmManager::Instance().create("Plus");
  alg->initialize();
  alg->setAlwaysStoreInADS(false);
  alg->setProperty("LHSWorkspace", lhsWorkspace);
  alg->setProperty("RHSWorkspace", rhsWorkspace);
  alg->setProperty("OutputWorkspace", NOT_IN_ADS);
  alg->execute();
  MatrixWorkspace_sptr outputWorkspace = alg->getProperty("OutputWorkspace");
  return outputWorkspace;
}

MatrixWorkspace_sptr normaliseByCurrent(MatrixWorkspace_sptr const &inputWorkspace) {
  auto alg = AlgorithmManager::Instance().create("NormaliseByCurrent");
  alg->initialize();
  alg->setAlwaysStoreInADS(false);
  alg->setProperty("InputWorkspace", inputWorkspace);
  alg->setProperty("OutputWorkspace", NOT_IN_ADS);
  alg->execute();
  MatrixWorkspace_sptr outputWorkspace = alg->getProperty("OutputWorkspace");
  return outputWorkspace;
}

MatrixWorkspace_sptr convertUnits(MatrixWorkspace_sptr const &inputWorkspace, std::string const &target) {
  auto alg = AlgorithmManager::Instance().create("ConvertUnits");
  alg->initialize();
  alg->setAlwaysStoreInADS(false);
  alg->setProperty("InputWorkspace", inputWorkspace);
  alg->setProperty("Target", target);
  alg->setProperty("OutputWorkspace", NOT_IN_ADS);
  alg->execute();
  MatrixWorkspace_sptr outputWorkspace = alg->getProperty("OutputWorkspace");
  return outputWorkspace;
}

MatrixWorkspace_sptr scaleX(MatrixWorkspace_sptr const &inputWorkspace, double const factor) {
  auto alg = AlgorithmManager::Instance().create("ScaleX");
  alg->initialize();
  alg->setAlwaysStoreInADS(false);
  alg->setProperty("InputWorkspace", inputWorkspace);
  alg->setProperty("Factor", factor);
  alg->setProperty("OutputWorkspace", NOT_IN_ADS);
  alg->execute();
  MatrixWorkspace_sptr outputWorkspace = alg->getProperty("OutputWorkspace");
  return outputWorkspace;
}

MatrixWorkspace_sptr convertToHistogram(MatrixWorkspace_sptr const &inputWorkspace) {
  auto alg = AlgorithmManager::Instance().create("ConvertToHistogram");
  alg->initialize();
  alg->setAlwaysStoreInADS(false);
  alg->setProperty("InputWorkspace", inputWorkspace);
  alg->setProperty("OutputWorkspace", NOT_IN_ADS);
  alg->execute();
  MatrixWorkspace_sptr outputWorkspace = alg->getProperty("OutputWorkspace");
  return outputWorkspace;
}

} // namespace

namespace MantidQt::CustomInterfaces {

ALFInstrumentModel::ALFInstrumentModel() { loadEmptyInstrument(instrumentName(), loadedWsName()); }

/*
 * Loads data into the ALFView. Normalises and converts to DSpacing if necessary.
 * @param filename:: The filepath to the ALFData
 * @return An optional error message
 */
std::optional<std::string> ALFInstrumentModel::loadAndTransform(std::string const &filename) {
  auto loadedWorkspace = load(filename);

  if (!isALFData(loadedWorkspace)) {
    return "The loaded data is not from the ALF instrument";
  }

  if (!isAxisDSpacing(loadedWorkspace)) {
    loadedWorkspace = convertUnits(normaliseByCurrent(loadedWorkspace), "dSpacing");
  }

  ADS.addOrReplace(loadedWsName(), loadedWorkspace);
  return std::nullopt;
}

std::string ALFInstrumentModel::extractedWsName() const {
  return "extractedTubes_" + instrumentName() + std::to_string(runNumber());
}

/*
 * Retrieves the run number from the currently loaded workspace.
 */
std::size_t ALFInstrumentModel::runNumber() const {
  auto const loadedName = loadedWsName();
  if (!ADS.doesExist(loadedName)) {
    return 0u;
  }

  if (auto workspace = ADS.retrieveWS<MatrixWorkspace>(loadedName)) {
    return static_cast<std::size_t>(workspace->getRunNumber());
  }
  return 0u;
}

void ALFInstrumentModel::setSelectedDetectors(std::vector<std::size_t> detectorIndices) {
  m_detectorIndices = std::move(detectorIndices);
}

MatrixWorkspace_sptr
ALFInstrumentModel::generateOutOfPlaneAngleWorkspace(MantidQt::MantidWidgets::InstrumentActor *actor) const {

  std::vector<double> x, y, e;
  prepareDataForIntegralsPlot(actor, m_detectorIndices, x, y, e);

  MatrixWorkspace_sptr workspace = createWorkspace(actor->getWorkspace()->getName(), x, y, e, 1, "Out of plane angle");

  workspace = scaleX(workspace, 180.0 / M_PI);
  workspace = rebunch(workspace, 2);
  workspace = convertToHistogram(workspace);
  return workspace;
}

void ALFInstrumentModel::prepareDataForIntegralsPlot(MantidQt::MantidWidgets::InstrumentActor *actor,
                                                     std::vector<std::size_t> const &detectorIndices,
                                                     std::vector<double> &x, std::vector<double> &y,
                                                     std::vector<double> &e) const {
  const auto &componentInfo = actor->componentInfo();
  const auto numberOfDetectorsPerTube =
      componentInfo.detectorsInSubtree(componentInfo.parent(detectorIndices[0])).size();
  const auto samplePos = componentInfo.samplePosition();
  auto normal = normalize(componentInfo.position(detectorIndices[1]) - componentInfo.position(detectorIndices[0]));
  const auto &detectorInfo = actor->detectorInfo();
  Mantid::API::MatrixWorkspace_const_sptr ws = actor->getWorkspace();

  // collect and sort xy pairs in xymap
  std::map<double, double> xymap, xemap;
  std::size_t imin, imax;

  for (auto i = 0u; i < detectorIndices.size(); ++i) {
    auto detIndex = detectorIndices[i];
    if (i % numberOfDetectorsPerTube == 0) {
      normal = normalize(componentInfo.position(detectorIndices[i + 1]) - componentInfo.position(detectorIndices[i]));
    }
    auto index = actor->getWorkspaceIndex(detIndex);
    actor->getBinMinMaxIndex(index, imin, imax);
    if (index != MantidQt::MantidWidgets::InstrumentActor::INVALID_INDEX && componentInfo.isDetector(detIndex)) {

      // get the x-value for detector idet
      double xvalue = getOutOfPlaneAngle(detectorInfo.position(detIndex), samplePos, normal);

      // get the y-value for detector idet
      const auto yValues = ws->readY(index);
      xymap[xvalue] = std::accumulate(yValues.cbegin() + imin, yValues.cbegin() + imax, 0);

      const auto eValues = ws->readE(index);
      // take squares of the errors
      std::vector<double> tmp(imax - imin);
      std::transform(eValues.cbegin() + imin, eValues.cbegin() + imax, eValues.cbegin() + imin, tmp.begin(),
                     std::multiplies<double>());
      // sum them and take sqrt
      xemap[xvalue] = sqrt(std::accumulate(tmp.begin(), tmp.end(), 0));
    }
  }
  x.reserve(xymap.size());
  y.reserve(xymap.size());
  e.reserve(xemap.size());
  std::transform(xymap.cbegin(), xymap.cend(), std::back_inserter(x), [](auto const &xy) { return xy.first; });
  std::transform(xymap.cbegin(), xymap.cend(), std::back_inserter(y), [](auto const &xy) { return xy.second; });
  std::transform(xemap.cbegin(), xemap.cend(), std::back_inserter(e), [](auto const &xe) { return xe.second; });
}

double ALFInstrumentModel::getOutOfPlaneAngle(const Mantid::Kernel::V3D &pos, const Mantid::Kernel::V3D &origin,
                                              const Mantid::Kernel::V3D &normal) const {
  const auto vec = normalize(pos - origin);
  return asin(vec.scalar_prod(normal));
}

/*
 * Extracts a single tube. Increments the average counter.
 */
std::optional<double> ALFInstrumentModel::extractSingleTube(Mantid::Geometry::IDetector_const_sptr detector) {
  if (auto const extractedWorkspace = retrieveSingleTube()) {
    ADS.addOrReplace(extractedWsName(), extractedWorkspace);
    return getTwoTheta(extractedWorkspace->getInstrument(), detector);
  }
  return std::nullopt;
}

MatrixWorkspace_sptr ALFInstrumentModel::retrieveSingleTube() {
  if (!ADS.doesExist(CURVES))
    return nullptr;

  // Get a handle on the curve workspace and then delete it from the ADS
  auto const curveWorkspace = ADS.retrieveWS<MatrixWorkspace>(CURVES);
  ADS.remove(CURVES);

  if (auto const scaleFactor = xConversionFactor(curveWorkspace)) {
    // Convert to degrees if the XAxis is an angle in radians, and then convert to histograms.
    return convertToHistogram(scaleX(curveWorkspace, *scaleFactor));
  }
  return nullptr;
}

std::optional<double> ALFInstrumentModel::averageTube(Mantid::Geometry::IDetector_const_sptr detector,
                                                      std::size_t const numberOfTubes) {
  // Multiply up the current average
  auto existingAverageTube = ADS.retrieveWS<MatrixWorkspace>(extractedWsName());
  existingAverageTube *= double(numberOfTubes);

  // Get the recently selected tube, and rebin to match previous extracted workspace
  auto newExtractedWorkspace = retrieveSingleTube();
  if (!newExtractedWorkspace) {
    return std::nullopt;
  }
  auto const newTwoTheta = getTwoTheta(newExtractedWorkspace->getInstrument(), detector);
  newExtractedWorkspace = rebinToWorkspace(newExtractedWorkspace, existingAverageTube);

  // Do an average
  auto averagedWorkspace = plus(newExtractedWorkspace, existingAverageTube);
  averagedWorkspace->mutableY(0) /= double(numberOfTubes + 1u);

  // Add the result back into the ADS
  ADS.addOrReplace(extractedWsName(), averagedWorkspace);

  return newTwoTheta;
}

/*
 * Returns true if the option to average a tube should be shown.
 */
bool ALFInstrumentModel::checkDataIsExtracted() const { return ADS.doesExist(extractedWsName()); }

} // namespace MantidQt::CustomInterfaces
