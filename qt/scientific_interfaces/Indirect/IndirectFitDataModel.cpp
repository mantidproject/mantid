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
bool IndirectFitDataModel::hasWorkspace(
    std::string const &workspaceName) const {
  auto const names = getWorkspaceNames();
  auto const iter = std::find(names.begin(), names.end(), workspaceName);
  return iter != names.end();
}

Mantid::API::MatrixWorkspace_sptr
IndirectFitDataModel::getWorkspace(TableDatasetIndex index) const {
  if (index < m_fittingData.size())
    return m_fittingData[index]->workspace();
  return nullptr;
}

Spectra IndirectFitDataModel::getSpectra(TableDatasetIndex index) const {
  if (index < m_fittingData.size())
    return m_fittingData[index]->spectra();
  return Spectra("");
}

TableDatasetIndex IndirectFitDataModel::numberOfWorkspaces() const {
  return TableDatasetIndex{m_fittingData.size()};
}

bool IndirectFitDataModel::isMultiFit() const {
  return numberOfWorkspaces().value > 1;
}

int IndirectFitDataModel::getNumberOfSpectra(TableDatasetIndex index) const {
  if (index < m_fittingData.size())
    return m_fittingData[index]->numberOfSpectra().value;
  else
    throw std::runtime_error(
        "Cannot find the number of spectra for a workspace: the workspace "
        "index provided is too large.");
}

int IndirectFitDataModel::getNumberOfDomains() const {
  int sum{0};
  for (TableDatasetIndex i = m_fittingData.zero(); i < m_fittingData.size();
       ++i) {
    sum += m_fittingData[i]->numberOfSpectra().value;
  }
  return sum;
}

std::vector<double> IndirectFitDataModel::getQValuesForData() const {
  std::vector<double> qValues;
  for (auto &fitData : m_fittingData) {
    auto indexQValues = fitData->getQValues();
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
  m_fittingData[dataIndex]->setSpectra(std::forward<Spectra>(spectra));
}

void IndirectFitDataModel::setSpectra(const Spectra &spectra,
                                      TableDatasetIndex dataIndex) {
  if (m_fittingData.empty())
    return;
  m_fittingData[dataIndex]->setSpectra(spectra);
}

std::vector<std::string> IndirectFitDataModel::getWorkspaceNames() const {
  std::vector<std::string> names;
  names.reserve(m_fittingData.size().value);
  for (TableDatasetIndex i = m_fittingData.zero(); i < m_fittingData.size();
       ++i)
    names.emplace_back(m_fittingData[i]->workspace()->getName());
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
      equivalentWorkspaces(workspace, m_fittingData.back()->workspace()))
    m_fittingData.back()->combine(IndirectFitData(workspace, spectra));
  else
    addNewWorkspace(workspace, spectra);
}

void IndirectFitDataModel::addNewWorkspace(
    const Mantid::API::MatrixWorkspace_sptr &workspace,
    const Spectra &spectra) {
  m_fittingData.emplace_back(
      new IndirectFitData(std::move(workspace), spectra));
}

void IndirectFitDataModel::removeWorkspace(TableDatasetIndex index) {}

PrivateFittingData::PrivateFittingData() : m_data() {}

PrivateFittingData::PrivateFittingData(PrivateFittingData &&privateData)
    : m_data(std::move(privateData.m_data)) {}

PrivateFittingData::PrivateFittingData(IndirectFitDataCollectionType &&data)
    : m_data(std::move(data)) {}

PrivateFittingData &
PrivateFittingData::operator=(PrivateFittingData &&fittingData) {
  m_data = std::move(fittingData.m_data);
  return *this;
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt