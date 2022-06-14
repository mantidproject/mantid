// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <string>

#include "DllConfig.h"
#include "IndirectFitData.h"
#include "IndirectFitDataModel.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"

using namespace MantidQt::MantidWidgets;

namespace {

using namespace MantidQt::CustomInterfaces::IDA;

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

namespace MantidQt::CustomInterfaces::IDA {

IndirectFitDataModel::IndirectFitDataModel()
    : m_fittingData(std::make_unique<std::vector<IndirectFitData>>()),
      m_resolutions(std::make_unique<std::vector<std::weak_ptr<Mantid::API::MatrixWorkspace>>>()),
      m_adsInstance(Mantid::API::AnalysisDataService::Instance()) {}

std::vector<IndirectFitData> *IndirectFitDataModel::getFittingData() { return m_fittingData.get(); }

bool IndirectFitDataModel::hasWorkspace(std::string const &workspaceName) const {
  auto const names = getWorkspaceNames();
  auto const iter = std::find(names.cbegin(), names.cend(), workspaceName);
  return iter != names.cend();
}

Mantid::API::MatrixWorkspace_sptr IndirectFitDataModel::getWorkspace(WorkspaceID workspaceID) const {
  if (workspaceID < m_fittingData->size())
    return m_fittingData->at(workspaceID.value).workspace();
  return nullptr;
}

FunctionModelSpectra IndirectFitDataModel::getSpectra(WorkspaceID workspaceID) const {
  if (workspaceID < m_fittingData->size())
    return m_fittingData->at(workspaceID.value).spectra();
  return FunctionModelSpectra("");
}

std::string IndirectFitDataModel::createDisplayName(WorkspaceID workspaceID) const {
  if (getNumberOfWorkspaces() > workspaceID)
    return getFitDataName(getWorkspaceNames()[workspaceID.value], getSpectra(workspaceID));
  else
    throw std::runtime_error("Cannot create a display name for a workspace:"
                             "the workspace index provided is too large.");
}

WorkspaceID IndirectFitDataModel::getNumberOfWorkspaces() const { return WorkspaceID{m_fittingData->size()}; }

size_t IndirectFitDataModel::getNumberOfSpectra(WorkspaceID workspaceID) const {
  if (workspaceID < m_fittingData->size())
    return m_fittingData->at(workspaceID.value).numberOfSpectra().value;
  else
    throw std::runtime_error("Cannot find the number of spectra for a workspace: the workspace "
                             "index provided is too large.");
}

size_t IndirectFitDataModel::getNumberOfDomains() const {
  size_t init(0);
  auto const getNumberOfSpectra = [](size_t sum, auto const &fittingData) {
    return sum + fittingData.numberOfSpectra().value;
  };
  return std::accumulate(m_fittingData->cbegin(), m_fittingData->cend(), init, getNumberOfSpectra);
}

std::vector<double> IndirectFitDataModel::getQValuesForData() const {
  std::vector<double> qValues;
  for (auto &fitData : *m_fittingData) {
    auto indexQValues = fitData.getQValues();
    qValues.insert(std::end(qValues), std::begin(indexQValues), std::end(indexQValues));
  }
  return qValues;
}

std::vector<std::pair<std::string, size_t>> IndirectFitDataModel::getResolutionsForFit() const {
  std::vector<std::pair<std::string, size_t>> resolutionVector;
  for (size_t index = 0; index < m_resolutions->size(); ++index) {
    const auto resolutionWorkspace = m_resolutions->at(index).lock();
    const auto spectra = getSpectra(WorkspaceID{index});
    if (!resolutionWorkspace) {
      resolutionVector.reserve(spectra.size().value);
      std::transform(spectra.begin(), spectra.end(), std::back_inserter(resolutionVector),
                     [](const auto &spectraIndex) { return std::make_pair("", spectraIndex.value); });
      continue;
    }

    const auto singleSpectraResolution = m_resolutions->at(index).lock()->getNumberHistograms() == 1;
    for (auto &spectraIndex : spectra) {
      const auto resolutionIndex = singleSpectraResolution ? 0 : spectraIndex.value;
      resolutionVector.emplace_back(m_resolutions->at(index).lock()->getName(), resolutionIndex);
    }
  }
  return resolutionVector;
}

void IndirectFitDataModel::setResolution(const std::string &name) {
  setResolution(name, getNumberOfWorkspaces() - WorkspaceID{1});
}

void IndirectFitDataModel::setResolution(const std::string &name, WorkspaceID workspaceID) {
  if (!name.empty() && m_adsInstance.doesExist(name)) {
    const auto resolution = m_adsInstance.retrieveWS<Mantid::API::MatrixWorkspace>(name);
    if (m_resolutions->size() > workspaceID.value) {
      m_resolutions->at(workspaceID.value) = resolution;
    } else if (m_resolutions->size() == workspaceID.value) {
      m_resolutions->emplace_back(resolution);
    } else {
      throw std::out_of_range("Provided resolution index '" + std::to_string(workspaceID.value) +
                              "' was out of range.");
    }
  } else {
    throw std::runtime_error("A valid resolution file needs to be selected.");
  }
}

void IndirectFitDataModel::setSpectra(const std::string &spectra, WorkspaceID workspaceID) {
  setSpectra(FunctionModelSpectra(spectra), workspaceID);
}

void IndirectFitDataModel::setSpectra(FunctionModelSpectra &&spectra, WorkspaceID workspaceID) {
  if (m_fittingData->empty())
    return;
  m_fittingData->at(workspaceID.value).setSpectra(std::forward<FunctionModelSpectra>(spectra));
}

void IndirectFitDataModel::setSpectra(const FunctionModelSpectra &spectra, WorkspaceID workspaceID) {
  if (m_fittingData->empty())
    return;
  m_fittingData->at(workspaceID.value).setSpectra(spectra);
}

std::vector<std::string> IndirectFitDataModel::getWorkspaceNames() const {
  std::vector<std::string> names;
  names.reserve(m_fittingData->size());
  std::transform(m_fittingData->cbegin(), m_fittingData->cend(), std::back_inserter(names),
                 [](const auto &fittingData) { return fittingData.workspace()->getName(); });
  return names;
}

void IndirectFitDataModel::addWorkspace(const std::string &workspaceName, const std::string &spectra) {
  if (spectra.empty())
    throw std::runtime_error("Fitting Data must consist of one or more spectra.");
  if (workspaceName.empty() || !m_adsInstance.doesExist(workspaceName))
    throw std::runtime_error("A valid sample file needs to be selected.");

  try {
    addWorkspace(workspaceName, FunctionModelSpectra(spectra));
  } catch (std::logic_error &e) {
    throw std::runtime_error(e.what());
  }
}

void IndirectFitDataModel::addWorkspace(const std::string &workspaceName, const FunctionModelSpectra &spectra) {
  auto ws = m_adsInstance.retrieveWS<Mantid::API::MatrixWorkspace>(workspaceName);
  addWorkspace(ws, spectra);
}

void IndirectFitDataModel::addWorkspace(Mantid::API::MatrixWorkspace_sptr workspace,
                                        const FunctionModelSpectra &spectra) {
  if (!m_fittingData->empty()) {
    for (auto &fitData : *m_fittingData) {
      if (equivalentWorkspaces(workspace, fitData.workspace())) {
        fitData.combine(IndirectFitData(workspace, spectra));
        return;
      }
    }
  }
  addNewWorkspace(workspace, spectra);
}

void IndirectFitDataModel::addNewWorkspace(const Mantid::API::MatrixWorkspace_sptr &workspace,
                                           const FunctionModelSpectra &spectra) {
  m_fittingData->emplace_back(workspace, spectra);
}

FitDomainIndex IndirectFitDataModel::getDomainIndex(WorkspaceID workspaceID, WorkspaceIndex spectrum) const {
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

void IndirectFitDataModel::clear() { m_fittingData->clear(); }

std::pair<double, double> IndirectFitDataModel::getFittingRange(WorkspaceID workspaceID,
                                                                WorkspaceIndex spectrum) const {
  if (workspaceID.value < m_fittingData->size() && !m_fittingData->at(workspaceID.value).zeroSpectra()) {
    return m_fittingData->at(workspaceID.value).getRange(spectrum);
  }
  return std::make_pair(0., 0.);
}

std::string IndirectFitDataModel::getExcludeRegion(WorkspaceID workspaceID, WorkspaceIndex spectrum) const {
  if (workspaceID.value < m_fittingData->size() && !m_fittingData->at(workspaceID.value).zeroSpectra()) {
    return m_fittingData->at(workspaceID.value).getExcludeRegion(spectrum);
  }
  return "";
}

void IndirectFitDataModel::setStartX(double startX, WorkspaceID workspaceID, WorkspaceIndex spectrum) {
  if (m_fittingData->empty())
    return;
  m_fittingData->at(workspaceID.value).setStartX(startX, spectrum);
}

void IndirectFitDataModel::setStartX(double startX, WorkspaceID workspaceID) {
  if (m_fittingData->empty())
    return;
  m_fittingData->at(workspaceID.value).setStartX(startX);
}

void IndirectFitDataModel::setStartX(double startX, FitDomainIndex fitDomainIndex) {
  auto subIndices = getSubIndices(fitDomainIndex);
  if (m_fittingData->empty())
    return;
  m_fittingData->at(subIndices.first.value).setStartX(startX, subIndices.second);
}

void IndirectFitDataModel::setEndX(double endX, WorkspaceID workspaceID, WorkspaceIndex spectrum) {
  if (m_fittingData->empty())
    return;
  m_fittingData->at(workspaceID.value).setEndX(endX, spectrum);
}

void IndirectFitDataModel::setEndX(double endX, WorkspaceID workspaceID) {
  if (m_fittingData->empty())
    return;
  m_fittingData->at(workspaceID.value).setEndX(endX);
}

void IndirectFitDataModel::setEndX(double endX, FitDomainIndex fitDomainIndex) {
  auto subIndices = getSubIndices(fitDomainIndex);
  if (m_fittingData->empty())
    return;
  m_fittingData->at(subIndices.first.value).setEndX(endX, subIndices.second);
}

void IndirectFitDataModel::setExcludeRegion(const std::string &exclude, WorkspaceID workspaceID,
                                            WorkspaceIndex spectrum) {
  if (m_fittingData->empty())
    return;
  m_fittingData->at(workspaceID.value).setExcludeRegionString(exclude, spectrum);
}

void IndirectFitDataModel::removeWorkspace(WorkspaceID workspaceID) {
  if (workspaceID < m_fittingData->size()) {
    m_fittingData->erase(m_fittingData->begin() + workspaceID.value);
  } else {
    throw std::runtime_error("Attempting to remove non-existent workspace.");
  }
}

void IndirectFitDataModel::removeDataByIndex(FitDomainIndex fitDomainIndex) {
  auto subIndices = getSubIndices(fitDomainIndex);
  auto &spectra = m_fittingData->at(subIndices.first.value).getMutableSpectra();
  spectra.erase(subIndices.second);
  // If the spectra list corresponding to a workspace is empty, remove workspace
  // at this index, else we'll have a workspace persist with no spectra loaded.
  if (spectra.empty()) {
    removeWorkspace(subIndices.first.value);
  }
}

std::vector<double> IndirectFitDataModel::getExcludeRegionVector(WorkspaceID workspaceID,
                                                                 WorkspaceIndex spectrum) const {
  auto fitData = m_fittingData->at(workspaceID.value);
  return fitData.excludeRegionsVector(spectrum);
}

Mantid::API::MatrixWorkspace_sptr IndirectFitDataModel::getWorkspace(FitDomainIndex index) const {
  auto subIndices = getSubIndices(index);
  return getWorkspace(subIndices.first);
}

std::pair<double, double> IndirectFitDataModel::getFittingRange(FitDomainIndex index) const {
  auto subIndices = getSubIndices(index);
  return getFittingRange(subIndices.first, subIndices.second);
}

size_t IndirectFitDataModel::getSpectrum(FitDomainIndex index) const {
  auto subIndices = getSubIndices(index);
  return subIndices.second.value;
}

std::vector<double> IndirectFitDataModel::getExcludeRegionVector(FitDomainIndex index) const {
  auto subIndices = getSubIndices(index);
  return getExcludeRegionVector(subIndices.first, subIndices.second);
}

std::string IndirectFitDataModel::getExcludeRegion(FitDomainIndex index) const {
  auto subIndices = getSubIndices(index);
  return getExcludeRegion(subIndices.first, subIndices.second);
}

void IndirectFitDataModel::setExcludeRegion(const std::string &exclude, FitDomainIndex index) {
  if (m_fittingData->empty())
    return;
  const auto subIndices = getSubIndices(index);
  m_fittingData->at(subIndices.first.value).setExcludeRegionString(exclude, subIndices.second);
}

std::pair<WorkspaceID, WorkspaceIndex> IndirectFitDataModel::getSubIndices(FitDomainIndex index) const {
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

} // namespace MantidQt::CustomInterfaces::IDA
