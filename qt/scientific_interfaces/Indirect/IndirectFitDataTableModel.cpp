// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <string>

#include "DllConfig.h"
#include "IndirectFitData.h"
#include "IndirectFitDataTableModel.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"

using namespace MantidQt::MantidWidgets;

namespace {

bool equivalentWorkspaces(const Mantid::API::MatrixWorkspace_const_sptr &lhs,
                          const Mantid::API::MatrixWorkspace_const_sptr &rhs) {
  if (!lhs || !rhs)
    return false;
  else if (lhs->getName() == "" && rhs->getName() == "")
    return lhs == rhs;
  return lhs->getName() == rhs->getName();
}

} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectFitDataTableModel::IndirectFitDataTableModel()
    : m_fittingDataMultiple(std::make_unique<std::vector<IndirectFitData>>()),
      m_resolutionsMultiple(std::make_unique<std::vector<std::weak_ptr<Mantid::API::MatrixWorkspace>>>()),
      m_adsInstance(Mantid::API::AnalysisDataService::Instance()) {

  m_fittingData = m_fittingDataMultiple.get();
  m_resolutions = m_resolutionsMultiple.get();
}

bool IndirectFitDataTableModel::hasWorkspace(std::string const &workspaceName) const {
  auto const names = getWorkspaceNames();
  auto const iter = std::find(names.cbegin(), names.cend(), workspaceName);
  return iter != names.cend();
}

Mantid::API::MatrixWorkspace_sptr IndirectFitDataTableModel::getWorkspace(WorkspaceID workspaceID) const {
  if (workspaceID < m_fittingData->size())
    return m_fittingData->at(workspaceID.value).workspace();
  return nullptr;
}

FunctionModelSpectra IndirectFitDataTableModel::getSpectra(WorkspaceID workspaceID) const {
  if (workspaceID < m_fittingData->size())
    return m_fittingData->at(workspaceID.value).spectra();
  return FunctionModelSpectra("");
}

WorkspaceID IndirectFitDataTableModel::getNumberOfWorkspaces() const { return WorkspaceID{m_fittingData->size()}; }

bool IndirectFitDataTableModel::isMultiFit() const { return getNumberOfWorkspaces().value > 1; }

size_t IndirectFitDataTableModel::getNumberOfSpectra(WorkspaceID workspaceID) const {
  if (workspaceID < m_fittingData->size())
    return m_fittingData->at(workspaceID.value).numberOfSpectra().value;
  else
    throw std::runtime_error("Cannot find the number of spectra for a workspace: the workspace "
                             "index provided is too large.");
}

size_t IndirectFitDataTableModel::getNumberOfDomains() const {
  size_t init(0);
  auto const getNumberOfSpectra = [](size_t sum, auto const &fittingData) {
    return sum + fittingData.numberOfSpectra().value;
  };
  return std::accumulate(m_fittingData->cbegin(), m_fittingData->cend(), init, getNumberOfSpectra);
}

std::vector<double> IndirectFitDataTableModel::getQValuesForData() const {
  std::vector<double> qValues;
  for (auto &fitData : *m_fittingData) {
    auto indexQValues = fitData.getQValues();
    qValues.insert(std::end(qValues), std::begin(indexQValues), std::end(indexQValues));
  }
  return qValues;
}

std::vector<std::pair<std::string, size_t>> IndirectFitDataTableModel::getResolutionsForFit() const {
  std::vector<std::pair<std::string, size_t>> resolutionVector;
  for (size_t index = 0; index < m_resolutions->size(); ++index) {
    const auto resolutionWorkspace = m_resolutions->at(index).lock();
    const auto spectra = getSpectra(WorkspaceID{index});
    if (!resolutionWorkspace) {
      for (auto &spectraIndex : spectra) {
        resolutionVector.emplace_back("", spectraIndex.value);
      }
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

void IndirectFitDataTableModel::setResolution(const std::string &name, WorkspaceID workspaceID) {
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

void IndirectFitDataTableModel::setSpectra(const std::string &spectra, WorkspaceID workspaceID) {
  setSpectra(FunctionModelSpectra(spectra), workspaceID);
}

void IndirectFitDataTableModel::setSpectra(FunctionModelSpectra &&spectra, WorkspaceID workspaceID) {
  if (m_fittingData->empty())
    return;
  m_fittingData->at(workspaceID.value).setSpectra(std::forward<FunctionModelSpectra>(spectra));
}

void IndirectFitDataTableModel::setSpectra(const FunctionModelSpectra &spectra, WorkspaceID workspaceID) {
  if (m_fittingData->empty())
    return;
  m_fittingData->at(workspaceID.value).setSpectra(spectra);
}

std::vector<std::string> IndirectFitDataTableModel::getWorkspaceNames() const {
  std::vector<std::string> names;
  names.reserve(m_fittingData->size());
  for (const auto &fittingData : *m_fittingData)
    names.emplace_back(fittingData.workspace()->getName());
  return names;
}

void IndirectFitDataTableModel::addWorkspace(const std::string &workspaceName) {
  auto ws = m_adsInstance.retrieveWS<Mantid::API::MatrixWorkspace>(workspaceName);
  addWorkspace(ws, FunctionModelSpectra(WorkspaceIndex{0}, WorkspaceIndex{ws->getNumberHistograms() - 1}));
}

void IndirectFitDataTableModel::addWorkspace(const std::string &workspaceName, const std::string &spectra) {
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

void IndirectFitDataTableModel::addWorkspace(const std::string &workspaceName, const FunctionModelSpectra &spectra) {
  auto ws = m_adsInstance.retrieveWS<Mantid::API::MatrixWorkspace>(workspaceName);
  addWorkspace(ws, spectra);
}

void IndirectFitDataTableModel::addWorkspace(Mantid::API::MatrixWorkspace_sptr workspace,
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

FitDomainIndex IndirectFitDataTableModel::getDomainIndex(WorkspaceID workspaceID, WorkspaceIndex spectrum) const {
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

void IndirectFitDataTableModel::clear() { m_fittingData->clear(); }

std::pair<double, double> IndirectFitDataTableModel::getFittingRange(WorkspaceID workspaceID,
                                                                     WorkspaceIndex spectrum) const {
  if (workspaceID.value < m_fittingData->size() && !m_fittingData->at(workspaceID.value).zeroSpectra()) {
    return m_fittingData->at(workspaceID.value).getRange(spectrum);
  }
  return std::make_pair(0., 0.);
}

std::string IndirectFitDataTableModel::getExcludeRegion(WorkspaceID workspaceID, WorkspaceIndex spectrum) const {
  if (workspaceID.value < m_fittingData->size() && !m_fittingData->at(workspaceID.value).zeroSpectra()) {
    return m_fittingData->at(workspaceID.value).getExcludeRegion(spectrum);
  }
  return "";
}

void IndirectFitDataTableModel::setStartX(double startX, WorkspaceID workspaceID, WorkspaceIndex spectrum) {
  if (m_fittingData->empty())
    return;
  m_fittingData->at(workspaceID.value).setStartX(startX, spectrum);
}

void IndirectFitDataTableModel::setStartX(double startX, WorkspaceID workspaceID) {
  if (m_fittingData->empty())
    return;
  m_fittingData->at(workspaceID.value).setStartX(startX);
}

void IndirectFitDataTableModel::setEndX(double endX, WorkspaceID workspaceID, WorkspaceIndex spectrum) {
  if (m_fittingData->empty())
    return;
  m_fittingData->at(workspaceID.value).setEndX(endX, spectrum);
}

void IndirectFitDataTableModel::setEndX(double endX, WorkspaceID workspaceID) {
  if (m_fittingData->empty())
    return;
  m_fittingData->at(workspaceID.value).setEndX(endX);
}

void IndirectFitDataTableModel::setExcludeRegion(const std::string &exclude, WorkspaceID workspaceID,
                                                 WorkspaceIndex spectrum) {
  if (m_fittingData->empty())
    return;
  m_fittingData->at(workspaceID.value).setExcludeRegionString(exclude, spectrum);
}

void IndirectFitDataTableModel::addNewWorkspace(const Mantid::API::MatrixWorkspace_sptr &workspace,
                                                const FunctionModelSpectra &spectra) {
  m_fittingData->emplace_back(workspace, spectra);
}

void IndirectFitDataTableModel::removeWorkspace(WorkspaceID workspaceID) {
  if (workspaceID < m_fittingData->size()) {
    m_fittingData->erase(m_fittingData->begin() + workspaceID.value);
  } else {
    throw std::runtime_error("Attempting to remove non-existent workspace.");
  }
}

void IndirectFitDataTableModel::removeDataByIndex(FitDomainIndex fitDomainIndex) {
  auto subIndices = getSubIndices(fitDomainIndex);
  auto &spectra = m_fittingData->at(subIndices.first.value).getMutableSpectra();
  spectra.erase(subIndices.second);
  // If the spectra list corresponding to a workspace is empty, remove workspace
  // at this index, else we'll have a workspace persist with no spectra loaded.
  if (spectra.empty()) {
    removeWorkspace(subIndices.first.value);
  }
}

std::vector<double> IndirectFitDataTableModel::getExcludeRegionVector(WorkspaceID workspaceID,
                                                                      WorkspaceIndex spectrum) const {
  auto fitData = m_fittingData->at(workspaceID.value);
  return fitData.excludeRegionsVector(spectrum);
}

Mantid::API::MatrixWorkspace_sptr IndirectFitDataTableModel::getWorkspace(FitDomainIndex index) const {
  auto subIndices = getSubIndices(index);
  return getWorkspace(subIndices.first);
}

std::pair<double, double> IndirectFitDataTableModel::getFittingRange(FitDomainIndex index) const {
  auto subIndices = getSubIndices(index);
  return getFittingRange(subIndices.first, subIndices.second);
}

size_t IndirectFitDataTableModel::getSpectrum(FitDomainIndex index) const {
  auto subIndices = getSubIndices(index);
  return subIndices.second.value;
}

std::vector<double> IndirectFitDataTableModel::getExcludeRegionVector(FitDomainIndex index) const {
  auto subIndices = getSubIndices(index);
  return getExcludeRegionVector(subIndices.first, subIndices.second);
}

std::string IndirectFitDataTableModel::getExcludeRegion(FitDomainIndex index) const {
  auto subIndices = getSubIndices(index);
  return getExcludeRegion(subIndices.first, subIndices.second);
}

void IndirectFitDataTableModel::setExcludeRegion(const std::string &exclude, FitDomainIndex index) {
  if (m_fittingData->empty())
    return;
  const auto subIndices = getSubIndices(index);
  m_fittingData->at(subIndices.first.value).setExcludeRegionString(exclude, subIndices.second);
}

std::pair<WorkspaceID, WorkspaceIndex> IndirectFitDataTableModel::getSubIndices(FitDomainIndex index) const {
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

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
