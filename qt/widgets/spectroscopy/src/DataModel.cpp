// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <string>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Spectroscopy/DataModel.h"
#include "MantidQtWidgets/Spectroscopy/FitData.h"

using namespace MantidQt::MantidWidgets;

namespace {

using namespace MantidQt::CustomInterfaces::Inelastic;

std::string getFitDataName(const std::string &baseWorkspaceName, const FunctionModelSpectra &workspaceIndexes) {
  return baseWorkspaceName + " (" + workspaceIndexes.getString() + ")";
}

bool equivalentWorkspaces(const Mantid::API::MatrixWorkspace_const_sptr &lhs,
                          const Mantid::API::MatrixWorkspace_const_sptr &rhs) {
  if (!lhs || !rhs)
    return false;
  else if (lhs->getName() == "" && rhs->getName() == "")
    return lhs == rhs;
  return lhs->getName() == rhs->getName();
}

} // namespace

namespace MantidQt::CustomInterfaces::Inelastic {

DataModel::DataModel()
    : m_adsInstance(Mantid::API::AnalysisDataService::Instance()),
      m_fittingData(std::make_unique<std::vector<FitData>>()), m_uniqueWsNames(std::set<std::string>()),
      m_uniqueResWsNames(std::set<std::string>()) {}

std::vector<FitData> *DataModel::getFittingData() { return m_fittingData.get(); }

bool DataModel::hasWorkspace(std::string const &workspaceName) const {
  auto const names = getWorkspaceNames();
  auto const iter = std::find(names.cbegin(), names.cend(), workspaceName);
  return iter != names.cend();
}

WorkspaceID DataModel::getWorkspaceID(const std::string &workspaceName) const {
  auto const names = getWorkspaceNames();
  auto const iter = std::find(names.cbegin(), names.cend(), workspaceName);

  return {static_cast<size_t>(iter - names.cbegin())};
}

void DataModel::clearModel() { m_fittingData->clear(); }

Mantid::API::MatrixWorkspace_sptr DataModel::getWorkspace(WorkspaceID workspaceID) const {
  if (workspaceID < m_fittingData->size())
    return m_fittingData->at(workspaceID.value).workspace();
  return nullptr;
}

FunctionModelSpectra DataModel::getSpectra(WorkspaceID workspaceID) const {
  if (workspaceID < m_fittingData->size())
    return m_fittingData->at(workspaceID.value).spectra();
  return FunctionModelSpectra("");
}

FunctionModelDataset DataModel::getDataset(WorkspaceID workspaceID) const {
  auto const &name = getWorkspace(workspaceID)->getName();
  return FunctionModelDataset(name, getSpectra(workspaceID));
}

std::string DataModel::createDisplayName(WorkspaceID workspaceID) const {
  if (getNumberOfWorkspaces() > workspaceID)
    return getFitDataName(getWorkspaceNames()[workspaceID.value], getSpectra(workspaceID));
  else
    throw std::runtime_error("Cannot create a display name for a workspace:"
                             "the workspace index provided is too large.");
}

WorkspaceID DataModel::getNumberOfWorkspaces() const { return WorkspaceID{m_fittingData->size()}; }

size_t DataModel::getNumberOfSpectra(WorkspaceID workspaceID) const {
  if (workspaceID < m_fittingData->size())
    return m_fittingData->at(workspaceID.value).numberOfSpectra().value;
  else
    throw std::runtime_error("Cannot find the number of spectra for a workspace: the workspace "
                             "index provided is too large.");
}

size_t DataModel::getNumberOfDomains() const {
  size_t init(0);
  auto const numberOfSpectra = [](size_t sum, auto const &fittingData) {
    return sum + fittingData.numberOfSpectra().value;
  };
  return std::accumulate(m_fittingData->cbegin(), m_fittingData->cend(), init, numberOfSpectra);
}

std::vector<double> DataModel::getQValuesForData() const {
  std::vector<double> qValues;
  for (auto const &fitData : *m_fittingData) {
    auto indexQValues = fitData.getQValues();
    qValues.insert(std::end(qValues), std::begin(indexQValues), std::end(indexQValues));
  }
  return qValues;
}

std::vector<std::pair<std::string, size_t>> DataModel::getResolutionsForFit() const {
  std::vector<std::pair<std::string, size_t>> resolutionVector;
  for (const auto &fittingData : *m_fittingData) {
    std::map<std::string, bool> checkedResolutions;
    for (const auto &[spectrum, resName] : fittingData.getResolutions()) {
      if (!checkedResolutions.contains(resName)) {
        if (m_adsInstance.doesExist(resName)) {
          const auto resWs = m_adsInstance.retrieveWS<Mantid::API::MatrixWorkspace>(resName);
          checkedResolutions.insert_or_assign(resName, resWs->getNumberHistograms() == 1);
        }
      }
      const auto name = checkedResolutions.contains(resName) ? resName : "";
      const auto spIndex = !name.empty() && checkedResolutions.at(resName) ? 0 : spectrum.value;
      resolutionVector.emplace_back(name, spIndex);
    }
  }
  return resolutionVector;
}

std::string DataModel::getResolutionName(const WorkspaceID &wsID, const WorkspaceIndex &index) const {
  return wsID.value < m_fittingData->size() ? m_fittingData->at(wsID.value).getResolutionFromWsIndex(index) : "";
}

bool DataModel::setResolution(const std::string &resName, const std::string &wsName,
                              const FunctionModelSpectra &spectra) {
  const auto workspaceID = getWorkspaceID(wsName);
  if (workspaceID.value >= getNumberOfWorkspaces().value) {
    throw std::runtime_error("Workspace '" + wsName + "' not found in the model.");
  }
  return setResolution(resName, workspaceID, spectra);
}

bool DataModel::setResolution(const std::string &resName, WorkspaceID workspaceID,
                              const FunctionModelSpectra &spectra) {
  bool hasValidValues = true;
  if (!resName.empty() && m_adsInstance.doesExist(resName)) {
    const auto resolution = m_adsInstance.retrieveWS<Mantid::API::MatrixWorkspace>(resName);
    const auto resSpectra = resolution->getNumberHistograms() == 1 ? FunctionModelSpectra(0, 0) : spectra;
    for (const auto &spIndex : resSpectra) {
      try {
        const auto &y = resolution->readY(spIndex.value);
        hasValidValues = hasValidValues && std::ranges::all_of(y, [](double value) { return value == value; });
      } catch (std::range_error const &) {
        // Either resolution has one histogram, or there should be 1-1 correspondence
      }
    }
    m_fittingData->at(workspaceID.value).setResolution(resName, spectra);
  } else {
    throw std::runtime_error("A valid resolution file needs to be selected.");
  }
  return hasValidValues;
}

void DataModel::removeResolution(const std::string &resName) {
  for (auto it = m_fittingData->begin(); it != m_fittingData->end();) {
    auto &fitData = *it;
    if (const auto names = fitData.getResolutionNames(); names.contains(resName)) {
      if (names.size() == 1) {
        // If there's only one resolution ws linked to a ws we remove the ws from the model as convolution is not
        // possible.
        it = m_fittingData->erase(it);
      } else {
        fitData.removeResolution(resName);
        ++it;
      }
    } else {
      ++it;
    }
  }
}

void DataModel::removeSpecialValues(const std::string &name) {
  auto alg = Mantid::API::AlgorithmManager::Instance().create("ReplaceSpecialValues");
  alg->initialize();
  alg->setProperty("InputWorkspace", name);
  alg->setProperty("OutputWorkspace", name);
  alg->setProperty("NaNValue", 0.0);
  alg->setProperty("InfinityValue", 0.0);
  alg->execute();
}

void DataModel::setSpectra(const std::string &spectra, WorkspaceID workspaceID) {
  setSpectra(FunctionModelSpectra(spectra), workspaceID);
}

void DataModel::setSpectra(FunctionModelSpectra &&spectra, WorkspaceID workspaceID) {
  if (m_fittingData->empty())
    return;
  m_fittingData->at(workspaceID.value).setSpectra(std::forward<FunctionModelSpectra>(spectra));
}

void DataModel::setSpectra(const FunctionModelSpectra &spectra, WorkspaceID workspaceID) {
  if (m_fittingData->empty())
    return;
  m_fittingData->at(workspaceID.value).setSpectra(spectra);
}

std::vector<std::string> DataModel::getWorkspaceNames() const {
  std::vector<std::string> names;
  names.reserve(m_fittingData->size());
  std::transform(m_fittingData->cbegin(), m_fittingData->cend(), std::back_inserter(names),
                 [](const auto &fittingData) { return fittingData.getWsName(); });
  return names;
}

void DataModel::addWorkspace(const std::string &workspaceName, const FunctionModelSpectra &spectra) {
  if (workspaceName.empty() || !m_adsInstance.doesExist(workspaceName))
    throw std::runtime_error("A valid sample file needs to be selected.");
  if (spectra.empty())
    throw std::runtime_error("Fitting Data must consist of one or more spectra.");

  auto ws = m_adsInstance.retrieveWS<Mantid::API::MatrixWorkspace>(workspaceName);
  addWorkspace(std::move(ws), spectra);
}

void DataModel::addWorkspace(Mantid::API::MatrixWorkspace_sptr workspace, const FunctionModelSpectra &spectra) {
  if (!m_fittingData->empty()) {
    auto it = std::find_if(m_fittingData->begin(), m_fittingData->end(), [&](FitData const &fitData) {
      return equivalentWorkspaces(workspace, fitData.workspace());
    });
    if (it != m_fittingData->end()) {
      it->combine(FitData(workspace, spectra));
      return;
    }
  }
  addNewWorkspace(workspace, spectra);
}

void DataModel::addNewWorkspace(const Mantid::API::MatrixWorkspace_sptr &workspace,
                                const FunctionModelSpectra &spectra) {
  m_fittingData->emplace_back(workspace, spectra);
}

FitDomainIndex DataModel::getDomainIndex(WorkspaceID workspaceID, WorkspaceIndex spectrum) const {
  FitDomainIndex index{0};
  for (size_t iws = 0; iws < m_fittingData->size(); ++iws) {
    if (iws < workspaceID.value) {
      index += getNumberOfSpectra(iws);
    } else {
      auto const spectra = getSpectra(iws);
      try {
        index += spectra.indexOf(spectrum);
      } catch (const std::runtime_error &) {
        if (spectrum.value != 0)
          throw;
      }
      break;
    }
  }
  return index;
}

void DataModel::clear() { m_fittingData->clear(); }

std::set<std::string> DataModel::getResolutionNames() {
  std::set<std::string> names;
  for (const auto &fitData : *m_fittingData) {
    const auto resNames = fitData.getResolutionNames();
    names.insert(resNames.begin(), resNames.end());
  }
  return names;
}

std::set<std::string> const &DataModel::resolutionNames() const { return m_uniqueResWsNames; }
std::set<std::string> const &DataModel::workspaceNames() const { return m_uniqueWsNames; }

void DataModel::updateWorkspaceNames() {
  // This makes it faster for the observers
  const auto names = getWorkspaceNames();
  m_uniqueWsNames = std::set<std::string>(names.begin(), names.end());
  m_uniqueResWsNames = getResolutionNames();
}

std::pair<double, double> DataModel::getFittingRange(WorkspaceID workspaceID, WorkspaceIndex spectrum) const {
  if (workspaceID.value < m_fittingData->size() && !m_fittingData->at(workspaceID.value).zeroSpectra()) {
    return m_fittingData->at(workspaceID.value).getRange(spectrum);
  }
  return std::make_pair(0., 0.);
}

std::string DataModel::getExcludeRegion(WorkspaceID workspaceID, WorkspaceIndex spectrum) const {
  if (workspaceID.value < m_fittingData->size() && !m_fittingData->at(workspaceID.value).zeroSpectra()) {
    return m_fittingData->at(workspaceID.value).getExcludeRegion(spectrum);
  }
  return "";
}

void DataModel::setStartX(double startX, WorkspaceID workspaceID, WorkspaceIndex spectrum) {
  if (m_fittingData->empty())
    return;
  m_fittingData->at(workspaceID.value).setStartX(startX, spectrum);
}

void DataModel::setStartX(double startX, WorkspaceID workspaceID) {
  if (m_fittingData->empty())
    return;
  m_fittingData->at(workspaceID.value).setStartX(startX);
}

void DataModel::setStartX(double startX, FitDomainIndex fitDomainIndex) {
  auto subIndices = getSubIndices(fitDomainIndex);
  if (m_fittingData->empty())
    return;
  m_fittingData->at(subIndices.first.value).setStartX(startX, subIndices.second);
}

void DataModel::setEndX(double endX, WorkspaceID workspaceID, WorkspaceIndex spectrum) {
  if (m_fittingData->empty())
    return;
  m_fittingData->at(workspaceID.value).setEndX(endX, spectrum);
}

void DataModel::setEndX(double endX, WorkspaceID workspaceID) {
  if (m_fittingData->empty())
    return;
  m_fittingData->at(workspaceID.value).setEndX(endX);
}

void DataModel::setEndX(double endX, FitDomainIndex fitDomainIndex) {
  auto subIndices = getSubIndices(fitDomainIndex);
  if (m_fittingData->empty())
    return;
  m_fittingData->at(subIndices.first.value).setEndX(endX, subIndices.second);
}

void DataModel::setExcludeRegion(const std::string &exclude, WorkspaceID workspaceID, WorkspaceIndex spectrum) {
  if (m_fittingData->empty())
    return;
  m_fittingData->at(workspaceID.value).setExcludeRegionString(exclude, spectrum);
}

void DataModel::removeWorkspaceByName(const std::string &name) {
  auto it = std::find_if(m_fittingData->cbegin(), m_fittingData->cend(),
                         [&name](const auto &fittingData) { return fittingData.getWsName() == name; });
  if (it != m_fittingData->cend()) {
    m_fittingData->erase(it);
  }
}

void DataModel::removeWorkspace(WorkspaceID workspaceID) {
  if (workspaceID < m_fittingData->size()) {
    m_fittingData->erase(m_fittingData->begin() + workspaceID.value);
  } else {
    throw std::runtime_error("Attempting to remove non-existent workspace.");
  }
}

void DataModel::removeDataByIndex(FitDomainIndex fitDomainIndex) {
  auto subIndices = getSubIndices(fitDomainIndex);
  auto &fitData = m_fittingData->at(subIndices.first.value);
  auto &spectra = fitData.getMutableSpectra();
  spectra.erase(subIndices.second);
  fitData.removeResolutionEntry(subIndices.second);

  // If the spectra list corresponding to a workspace is empty, remove workspace
  // at this index, else we'll have a workspace persist with no spectra loaded.
  if (spectra.empty()) {
    removeWorkspace(subIndices.first.value);
  }
}

std::vector<double> DataModel::getExcludeRegionVector(WorkspaceID workspaceID, WorkspaceIndex spectrum) const {
  auto const &fitData = m_fittingData->at(workspaceID.value);
  return fitData.excludeRegionsVector(spectrum);
}

Mantid::API::MatrixWorkspace_sptr DataModel::getWorkspace(FitDomainIndex index) const {
  auto subIndices = getSubIndices(index);
  return getWorkspace(subIndices.first);
}

std::pair<double, double> DataModel::getFittingRange(FitDomainIndex index) const {
  auto subIndices = getSubIndices(index);
  return getFittingRange(subIndices.first, subIndices.second);
}

size_t DataModel::getSpectrum(FitDomainIndex index) const {
  auto subIndices = getSubIndices(index);
  return subIndices.second.value;
}

std::vector<double> DataModel::getExcludeRegionVector(FitDomainIndex index) const {
  auto subIndices = getSubIndices(index);
  return getExcludeRegionVector(subIndices.first, subIndices.second);
}

std::string DataModel::getExcludeRegion(FitDomainIndex index) const {
  auto subIndices = getSubIndices(index);
  return getExcludeRegion(subIndices.first, subIndices.second);
}

void DataModel::setExcludeRegion(const std::string &exclude, FitDomainIndex index) {
  if (m_fittingData->empty())
    return;
  const auto subIndices = getSubIndices(index);
  m_fittingData->at(subIndices.first.value).setExcludeRegionString(exclude, subIndices.second);
}

std::pair<WorkspaceID, WorkspaceIndex> DataModel::getSubIndices(FitDomainIndex index) const {
  size_t sum{0};
  for (size_t workspaceID = 0; workspaceID < m_fittingData->size(); workspaceID++) {
    for (size_t workspaceIndex = 0; workspaceIndex < m_fittingData->at(workspaceID).spectra().size().value;
         workspaceIndex++) {
      if (sum == index.value) {
        WorkspaceIndex spectrum = m_fittingData->at(workspaceID).spectra()[FitDomainIndex{workspaceIndex}];
        return std::make_pair(WorkspaceID{workspaceID}, spectrum);
      }
      sum++;
    }
  }
  throw std::runtime_error("Failed to find workspace and spectrum index for fit domain.");
}
} // namespace MantidQt::CustomInterfaces::Inelastic
