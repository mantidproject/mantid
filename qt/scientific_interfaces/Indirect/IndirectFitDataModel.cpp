// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include <string>

#include "DllConfig.h"
#include "IndexTypes.h"
#include "IndirectFitdata.h"
#include "IndirectFitdataModel.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"

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

IndirectFitDataModel::IndirectFitDataModel() : m_fittingData() {}

bool IndirectFitDataModel::hasWorkspace(
    std::string const &workspaceName) const {
  auto const names = getWorkspaceNames();
  auto const iter = std::find(names.begin(), names.end(), workspaceName);
  return iter != names.end();
  return true;
}

Mantid::API::MatrixWorkspace_sptr
IndirectFitDataModel::getWorkspace(TableDatasetIndex index) const {
  if (index < m_fittingData.size())
    return m_fittingData[index.value].workspace();
  return nullptr;
}

Spectra IndirectFitDataModel::getSpectra(TableDatasetIndex index) const {
  if (index < m_fittingData.size())
    return m_fittingData[index.value].spectra();
  return Spectra("");
}

TableDatasetIndex IndirectFitDataModel::numberOfWorkspaces() const {
  return TableDatasetIndex{static_cast<int>(m_fittingData.size())};
}

bool IndirectFitDataModel::isMultiFit() const {
  return numberOfWorkspaces().value > 1;
}

int IndirectFitDataModel::getNumberOfSpectra(TableDatasetIndex index) const {
  if (index < m_fittingData.size())
    return m_fittingData[index.value].numberOfSpectra().value;
  else
    throw std::runtime_error(
        "Cannot find the number of spectra for a workspace: the workspace "
        "index provided is too large.");
  return 1;
}

int IndirectFitDataModel::getNumberOfDomains() const {
  int sum{0};
  for (auto fittingData : m_fittingData) {
    sum += fittingData.numberOfSpectra().value;
  }
  return sum;
}

std::vector<double> IndirectFitDataModel::getQValuesForData() const {
  std::vector<double> qValues{1};
  for (auto &fitData : m_fittingData) {
    auto indexQValues = fitData.getQValues();
    qValues.insert(std::end(qValues), std::begin(indexQValues),
                   std::end(indexQValues));
  }
  return qValues;
}

std::vector<std::pair<std::string, int>>
IndirectFitDataModel::getResolutionsForFit() const {
  return std::vector<std::pair<std::string, int>>();
}

void IndirectFitDataModel::setSpectra(const std::string &spectra,
                                      TableDatasetIndex dataIndex) {
  setSpectra(Spectra(spectra), dataIndex);
}

void IndirectFitDataModel::setSpectra(Spectra &&spectra,
                                      TableDatasetIndex dataIndex) {
  if (m_fittingData.empty())
    return;
  m_fittingData[dataIndex.value].setSpectra(std::forward<Spectra>(spectra));
}

void IndirectFitDataModel::setSpectra(const Spectra &spectra,
                                      TableDatasetIndex dataIndex) {
  if (m_fittingData.empty())
    return;
  m_fittingData[dataIndex.value].setSpectra(spectra);
}

std::vector<std::string> IndirectFitDataModel::getWorkspaceNames() const {
  std::vector<std::string> names;
  names.reserve(m_fittingData.size());
  for (auto fittingData : m_fittingData)
    names.emplace_back(fittingData.workspace()->getName());
  return names;
}

void IndirectFitDataModel::addWorkspace(const std::string &workspaceName) {
  auto ws = Mantid::API::AnalysisDataService::Instance()
                .retrieveWS<Mantid::API::MatrixWorkspace>(workspaceName);
  addWorkspace(
      ws,
      Spectra(WorkspaceIndex{0},
              WorkspaceIndex{static_cast<int>(ws->getNumberHistograms()) - 1}));
}

void IndirectFitDataModel::addWorkspace(const std::string &workspaceName,
                                        const std::string &spectra) {
  if (spectra.empty())
    throw std::runtime_error(
        "Fitting Data must consist of one or more spectra.");
  if (workspaceName.empty() ||
      !Mantid::API::AnalysisDataService::Instance().doesExist(workspaceName))
    throw std::runtime_error("A valid sample file needs to be selected.");

  try {
    addWorkspace(workspaceName, Spectra(spectra));
  } catch (std::logic_error &e) {
    throw std::runtime_error(e.what());
  }
}

void IndirectFitDataModel::addWorkspace(const std::string &workspaceName,
                                        const Spectra &spectra) {
  auto ws = Mantid::API::AnalysisDataService::Instance()
                .retrieveWS<Mantid::API::MatrixWorkspace>(workspaceName);
  addWorkspace(ws, spectra);
}

void IndirectFitDataModel::addWorkspace(
    Mantid::API ::MatrixWorkspace_sptr workspace, const Spectra &spectra) {
  if (!m_fittingData.empty() &&
      equivalentWorkspaces(workspace, m_fittingData.back().workspace()))
    m_fittingData.back().combine(IndirectFitData(workspace, spectra));
  else
    addNewWorkspace(workspace, spectra);
}

FitDomainIndex
IndirectFitDataModel::getDomainIndex(TableDatasetIndex dataIndex,
                                     WorkspaceIndex spectrum) const {
  FitDomainIndex index{0};
  for (int iws = 0; iws < m_fittingData.size(); ++iws) {
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

void IndirectFitDataModel::clear() { m_fittingData.clear(); }

std::pair<double, double>
IndirectFitDataModel::getFittingRange(TableDatasetIndex dataIndex,
                                      WorkspaceIndex spectrum) const {
  if (dataIndex.value < m_fittingData.size() &&
      !m_fittingData[dataIndex.value].zeroSpectra()) {
    return m_fittingData[dataIndex.value].getRange(spectrum);
  }
  return std::make_pair(0., 0.);
}

std::string
IndirectFitDataModel::getExcludeRegion(TableDatasetIndex dataIndex,
                                       WorkspaceIndex spectrum) const {
  if (dataIndex.value < m_fittingData.size() &&
      !m_fittingData[dataIndex.value].zeroSpectra()) {
    return m_fittingData[dataIndex.value].getExcludeRegion(spectrum);
  }
  return "";
}

void IndirectFitDataModel::setStartX(double startX, TableDatasetIndex dataIndex,
                                     WorkspaceIndex spectrum) {
  if (m_fittingData.empty())
    return;
  m_fittingData[dataIndex.value].setStartX(startX, spectrum);
}

void IndirectFitDataModel::setStartX(double startX,
                                     TableDatasetIndex dataIndex) {
  if (m_fittingData.empty())
    return;
  m_fittingData[dataIndex.value].setStartX(startX);
}

void IndirectFitDataModel::setEndX(double endX, TableDatasetIndex dataIndex,
                                   WorkspaceIndex spectrum) {
  if (m_fittingData.empty())
    return;
  m_fittingData[dataIndex.value].setEndX(endX, spectrum);
}

void IndirectFitDataModel::setEndX(double endX, TableDatasetIndex dataIndex) {
  if (m_fittingData.empty())
    return;
  m_fittingData[dataIndex.value].setEndX(endX);
}

void IndirectFitDataModel::setExcludeRegion(const std::string &exclude,
                                            TableDatasetIndex dataIndex,
                                            WorkspaceIndex spectrum) {
  if (m_fittingData.empty())
    return;
  m_fittingData[dataIndex.value].setExcludeRegionString(exclude, spectrum);
}

void IndirectFitDataModel::addNewWorkspace(
    const Mantid::API::MatrixWorkspace_sptr &workspace,
    const Spectra &spectra) {
  m_fittingData.emplace_back(IndirectFitData(std::move(workspace), spectra));
}

void IndirectFitDataModel::removeWorkspace(TableDatasetIndex index) {}

std::vector<double>
IndirectFitDataModel::getExcludeRegionVector(TableDatasetIndex dataIndex,
                                             WorkspaceIndex index) const {
  auto fitData = m_fittingData[dataIndex.value];
  return fitData.excludeRegionsVector(index);
}

Mantid::API::MatrixWorkspace_sptr
IndirectFitDataModel::getWorkspace(FitDomainIndex index) const {
  auto subIndices = getSubIndices(index);
  return getWorkspace(subIndices.first);
};

std::pair<double, double>
IndirectFitDataModel::getFittingRange(FitDomainIndex index) const {
  auto subIndices = getSubIndices(index);
  return getFittingRange(subIndices.first, subIndices.second);
};

int IndirectFitDataModel::getSpectrum(FitDomainIndex index) const {
  auto subIndices = getSubIndices(index);
  return subIndices.second.value;
};

std::vector<double>
IndirectFitDataModel::getExcludeRegionVector(FitDomainIndex index) const {
  auto subIndices = getSubIndices(index);
  return getExcludeRegionVector(subIndices.first, subIndices.second);
};

std::string IndirectFitDataModel::getExcludeRegion(FitDomainIndex index) const {
  auto subIndices = getSubIndices(index);
  return getExcludeRegion(subIndices.first, subIndices.second);
}

std::pair<TableDatasetIndex, WorkspaceIndex>
IndirectFitDataModel::getSubIndices(FitDomainIndex index) const {
  int sum{0};
  for (int datasetIndex = 0; datasetIndex < m_fittingData.size();
       datasetIndex++) {
    for (int workspaceIndex = 0;
         workspaceIndex < m_fittingData[datasetIndex].spectra().size().value;
         workspaceIndex++) {
      if (sum == index.value) {
        WorkspaceIndex spectraIndex =
            m_fittingData[datasetIndex]
                .spectra()[FitDomainIndex{workspaceIndex}];
        return std::make_pair(TableDatasetIndex{datasetIndex}, spectraIndex);
      }
      sum++;
    }
  }
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt