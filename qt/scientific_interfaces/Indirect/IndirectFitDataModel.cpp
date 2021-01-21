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

bool equivalentWorkspaces(const Mantid::API::MatrixWorkspace_const_sptr &lhs,
                          const Mantid::API::MatrixWorkspace_const_sptr &rhs) {
  if (!lhs || !rhs)
    return false;
  else if (lhs->getName() == "" && rhs->getName() == "")
    return lhs == rhs;
  return lhs->getName() == rhs->getName();
}

bool doesExistInADS(std::string const &workspaceName) {
  return Mantid::API::AnalysisDataService::Instance().doesExist(workspaceName);
}
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectFitDataModel::IndirectFitDataModel()
    : m_fittingDataSingle(std::make_unique<std::vector<IndirectFitData>>()),
      m_resolutionsSingle(std::make_unique<std::vector<std::weak_ptr<Mantid::API::MatrixWorkspace>>>()),
      m_fittingDataMultiple(std::make_unique<std::vector<IndirectFitData>>()),
      m_resolutionsMultiple(std::make_unique<std::vector<std::weak_ptr<Mantid::API::MatrixWorkspace>>>()) {

  m_fittingData = m_fittingDataSingle.get();
  m_resolutions = m_resolutionsSingle.get();
}

bool IndirectFitDataModel::hasWorkspace(std::string const &workspaceName) const {
  auto const names = getWorkspaceNames();
  auto const iter = std::find(names.begin(), names.end(), workspaceName);
  return iter != names.end();
}

Mantid::API::MatrixWorkspace_sptr IndirectFitDataModel::getWorkspace(TableDatasetIndex index) const {
  if (index < m_fittingData->size())
    return m_fittingData->at(index.value).workspace();
  return nullptr;
}

FunctionModelSpectra IndirectFitDataModel::getSpectra(TableDatasetIndex index) const {
  if (index < m_fittingData->size())
    return m_fittingData->at(index.value).spectra();
  return FunctionModelSpectra("");
}

TableDatasetIndex IndirectFitDataModel::numberOfWorkspaces() const { return TableDatasetIndex{m_fittingData->size()}; }

bool IndirectFitDataModel::isMultiFit() const { return numberOfWorkspaces().value > 1; }

size_t IndirectFitDataModel::getNumberOfSpectra(TableDatasetIndex index) const {
  if (index < m_fittingData->size())
    return m_fittingData->at(index.value).numberOfSpectra().value;
  else
    throw std::runtime_error("Cannot find the number of spectra for a workspace: the workspace "
                             "index provided is too large.");
  return 1;
}

size_t IndirectFitDataModel::getNumberOfDomains() const {
  size_t sum{0};
  for (auto fittingData : *m_fittingData) {
    sum += fittingData.numberOfSpectra().value;
  }
  return sum;
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
    auto resolutionWorkspace = m_resolutions->at(index).lock();
    auto spectra = getSpectra(TableDatasetIndex{index});
    if (!resolutionWorkspace) {
      for (auto &spectraIndex : spectra) {
        resolutionVector.emplace_back("", spectraIndex.value);
      }
      continue;
    }

    auto singleSpectraResolution = m_resolutions->at(index).lock()->getNumberHistograms() == 1;
    for (auto &spectraIndex : spectra) {
      auto resolutionIndex = singleSpectraResolution ? 0 : spectraIndex.value;
      resolutionVector.emplace_back(m_resolutions->at(index).lock()->getName(), resolutionIndex);
    }
  }
  return resolutionVector;
}

void IndirectFitDataModel::setResolution(const std::string &name, TableDatasetIndex index) {
  if (!name.empty() && doesExistInADS(name)) {
    auto resolution = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>(name);
    if (m_resolutions->size() > index.value) {
      m_resolutions->at(index.value) = resolution;
    } else if (m_resolutions->size() == index.value) {
      m_resolutions->emplace_back(resolution);
    } else {
      throw std::out_of_range("Provided resolution index '" + std::to_string(index.value) + "' was out of range.");
    }
  } else {
    throw std::runtime_error("A valid resolution file needs to be selected.");
  }
}

void IndirectFitDataModel::setSpectra(const std::string &spectra, TableDatasetIndex dataIndex) {
  setSpectra(FunctionModelSpectra(spectra), dataIndex);
}

void IndirectFitDataModel::setSpectra(FunctionModelSpectra &&spectra, TableDatasetIndex dataIndex) {
  if (m_fittingData->empty())
    return;
  m_fittingData->at(dataIndex.value).setSpectra(std::forward<FunctionModelSpectra>(spectra));
}

void IndirectFitDataModel::setSpectra(const FunctionModelSpectra &spectra, TableDatasetIndex dataIndex) {
  if (m_fittingData->empty())
    return;
  m_fittingData->at(dataIndex.value).setSpectra(spectra);
}

std::vector<std::string> IndirectFitDataModel::getWorkspaceNames() const {
  std::vector<std::string> names;
  names.reserve(m_fittingData->size());
  for (const auto &fittingData : *m_fittingData)
    names.emplace_back(fittingData.workspace()->getName());
  return names;
}

void IndirectFitDataModel::addWorkspace(const std::string &workspaceName) {
  auto ws = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>(workspaceName);
  addWorkspace(ws, FunctionModelSpectra(WorkspaceIndex{0}, WorkspaceIndex{ws->getNumberHistograms() - 1}));
}

void IndirectFitDataModel::addWorkspace(const std::string &workspaceName, const std::string &spectra) {
  if (spectra.empty())
    throw std::runtime_error("Fitting Data must consist of one or more spectra.");
  if (workspaceName.empty() || !Mantid::API::AnalysisDataService::Instance().doesExist(workspaceName))
    throw std::runtime_error("A valid sample file needs to be selected.");

  try {
    addWorkspace(workspaceName, FunctionModelSpectra(spectra));
  } catch (std::logic_error &e) {
    throw std::runtime_error(e.what());
  }
}

void IndirectFitDataModel::addWorkspace(const std::string &workspaceName, const FunctionModelSpectra &spectra) {
  auto ws = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>(workspaceName);
  addWorkspace(ws, spectra);
}

void IndirectFitDataModel::addWorkspace(Mantid::API::MatrixWorkspace_sptr workspace,
                                        const FunctionModelSpectra &spectra) {
  if (!m_fittingData->empty()) {
    for (auto i : *m_fittingData) {
      if (equivalentWorkspaces(workspace, i.workspace())) {
        i.combine(IndirectFitData(workspace, spectra));
        return;
      }
    }
  }
  addNewWorkspace(workspace, spectra);
}

FitDomainIndex IndirectFitDataModel::getDomainIndex(TableDatasetIndex dataIndex, WorkspaceIndex spectrum) const {
  FitDomainIndex index{0};
  for (size_t iws = 0; iws < m_fittingData->size(); ++iws) {
    if (iws < dataIndex.value) {
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

std::pair<double, double> IndirectFitDataModel::getFittingRange(TableDatasetIndex dataIndex,
                                                                WorkspaceIndex spectrum) const {
  if (dataIndex.value < m_fittingData->size() && !m_fittingData->at(dataIndex.value).zeroSpectra()) {
    return m_fittingData->at(dataIndex.value).getRange(spectrum);
  }
  return std::make_pair(0., 0.);
}

std::string IndirectFitDataModel::getExcludeRegion(TableDatasetIndex dataIndex, WorkspaceIndex spectrum) const {
  if (dataIndex.value < m_fittingData->size() && !m_fittingData->at(dataIndex.value).zeroSpectra()) {
    return m_fittingData->at(dataIndex.value).getExcludeRegion(spectrum);
  }
  return "";
}

void IndirectFitDataModel::setStartX(double startX, TableDatasetIndex dataIndex, WorkspaceIndex spectrum) {
  if (m_fittingData->empty())
    return;
  m_fittingData->at(dataIndex.value).setStartX(startX, spectrum);
}

void IndirectFitDataModel::setStartX(double startX, TableDatasetIndex dataIndex) {
  if (m_fittingData->empty())
    return;
  m_fittingData->at(dataIndex.value).setStartX(startX);
}

void IndirectFitDataModel::setEndX(double endX, TableDatasetIndex dataIndex, WorkspaceIndex spectrum) {
  if (m_fittingData->empty())
    return;
  m_fittingData->at(dataIndex.value).setEndX(endX, spectrum);
}

void IndirectFitDataModel::setEndX(double endX, TableDatasetIndex dataIndex) {
  if (m_fittingData->empty())
    return;
  m_fittingData->at(dataIndex.value).setEndX(endX);
}

void IndirectFitDataModel::setExcludeRegion(const std::string &exclude, TableDatasetIndex dataIndex,
                                            WorkspaceIndex spectrum) {
  if (m_fittingData->empty())
    return;
  m_fittingData->at(dataIndex.value).setExcludeRegionString(exclude, spectrum);
}

void IndirectFitDataModel::addNewWorkspace(const Mantid::API::MatrixWorkspace_sptr &workspace,
                                           const FunctionModelSpectra &spectra) {
  m_fittingData->emplace_back(workspace, spectra);
}

void IndirectFitDataModel::removeWorkspace(TableDatasetIndex index) {
  if (index < m_fittingData->size()) {
    m_fittingData->erase(m_fittingData->begin() + index.value);
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

void IndirectFitDataModel::switchToSingleInputMode() {
  m_fittingData = m_fittingDataSingle.get();
  m_resolutions = m_resolutionsSingle.get();
}

void IndirectFitDataModel::switchToMultipleInputMode() {
  m_fittingData = m_fittingDataMultiple.get();
  m_resolutions = m_resolutionsMultiple.get();
}

std::vector<double> IndirectFitDataModel::getExcludeRegionVector(TableDatasetIndex dataIndex,
                                                                 WorkspaceIndex index) const {
  auto fitData = m_fittingData->at(dataIndex.value);
  return fitData.excludeRegionsVector(index);
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
  auto subIndices = getSubIndices(index);
  m_fittingData->at(subIndices.first.value).setExcludeRegionString(exclude, subIndices.second);
}

std::pair<TableDatasetIndex, WorkspaceIndex> IndirectFitDataModel::getSubIndices(FitDomainIndex index) const {
  size_t sum{0};
  for (size_t datasetIndex = 0; datasetIndex < m_fittingData->size(); datasetIndex++) {
    for (size_t workspaceIndex = 0; workspaceIndex < m_fittingData->at(datasetIndex).spectra().size().value;
         workspaceIndex++) {
      if (sum == index.value) {
        WorkspaceIndex spectraIndex = m_fittingData->at(datasetIndex).spectra()[FitDomainIndex{workspaceIndex}];
        return std::make_pair(TableDatasetIndex{datasetIndex}, spectraIndex);
      }
      sum++;
    }
  }
  throw std::runtime_error("Failed to find workspace and spectrum index for fit domain.");
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
