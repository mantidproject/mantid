// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <string>

#include "DllConfig.h"
#include "FitData.h"
#include "FitDataModel.h"
#include "MantidAPI/AlgorithmManager.h"
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

FitDataModel::FitDataModel()
    : m_fittingData(std::make_unique<std::vector<FitData>>()),
      m_resolutions(std::make_unique<std::vector<std::weak_ptr<Mantid::API::MatrixWorkspace>>>()),
      m_adsInstance(Mantid::API::AnalysisDataService::Instance()) {}

std::vector<FitData> *FitDataModel::getFittingData() { return m_fittingData.get(); }

bool FitDataModel::hasWorkspace(std::string const &workspaceName) const {
  auto const names = getWorkspaceNames();
  auto const iter = std::find(names.cbegin(), names.cend(), workspaceName);
  return iter != names.cend();
}

Mantid::API::MatrixWorkspace_sptr FitDataModel::getWorkspace(WorkspaceID workspaceID) const {
  if (workspaceID < m_fittingData->size())
    return m_fittingData->at(workspaceID.value).workspace();
  return nullptr;
}

FunctionModelSpectra FitDataModel::getSpectra(WorkspaceID workspaceID) const {
  if (workspaceID < m_fittingData->size())
    return m_fittingData->at(workspaceID.value).spectra();
  return FunctionModelSpectra("");
}

std::string FitDataModel::createDisplayName(WorkspaceID workspaceID) const {
  if (getNumberOfWorkspaces() > workspaceID)
    return getFitDataName(getWorkspaceNames()[workspaceID.value], getSpectra(workspaceID));
  else
    throw std::runtime_error("Cannot create a display name for a workspace:"
                             "the workspace index provided is too large.");
}

WorkspaceID FitDataModel::getNumberOfWorkspaces() const { return WorkspaceID{m_fittingData->size()}; }

size_t FitDataModel::getNumberOfSpectra(WorkspaceID workspaceID) const {
  if (workspaceID < m_fittingData->size())
    return m_fittingData->at(workspaceID.value).numberOfSpectra().value;
  else
    throw std::runtime_error("Cannot find the number of spectra for a workspace: the workspace "
                             "index provided is too large.");
}

size_t FitDataModel::getNumberOfDomains() const {
  size_t init(0);
  auto const numberOfSpectra = [](size_t sum, auto const &fittingData) {
    return sum + fittingData.numberOfSpectra().value;
  };
  return std::accumulate(m_fittingData->cbegin(), m_fittingData->cend(), init, numberOfSpectra);
}

std::vector<double> FitDataModel::getQValuesForData() const {
  std::vector<double> qValues;
  for (auto const &fitData : *m_fittingData) {
    auto indexQValues = fitData.getQValues();
    qValues.insert(std::end(qValues), std::begin(indexQValues), std::end(indexQValues));
  }
  return qValues;
}

std::vector<std::pair<std::string, size_t>> FitDataModel::getResolutionsForFit() const {
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
    for (auto const &spectraIndex : spectra) {
      const auto resolutionIndex = singleSpectraResolution ? 0 : spectraIndex.value;
      resolutionVector.emplace_back(m_resolutions->at(index).lock()->getName(), resolutionIndex);
    }
  }
  return resolutionVector;
}

bool FitDataModel::setResolution(const std::string &name) {
  return setResolution(name, getNumberOfWorkspaces() - WorkspaceID{1});
}

bool FitDataModel::setResolution(const std::string &name, WorkspaceID workspaceID) {
  bool hasValidValues = true;
  if (!name.empty() && m_adsInstance.doesExist(name)) {
    const auto resolution = m_adsInstance.retrieveWS<Mantid::API::MatrixWorkspace>(name);
    auto y = resolution->readY(workspaceID.value);
    hasValidValues = std::all_of(y.cbegin(), y.cend(), [](double value) { return value == value; });

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
  return hasValidValues;
}

void FitDataModel::removeSpecialValues(const std::string &name) {
  auto alg = Mantid::API::AlgorithmManager::Instance().create("ReplaceSpecialValues");
  alg->initialize();
  alg->setProperty("InputWorkspace", name);
  alg->setProperty("OutputWorkspace", name);
  alg->setProperty("NaNValue", 0.0);
  alg->setProperty("InfinityValue", 0.0);
  alg->execute();
}

void FitDataModel::setSpectra(const std::string &spectra, WorkspaceID workspaceID) {
  setSpectra(FunctionModelSpectra(spectra), workspaceID);
}

void FitDataModel::setSpectra(FunctionModelSpectra &&spectra, WorkspaceID workspaceID) {
  if (m_fittingData->empty())
    return;
  m_fittingData->at(workspaceID.value).setSpectra(std::forward<FunctionModelSpectra>(spectra));
}

void FitDataModel::setSpectra(const FunctionModelSpectra &spectra, WorkspaceID workspaceID) {
  if (m_fittingData->empty())
    return;
  m_fittingData->at(workspaceID.value).setSpectra(spectra);
}

std::vector<std::string> FitDataModel::getWorkspaceNames() const {
  std::vector<std::string> names;
  names.reserve(m_fittingData->size());
  std::transform(m_fittingData->cbegin(), m_fittingData->cend(), std::back_inserter(names),
                 [](const auto &fittingData) { return fittingData.workspace()->getName(); });
  return names;
}

void FitDataModel::addWorkspace(const std::string &workspaceName, const FunctionModelSpectra &spectra) {
  if (workspaceName.empty() || !m_adsInstance.doesExist(workspaceName))
    throw std::runtime_error("A valid sample file needs to be selected.");
  if (spectra.empty())
    throw std::runtime_error("Fitting Data must consist of one or more spectra.");

  auto ws = m_adsInstance.retrieveWS<Mantid::API::MatrixWorkspace>(workspaceName);
  addWorkspace(ws, spectra);
}

void FitDataModel::addWorkspace(Mantid::API::MatrixWorkspace_sptr workspace, const FunctionModelSpectra &spectra) {
  if (!m_fittingData->empty()) {
    auto it = std::find_if((*m_fittingData).begin(), (*m_fittingData).end(), [&](FitData const &fitData) {
      return equivalentWorkspaces(workspace, fitData.workspace());
    });
    if (it != (*m_fittingData).end()) {
      (*it).combine(FitData(workspace, spectra));
    }
  }
  addNewWorkspace(workspace, spectra);
}

void FitDataModel::addNewWorkspace(const Mantid::API::MatrixWorkspace_sptr &workspace,
                                   const FunctionModelSpectra &spectra) {
  m_fittingData->emplace_back(workspace, spectra);
}

FitDomainIndex FitDataModel::getDomainIndex(WorkspaceID workspaceID, WorkspaceIndex spectrum) const {
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

void FitDataModel::clear() { m_fittingData->clear(); }

std::pair<double, double> FitDataModel::getFittingRange(WorkspaceID workspaceID, WorkspaceIndex spectrum) const {
  if (workspaceID.value < m_fittingData->size() && !m_fittingData->at(workspaceID.value).zeroSpectra()) {
    return m_fittingData->at(workspaceID.value).getRange(spectrum);
  }
  return std::make_pair(0., 0.);
}

std::string FitDataModel::getExcludeRegion(WorkspaceID workspaceID, WorkspaceIndex spectrum) const {
  if (workspaceID.value < m_fittingData->size() && !m_fittingData->at(workspaceID.value).zeroSpectra()) {
    return m_fittingData->at(workspaceID.value).getExcludeRegion(spectrum);
  }
  return "";
}

void FitDataModel::setStartX(double startX, WorkspaceID workspaceID, WorkspaceIndex spectrum) {
  if (m_fittingData->empty())
    return;
  m_fittingData->at(workspaceID.value).setStartX(startX, spectrum);
}

void FitDataModel::setStartX(double startX, WorkspaceID workspaceID) {
  if (m_fittingData->empty())
    return;
  m_fittingData->at(workspaceID.value).setStartX(startX);
}

void FitDataModel::setStartX(double startX, FitDomainIndex fitDomainIndex) {
  auto subIndices = getSubIndices(fitDomainIndex);
  if (m_fittingData->empty())
    return;
  m_fittingData->at(subIndices.first.value).setStartX(startX, subIndices.second);
}

void FitDataModel::setEndX(double endX, WorkspaceID workspaceID, WorkspaceIndex spectrum) {
  if (m_fittingData->empty())
    return;
  m_fittingData->at(workspaceID.value).setEndX(endX, spectrum);
}

void FitDataModel::setEndX(double endX, WorkspaceID workspaceID) {
  if (m_fittingData->empty())
    return;
  m_fittingData->at(workspaceID.value).setEndX(endX);
}

void FitDataModel::setEndX(double endX, FitDomainIndex fitDomainIndex) {
  auto subIndices = getSubIndices(fitDomainIndex);
  if (m_fittingData->empty())
    return;
  m_fittingData->at(subIndices.first.value).setEndX(endX, subIndices.second);
}

void FitDataModel::setExcludeRegion(const std::string &exclude, WorkspaceID workspaceID, WorkspaceIndex spectrum) {
  if (m_fittingData->empty())
    return;
  m_fittingData->at(workspaceID.value).setExcludeRegionString(exclude, spectrum);
}

void FitDataModel::removeWorkspace(WorkspaceID workspaceID) {
  if (workspaceID < m_fittingData->size()) {
    m_fittingData->erase(m_fittingData->begin() + workspaceID.value);
  } else {
    throw std::runtime_error("Attempting to remove non-existent workspace.");
  }
}

void FitDataModel::removeDataByIndex(FitDomainIndex fitDomainIndex) {
  auto subIndices = getSubIndices(fitDomainIndex);
  auto &spectra = m_fittingData->at(subIndices.first.value).getMutableSpectra();
  spectra.erase(subIndices.second);
  // If the spectra list corresponding to a workspace is empty, remove workspace
  // at this index, else we'll have a workspace persist with no spectra loaded.
  if (spectra.empty()) {
    removeWorkspace(subIndices.first.value);
  }
}

std::vector<double> FitDataModel::getExcludeRegionVector(WorkspaceID workspaceID, WorkspaceIndex spectrum) const {
  auto fitData = m_fittingData->at(workspaceID.value);
  return fitData.excludeRegionsVector(spectrum);
}

Mantid::API::MatrixWorkspace_sptr FitDataModel::getWorkspace(FitDomainIndex index) const {
  auto subIndices = getSubIndices(index);
  return getWorkspace(subIndices.first);
}

std::pair<double, double> FitDataModel::getFittingRange(FitDomainIndex index) const {
  auto subIndices = getSubIndices(index);
  return getFittingRange(subIndices.first, subIndices.second);
}

size_t FitDataModel::getSpectrum(FitDomainIndex index) const {
  auto subIndices = getSubIndices(index);
  return subIndices.second.value;
}

std::vector<double> FitDataModel::getExcludeRegionVector(FitDomainIndex index) const {
  auto subIndices = getSubIndices(index);
  return getExcludeRegionVector(subIndices.first, subIndices.second);
}

std::string FitDataModel::getExcludeRegion(FitDomainIndex index) const {
  auto subIndices = getSubIndices(index);
  return getExcludeRegion(subIndices.first, subIndices.second);
}

void FitDataModel::setExcludeRegion(const std::string &exclude, FitDomainIndex index) {
  if (m_fittingData->empty())
    return;
  const auto subIndices = getSubIndices(index);
  m_fittingData->at(subIndices.first.value).setExcludeRegionString(exclude, subIndices.second);
}

std::pair<WorkspaceID, WorkspaceIndex> FitDataModel::getSubIndices(FitDomainIndex index) const {
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
