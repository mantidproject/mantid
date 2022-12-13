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

std::string const NOT_IN_ADS = "not_stored_in_ads";

bool isALFData(MatrixWorkspace_const_sptr const &workspace) { return workspace->getInstrument()->getName() == "ALF"; }

bool isAxisDSpacing(MatrixWorkspace_const_sptr const &workspace) {
  return workspace->getAxis(0)->unit()->unitID() == "dSpacing";
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

void appendTwoThetaClosestToZero(std::vector<double> &twoThetas,
                                 std::optional<std::pair<double, std::size_t>> &workspaceIndexClosestToZero,
                                 MatrixWorkspace_const_sptr const &workspace,
                                 Mantid::Geometry::Instrument_const_sptr instrument) {
  if (workspaceIndexClosestToZero) {
    auto const twoTheta = getTwoTheta(instrument, workspace->getDetector((*workspaceIndexClosestToZero).second));
    if (twoTheta) {
      twoThetas.emplace_back(*twoTheta);
    }
    workspaceIndexClosestToZero = std::nullopt;
  }
}

/*
 * Finds all the detector indices in a tube if at least one detector index from that tube is provided.
 * @param componentInfo : The component info object which stores info about all detectors in their tubes
 * @param partTubeDetectorIndices : The detector indices that could come from different parts of multiple tubes
 * @return A vector of detector indices for representing entire tubes being selected.
 */
std::vector<std::size_t> findWholeTubeDetectors(Mantid::Geometry::ComponentInfo const &componentInfo,
                                                std::vector<std::size_t> const &partTubeDetectorIndices) {
  std::vector<std::size_t> wholeTubeIndices;
  for (auto const &detectorIndex : partTubeDetectorIndices) {
    auto const iter = std::find(wholeTubeIndices.cbegin(), wholeTubeIndices.cend(), detectorIndex);
    // Check that the indices for this tube haven't already been added
    if (iter == wholeTubeIndices.cend()) {
      // Find all of the detector indices for the whole tube
      auto const detectors = componentInfo.detectorsInSubtree(componentInfo.parent(detectorIndex));
      std::transform(detectors.cbegin(), detectors.cend(), std::back_inserter(wholeTubeIndices),
                     [](auto const &index) { return index; });
    }
  }
  return wholeTubeIndices;
}

double calculateError(Mantid::HistogramData::HistogramE const &eValues, std::size_t const binIndexMin,
                      std::size_t const binIndexMax) {
  // Calculate the squared values
  std::vector<double> squaredValues;
  squaredValues.reserve(binIndexMax - binIndexMin);
  std::transform(eValues.cbegin() + binIndexMin, eValues.cbegin() + binIndexMax, eValues.cbegin() + binIndexMin,
                 squaredValues.begin(), std::multiplies<double>());
  // Sum them and then take the sqrt
  return sqrt(std::accumulate(squaredValues.begin(), squaredValues.end(), 0.0));
}

double calculateYCounts(Mantid::HistogramData::HistogramY const &yValues, std::size_t const binIndexMin,
                        std::size_t const binIndexMax) {
  return std::accumulate(yValues.cbegin() + binIndexMin, yValues.cbegin() + binIndexMax, 0.0);
}

double calculateOutOfPlaneAngle(Mantid::Kernel::V3D const &pos, Mantid::Kernel::V3D const &origin,
                                Mantid::Kernel::V3D const &normal) {
  auto const vec = normalize(pos - origin);
  return asin(vec.scalar_prod(normal));
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
                                     std::size_t const numberOfSpectra, std::string const &unitX) {
  auto alg = AlgorithmManager::Instance().create("CreateWorkspace");
  alg->initialize();
  alg->setAlwaysStoreInADS(false);
  alg->setProperty("ParentWorkspace", parentName);
  alg->setProperty("DataX", x);
  alg->setProperty("DataY", y);
  alg->setProperty("DataE", e);
  alg->setProperty("NSpec", static_cast<int>(numberOfSpectra));
  alg->setProperty("UnitX", unitX);
  alg->setPropertyValue("OutputWorkspace", NOT_IN_ADS);
  alg->execute();
  MatrixWorkspace_sptr outputWorkspace = alg->getProperty("OutputWorkspace");
  return outputWorkspace;
}

MatrixWorkspace_sptr rebunch(MatrixWorkspace_sptr const &inputWorkspace, std::size_t const nBunch) {
  auto alg = Mantid::API::AlgorithmManager::Instance().create("Rebunch");
  alg->initialize();
  alg->setAlwaysStoreInADS(false);
  alg->setProperty("InputWorkspace", inputWorkspace);
  alg->setProperty("NBunch", static_cast<int>(nBunch));
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

} // namespace

namespace MantidQt::CustomInterfaces {

ALFInstrumentModel::ALFInstrumentModel() : m_detectorIndices() { loadEmptyInstrument("ALF", loadedWsName()); }

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

void ALFInstrumentModel::setSelectedDetectors(Mantid::Geometry::ComponentInfo const &componentInfo,
                                              std::vector<std::size_t> const &detectorIndices) {
  m_detectorIndices = findWholeTubeDetectors(componentInfo, detectorIndices);
}

std::tuple<MatrixWorkspace_sptr, std::vector<double>>
ALFInstrumentModel::generateOutOfPlaneAngleWorkspace(MantidQt::MantidWidgets::IInstrumentActor const &actor) const {
  std::vector<double> twoThetas;
  if (m_detectorIndices.empty()) {
    return {nullptr, twoThetas};
  }

  std::vector<double> x, y, e;
  collectXAndYData(actor, x, y, e, twoThetas);

  // Create workspace containing the out of plane angle data in one spectrum
  auto workspace = createWorkspace(actor.getWorkspace()->getName(), x, y, e, 1u, "Out of plane angle");
  // Convert x axis from radians to degrees
  workspace = scaleX(workspace, 180.0 / M_PI);
  // Rebin and average the workspace based on the number of tubes that are selected
  workspace = rebunch(workspace, numberOfTubes(actor));
  return {workspace, twoThetas};
}

void ALFInstrumentModel::collectXAndYData(MantidQt::MantidWidgets::IInstrumentActor const &actor,
                                          std::vector<double> &x, std::vector<double> &y, std::vector<double> &e,
                                          std::vector<double> &twoThetas) const {
  auto const &componentInfo = actor.componentInfo();
  auto const &detectorInfo = actor.detectorInfo();

  Mantid::API::MatrixWorkspace_const_sptr workspace = actor.getWorkspace();

  // Collect and sort Y and E values by X
  std::map<double, double> xymap, xemap;
  collectAndSortYByX(xymap, xemap, twoThetas, actor, workspace, componentInfo, detectorInfo);

  x.reserve(xymap.size());
  y.reserve(xymap.size());
  e.reserve(xemap.size());
  std::transform(xymap.cbegin(), xymap.cend(), std::back_inserter(x), [](auto const &xy) { return xy.first; });
  std::transform(xymap.cbegin(), xymap.cend(), std::back_inserter(y), [](auto const &xy) { return xy.second; });
  std::transform(xemap.cbegin(), xemap.cend(), std::back_inserter(e), [](auto const &xe) { return xe.second; });
}

void ALFInstrumentModel::collectAndSortYByX(std::map<double, double> &xy, std::map<double, double> &xe,
                                            std::vector<double> &twoThetas,
                                            MantidQt::MantidWidgets::IInstrumentActor const &actor,
                                            MatrixWorkspace_const_sptr const &workspace,
                                            Mantid::Geometry::ComponentInfo const &componentInfo,
                                            Mantid::Geometry::DetectorInfo const &detectorInfo) const {
  auto const nDetectorsPerTube = numberOfDetectorsPerTube(componentInfo);
  auto const samplePosition = componentInfo.samplePosition();
  auto const instrument = actor.getInstrument();

  Mantid::Kernel::V3D normal;
  std::size_t imin, imax;
  std::optional<std::pair<double, std::size_t>> workspaceIndexClosestToZeroX = std::nullopt;
  for (auto i = 0u; i < m_detectorIndices.size(); ++i) {
    auto const detectorIndex = m_detectorIndices[i];
    auto const workspaceIndex = actor.getWorkspaceIndex(detectorIndex);

    if (i % nDetectorsPerTube == 0) {
      normal = normalize(componentInfo.position(m_detectorIndices[i + 1]) - componentInfo.position(detectorIndex));
      actor.getBinMinMaxIndex(workspaceIndex, imin, imax);
      appendTwoThetaClosestToZero(twoThetas, workspaceIndexClosestToZeroX, workspace, instrument);
    }

    if (workspaceIndex != MantidQt::MantidWidgets::InstrumentActor::INVALID_INDEX &&
        componentInfo.isDetector(detectorIndex)) {

      auto const xValue = calculateOutOfPlaneAngle(detectorInfo.position(detectorIndex), samplePosition, normal);
      xy[xValue] = calculateYCounts(workspace->y(workspaceIndex), imin, imax);
      xe[xValue] = calculateError(workspace->e(workspaceIndex), imin, imax);

      auto const absXValue = std::abs(xValue);
      if (!workspaceIndexClosestToZeroX || absXValue < (*workspaceIndexClosestToZeroX).first) {
        workspaceIndexClosestToZeroX = std::make_pair(absXValue, workspaceIndex);
      }
    }
  }

  appendTwoThetaClosestToZero(twoThetas, workspaceIndexClosestToZeroX, workspace, instrument);
}

std::size_t ALFInstrumentModel::numberOfDetectorsPerTube(Mantid::Geometry::ComponentInfo const &componentInfo) const {
  return componentInfo.detectorsInSubtree(componentInfo.parent(m_detectorIndices[0])).size();
}

std::size_t ALFInstrumentModel::numberOfTubes(MantidQt::MantidWidgets::IInstrumentActor const &actor) const {
  auto const nDetectorsPerTube = numberOfDetectorsPerTube(actor.componentInfo());
  if (nDetectorsPerTube == 0u) {
    return 0u;
  }
  return static_cast<std::size_t>(m_detectorIndices.size() / nDetectorsPerTube);
}

} // namespace MantidQt::CustomInterfaces
