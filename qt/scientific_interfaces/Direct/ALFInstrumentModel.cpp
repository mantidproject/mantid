// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFInstrumentModel.h"

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmProperties.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
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

std::string const D_SPACING_UNIT = "dSpacing";
std::string const INSTRUMENT_NAME = "ALF";
std::string const NOT_IN_ADS = "not_stored_in_ads";

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

MatrixWorkspace_sptr loadEmptyInstrument() {
  auto alg = AlgorithmManager::Instance().create("LoadEmptyInstrument");
  alg->initialize();
  alg->setAlwaysStoreInADS(false);
  alg->setProperty("InstrumentName", INSTRUMENT_NAME);
  alg->setProperty("OutputWorkspace", NOT_IN_ADS);
  alg->execute();
  MatrixWorkspace_sptr outputWorkspace = alg->getProperty("OutputWorkspace");
  return outputWorkspace;
}

} // namespace

namespace MantidQt::CustomInterfaces {

ALFInstrumentModel::ALFInstrumentModel()
    : m_emptyInstrument(loadEmptyInstrument()), m_sample(), m_vanadium(), m_tubes(), m_twoThetasClosestToZero() {
  ADS.addOrReplace(loadedWsName(), m_emptyInstrument->clone());
}

void ALFInstrumentModel::setData(ALFData const &dataType, MatrixWorkspace_sptr const &workspace) {
  switch (dataType) {
  case ALFData::SAMPLE:
    setSample(workspace);
    break;
  case ALFData::VANADIUM:
    setVanadium(workspace);
    break;
  default:
    throw std::invalid_argument("ALFData must be one of { SAMPLE, VANADIUM }");
  }
}

void ALFInstrumentModel::setSample(MatrixWorkspace_sptr const &sample) {
  m_twoThetasClosestToZero.clear();

  auto const sampleRemoved = m_sample && !sample;
  m_sample = sample;
  if (sampleRemoved) {
    ADS.addOrReplace(loadedWsName(), m_emptyInstrument->clone());
  }
}

void ALFInstrumentModel::setVanadium(MatrixWorkspace_sptr const &vanadium) {
  m_twoThetasClosestToZero.clear();
  m_vanadium = vanadium;
}

bool ALFInstrumentModel::hasData(ALFData const &dataType) const { return bool(data(dataType)); }

Mantid::API::MatrixWorkspace_sptr ALFInstrumentModel::data(ALFData const &dataType) const {
  switch (dataType) {
  case ALFData::SAMPLE:
    return m_sample;
  case ALFData::VANADIUM:
    return m_vanadium;
  }
  throw std::invalid_argument("ALFData must be one of { SAMPLE, VANADIUM }");
}

void ALFInstrumentModel::replaceSampleWorkspaceInADS(Mantid::API::MatrixWorkspace_sptr const &workspace) const {
  ADS.addOrReplace(loadedWsName(), workspace);
}

std::size_t ALFInstrumentModel::run(ALFData const &dataType) const {
  switch (dataType) {
  case ALFData::SAMPLE:
    return runNumber(m_sample);
  case ALFData::VANADIUM:
    return runNumber(m_vanadium);
  }
  throw std::invalid_argument("ALFData must be one of { SAMPLE, VANADIUM }");
}

std::size_t ALFInstrumentModel::runNumber(Mantid::API::MatrixWorkspace_sptr const &workspace) const {
  if (!workspace) {
    return 0u;
  }
  return static_cast<std::size_t>(workspace->getRunNumber());
}

bool ALFInstrumentModel::isALFData(MatrixWorkspace_const_sptr const &workspace) const {
  return workspace->getInstrument()->getName() == INSTRUMENT_NAME;
}

bool ALFInstrumentModel::binningMismatch() const {
  return m_vanadium && !WorkspaceHelpers::matchingBins(m_sample, m_vanadium);
}

bool ALFInstrumentModel::axisIsDSpacing() const { return m_sample->getAxis(0)->unit()->unitID() == D_SPACING_UNIT; }

bool ALFInstrumentModel::setSelectedTubes(std::vector<DetectorTube> tubes) {
  // If the number of tubes is different then we definitely need to update the stored tubes
  if (tubes.size() != m_tubes.size()) {
    m_tubes = std::move(tubes);
    return true;
  }

  // Check if a new tube exists in the provided tubes
  const auto hasNewTube =
      std::any_of(tubes.cbegin(), tubes.cend(), [&](const auto &tube) { return !tubeExists(tube); });
  if (hasNewTube) {
    m_tubes = std::move(tubes);
  }
  return hasNewTube;
}

bool ALFInstrumentModel::addSelectedTube(DetectorTube const &tube) {
  auto const isNewTube = !tubeExists(tube);
  if (isNewTube) {
    m_tubes.emplace_back(tube);
  }

  return isNewTube;
}

bool ALFInstrumentModel::hasSelectedTubes() const { return !m_tubes.empty(); }

bool ALFInstrumentModel::tubeExists(DetectorTube const &tube) const {
  return std::find(m_tubes.cbegin(), m_tubes.cend(), tube) != m_tubes.cend();
}

std::unique_ptr<Mantid::API::AlgorithmRuntimeProps>
ALFInstrumentModel::loadProperties(std::string const &filename) const {
  auto properties = std::make_unique<AlgorithmRuntimeProps>();
  AlgorithmProperties::update("Filename", filename, *properties);
  AlgorithmProperties::update("OutputWorkspace", NOT_IN_ADS, *properties);
  return properties;
}

std::unique_ptr<Mantid::API::AlgorithmRuntimeProps>
ALFInstrumentModel::normaliseByCurrentProperties(Mantid::API::MatrixWorkspace_sptr const &inputWorkspace) const {
  auto properties = std::make_unique<AlgorithmRuntimeProps>();
  AlgorithmProperties::update("InputWorkspace", inputWorkspace, *properties);
  AlgorithmProperties::update("OutputWorkspace", NOT_IN_ADS, *properties);
  return properties;
}

std::unique_ptr<AlgorithmRuntimeProps> ALFInstrumentModel::rebinToWorkspaceProperties() const {
  auto properties = std::make_unique<AlgorithmRuntimeProps>();
  AlgorithmProperties::update("WorkspaceToRebin", m_vanadium, *properties);
  AlgorithmProperties::update("WorkspaceToMatch", m_sample, *properties);
  AlgorithmProperties::update("OutputWorkspace", NOT_IN_ADS, *properties);
  return properties;
}

std::unique_ptr<AlgorithmRuntimeProps> ALFInstrumentModel::divideProperties() const {
  auto properties = std::make_unique<AlgorithmRuntimeProps>();
  AlgorithmProperties::update("LHSWorkspace", m_sample, *properties);
  AlgorithmProperties::update("RHSWorkspace", m_vanadium, *properties);
  AlgorithmProperties::update("AllowDifferentNumberSpectra", true, *properties);
  AlgorithmProperties::update("OutputWorkspace", NOT_IN_ADS, *properties);
  return properties;
}

std::unique_ptr<AlgorithmRuntimeProps>
ALFInstrumentModel::replaceSpecialValuesProperties(Mantid::API::MatrixWorkspace_sptr const &inputWorkspace) const {
  auto properties = std::make_unique<AlgorithmRuntimeProps>();
  AlgorithmProperties::update("InputWorkspace", inputWorkspace, *properties);
  AlgorithmProperties::update("InfinityValue", 0.0, *properties);
  AlgorithmProperties::update("NaNValue", 1.0, *properties);
  AlgorithmProperties::update("CheckErrorAxis", true, *properties);
  AlgorithmProperties::update("OutputWorkspace", NOT_IN_ADS, *properties);
  return properties;
}

std::unique_ptr<AlgorithmRuntimeProps>
ALFInstrumentModel::convertUnitsProperties(MatrixWorkspace_sptr const &inputWorkspace) const {
  auto properties = std::make_unique<AlgorithmRuntimeProps>();
  AlgorithmProperties::update("InputWorkspace", inputWorkspace, *properties);
  AlgorithmProperties::update("Target", D_SPACING_UNIT, *properties);
  AlgorithmProperties::update("OutputWorkspace", NOT_IN_ADS, *properties);
  return properties;
}

std::unique_ptr<AlgorithmRuntimeProps>
ALFInstrumentModel::createWorkspaceAlgorithmProperties(MantidQt::MantidWidgets::IInstrumentActor const &actor) {
  std::vector<double> x, y, e;
  collectXAndYData(actor, x, y, e);

  auto properties = std::make_unique<AlgorithmRuntimeProps>();
  AlgorithmProperties::update("ParentWorkspace", actor.getWorkspace()->getName(), *properties);
  AlgorithmProperties::update("DataX", x, *properties, false);
  AlgorithmProperties::update("DataY", y, *properties, false);
  AlgorithmProperties::update("DataE", e, *properties, false);
  AlgorithmProperties::update("NSpec", 1, *properties);
  AlgorithmProperties::update("UnitX", std::string("Out of plane angle"), *properties);
  AlgorithmProperties::update("OutputWorkspace", NOT_IN_ADS, *properties);
  return properties;
}

std::unique_ptr<AlgorithmRuntimeProps>
ALFInstrumentModel::scaleXProperties(MatrixWorkspace_sptr const &inputWorkspace) const {
  auto properties = std::make_unique<AlgorithmRuntimeProps>();
  AlgorithmProperties::update("InputWorkspace", inputWorkspace, *properties);
  AlgorithmProperties::update("Factor", 180.0 / M_PI, *properties);
  AlgorithmProperties::update("OutputWorkspace", NOT_IN_ADS, *properties);
  return properties;
}

std::unique_ptr<AlgorithmRuntimeProps>
ALFInstrumentModel::rebunchProperties(MatrixWorkspace_sptr const &inputWorkspace) const {
  auto properties = std::make_unique<AlgorithmRuntimeProps>();
  AlgorithmProperties::update("InputWorkspace", inputWorkspace, *properties);
  AlgorithmProperties::update("NBunch", static_cast<int>(numberOfTubes()), *properties);
  AlgorithmProperties::update("OutputWorkspace", NOT_IN_ADS, *properties);
  return properties;
}

void ALFInstrumentModel::collectXAndYData(MantidQt::MantidWidgets::IInstrumentActor const &actor,
                                          std::vector<double> &x, std::vector<double> &y, std::vector<double> &e) {
  auto const &componentInfo = actor.componentInfo();
  auto const &detectorInfo = actor.detectorInfo();

  Mantid::API::MatrixWorkspace_const_sptr workspace = actor.getWorkspace();

  // Collect and sort Y and E values by X
  std::map<double, double> xymap, xemap;
  collectAndSortYByX(xymap, xemap, actor, workspace, componentInfo, detectorInfo);

  x.reserve(xymap.size());
  y.reserve(xymap.size());
  e.reserve(xemap.size());
  std::transform(xymap.cbegin(), xymap.cend(), std::back_inserter(x), [](auto const &xy) { return xy.first; });
  std::transform(xymap.cbegin(), xymap.cend(), std::back_inserter(y), [](auto const &xy) { return xy.second; });
  std::transform(xemap.cbegin(), xemap.cend(), std::back_inserter(e), [](auto const &xe) { return xe.second; });
}

void ALFInstrumentModel::collectAndSortYByX(std::map<double, double> &xy, std::map<double, double> &xe,
                                            MantidQt::MantidWidgets::IInstrumentActor const &actor,
                                            MatrixWorkspace_const_sptr const &workspace,
                                            Mantid::Geometry::ComponentInfo const &componentInfo,
                                            Mantid::Geometry::DetectorInfo const &detectorInfo) {
  m_twoThetasClosestToZero.clear();

  auto const nDetectorsPerTube = m_tubes.front().size();
  auto const samplePosition = componentInfo.samplePosition();
  auto const instrument = actor.getInstrument();

  Mantid::Kernel::V3D normal;
  std::size_t imin, imax;
  std::optional<std::pair<double, std::size_t>> workspaceIndexClosestToZeroX = std::nullopt;
  for (auto const &tubeDetectorIndices : m_tubes) {
    for (auto i = 0u; i < tubeDetectorIndices.size(); ++i) {
      auto const detectorIndex = tubeDetectorIndices[i];
      auto const workspaceIndex = actor.getWorkspaceIndex(detectorIndex);

      if (i % nDetectorsPerTube == 0) {
        normal = normalize(componentInfo.position(tubeDetectorIndices[i + 1]) - componentInfo.position(detectorIndex));
        actor.getBinMinMaxIndex(workspaceIndex, imin, imax);
        appendTwoThetaClosestToZero(m_twoThetasClosestToZero, workspaceIndexClosestToZeroX, workspace, instrument);
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
  }
  appendTwoThetaClosestToZero(m_twoThetasClosestToZero, workspaceIndexClosestToZeroX, workspace, instrument);
}

} // namespace MantidQt::CustomInterfaces
