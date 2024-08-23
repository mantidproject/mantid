// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FitDataPresenter.h"
#include "FitTab.h"

#include <map>
#include <utility>

#include "MantidQtWidgets/Common/AddWorkspaceDialog.h"

namespace MantidQt::CustomInterfaces::Inelastic {

FitDataPresenter::FitDataPresenter(IFitTab *tab, IDataModel *model, IFitDataView *view)
    : m_tab(tab), m_model(model), m_view(view) {
  m_view->subscribePresenter(this);
  observeReplace(true);
}

FitDataPresenter::~FitDataPresenter() { observeReplace(false); }

IFitDataView const *FitDataPresenter::getView() const { return m_view; }

std::string FitDataPresenter::tabName() const { return m_tab->tabName(); }

bool FitDataPresenter::addWorkspaceFromDialog(MantidWidgets::IAddWorkspaceDialog const *dialog) {
  if (const auto indirectDialog = dynamic_cast<MantidWidgets::AddWorkspaceDialog const *>(dialog)) {
    addWorkspace(indirectDialog->workspaceName(), indirectDialog->workspaceIndices());
    return true;
  }
  return false;
}

void FitDataPresenter::addWorkspace(const std::string &workspaceName, const FunctionModelSpectra &workspaceIndices) {
  m_model->addWorkspace(workspaceName, workspaceIndices);
}

void FitDataPresenter::setResolution(const std::string &name) {
  if (m_model->setResolution(name) == false) {
    m_model->removeSpecialValues(name);
    displayWarning("Replaced the NaN's and infinities in " + name + " with zeros");
  }
}

void FitDataPresenter::setStartX(double startX, WorkspaceID workspaceID) {
  if (m_model->getNumberOfWorkspaces() > workspaceID) {
    m_model->setStartX(startX, workspaceID);
  }
}

void FitDataPresenter::setStartX(double startX, WorkspaceID workspaceID, WorkspaceIndex spectrum) {
  if (m_model->getNumberOfWorkspaces() > workspaceID) {
    m_model->setStartX(startX, workspaceID, spectrum);
  }
}

void FitDataPresenter::setEndX(double endX, WorkspaceID workspaceID) {
  if (m_model->getNumberOfWorkspaces() > workspaceID) {
    m_model->setEndX(endX, workspaceID);
  }
}

void FitDataPresenter::setEndX(double endX, WorkspaceID workspaceID, WorkspaceIndex spectrum) {
  if (m_model->getNumberOfWorkspaces() > workspaceID) {
    m_model->setEndX(endX, workspaceID, spectrum);
  }
}

std::vector<std::pair<std::string, size_t>> FitDataPresenter::getResolutionsForFit() const {
  return m_model->getResolutionsForFit();
}

void FitDataPresenter::validate(IUserInputValidator *validator) { m_view->validate(validator); }

void FitDataPresenter::handleAddData(MantidWidgets::IAddWorkspaceDialog const *dialog) {
  try {
    m_tab->handleDataAdded(dialog);
    updateTableFromModel();
    m_tab->handleDataChanged();
  } catch (const std::runtime_error &ex) {
    displayWarning(ex.what());
  } catch (const std::invalid_argument &ex) {
    displayWarning(ex.what());
  }
}

void FitDataPresenter::updateTableFromModel() {
  m_view->clearTable();
  for (auto domainIndex = FitDomainIndex{0}; domainIndex < getNumberOfDomains(); domainIndex++) {
    addTableEntry(domainIndex);
  }
}

WorkspaceID FitDataPresenter::getNumberOfWorkspaces() const { return m_model->getNumberOfWorkspaces(); }

size_t FitDataPresenter::getNumberOfDomains() const { return m_model->getNumberOfDomains(); }

QList<FunctionModelDataset> FitDataPresenter::getDatasets() const {
  QList<FunctionModelDataset> datasets;

  for (auto i = 0u; i < m_model->getNumberOfWorkspaces().value; ++i) {
    WorkspaceID workspaceID{i};
    datasets.append(m_model->getDataset(workspaceID));
  }
  return datasets;
}

DataForParameterEstimationCollection
FitDataPresenter::getDataForParameterEstimation(const EstimationDataSelector &selector) const {
  DataForParameterEstimationCollection dataCollection;
  for (auto i = WorkspaceID{0}; i < m_model->getNumberOfWorkspaces(); ++i) {
    auto const ws = m_model->getWorkspace(i);
    for (const auto &spectrum : m_model->getSpectra(i)) {
      auto const &x = ws->readX(spectrum.value);
      auto const &y = ws->readY(spectrum.value);
      auto range = m_model->getFittingRange(i, spectrum);
      dataCollection.emplace_back(selector(x, y, range));
    }
  }
  return dataCollection;
}

std::vector<double> FitDataPresenter::getQValuesForData() const { return m_model->getQValuesForData(); }

void FitDataPresenter::displayWarning(const std::string &warning) { m_view->displayWarning(warning); }

void FitDataPresenter::addTableEntry(FitDomainIndex row) {
  const auto &name = m_model->getWorkspace(row)->getName();
  const auto workspaceIndex = m_model->getSpectrum(row);
  const auto range = m_model->getFittingRange(row);
  const auto exclude = m_model->getExcludeRegion(row);

  FitDataRow newRow;
  newRow.name = name;
  newRow.workspaceIndex = workspaceIndex;
  newRow.startX = range.first;
  newRow.endX = range.second;
  newRow.exclude = exclude;

  m_view->addTableEntry(row.value, newRow);
}

void FitDataPresenter::handleCellChanged(int row, int column) {
  if (m_view->getColumnIndexFromName("StartX") == column) {
    setTableStartXAndEmit(m_view->getText(row, column).toDouble(), row, column);
  } else if (m_view->getColumnIndexFromName("EndX") == column) {
    setTableEndXAndEmit(m_view->getText(row, column).toDouble(), row, column);
  } else if (m_view->getColumnIndexFromName("Mask X Range") == column) {
    setModelExcludeAndEmit(m_view->getText(row, column).toStdString(), row);
  }
}

void FitDataPresenter::setTableStartXAndEmit(double X, int row, int column) {
  auto subIndices = m_model->getSubIndices(row);

  m_model->setStartX(X, subIndices.first, subIndices.second);
  m_view->updateNumCellEntry(m_model->getFittingRange(row).first, row, column);
  m_tab->handleTableStartXChanged(m_model->getFittingRange(row).first, subIndices.first, subIndices.second);
}

void FitDataPresenter::setTableEndXAndEmit(double X, int row, int column) {
  auto subIndices = m_model->getSubIndices(row);

  m_model->setEndX(X, subIndices.first, subIndices.second);
  m_view->updateNumCellEntry(m_model->getFittingRange(row).second, row, column);
  m_tab->handleTableEndXChanged(m_model->getFittingRange(row).second, subIndices.first, subIndices.second);
}

void FitDataPresenter::setModelStartXAndEmit(double startX, FitDomainIndex row) {
  auto subIndices = m_model->getSubIndices(row);

  m_model->setStartX(startX, subIndices.first, subIndices.second);
  m_tab->handleTableStartXChanged(startX, subIndices.first, subIndices.second);
}

void FitDataPresenter::setModelEndXAndEmit(double endX, FitDomainIndex row) {
  auto subIndices = m_model->getSubIndices(row);

  m_model->setEndX(endX, subIndices.first, subIndices.second);
  m_tab->handleTableEndXChanged(endX, subIndices.first, subIndices.second);
}

void FitDataPresenter::setModelExcludeAndEmit(const std::string &exclude, FitDomainIndex row) {
  auto subIndices = m_model->getSubIndices(row);
  m_model->setExcludeRegion(exclude, subIndices.first, subIndices.second);
}

/**
 * Gets a map of selected rows, with no repeats
 * @param selectedIndices:: [input] the list of QModelIndex's
 * @returns map of unique (by row) QModelIndex's
 */
std::map<int, QModelIndex> FitDataPresenter::getUniqueIndices(const QModelIndexList &selectedIndices) {
  // remove repeated rows
  std::map<int, QModelIndex> uniqueIndices;
  for (auto item : selectedIndices) {
    uniqueIndices.emplace(item.row(), item);
  }
  return uniqueIndices;
}

/**
 * Removes selected rows, with no repeats
 */
void FitDataPresenter::handleRemoveClicked() {
  auto selectedIndices = m_view->getSelectedIndexes();
  if (selectedIndices.size() == 0) {
    // check that there are selected indexes.
    return;
  }
  auto uniqueIndices = getUniqueIndices(selectedIndices);
  for (auto item = uniqueIndices.rbegin(); item != uniqueIndices.rend(); ++item) {
    m_model->removeDataByIndex(FitDomainIndex(item->second.row()));
  }
  updateTableFromModel();
  m_tab->handleDataRemoved();
  m_tab->handleDataChanged();
}

void FitDataPresenter::handleUnifyClicked() {
  auto selectedIndices = m_view->getSelectedIndexes();
  if (selectedIndices.size() == 0) {
    // check that there are selected indexes.
    return;
  }
  std::sort(selectedIndices.begin(), selectedIndices.end());
  auto fitRange = m_model->getFittingRange(FitDomainIndex(selectedIndices.begin()->row()));
  for (auto item = selectedIndices.end(); item != selectedIndices.begin();) {
    --item;
    setModelStartXAndEmit(fitRange.first, FitDomainIndex(item->row()));
    setModelEndXAndEmit(fitRange.second, FitDomainIndex(item->row()));
  }
  updateTableFromModel();
}

std::vector<std::string> FitDataPresenter::createDisplayNames() const {
  std::vector<std::string> displayNames;
  for (auto i = WorkspaceID(0); i < m_model->getNumberOfWorkspaces(); i++) {
    displayNames.push_back(m_model->createDisplayName(i));
  }
  return displayNames;
}

} // namespace MantidQt::CustomInterfaces::Inelastic
