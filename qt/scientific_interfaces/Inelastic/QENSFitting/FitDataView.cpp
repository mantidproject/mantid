// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FitDataView.h"
#include "FitDataPresenter.h"
#include "MantidQtWidgets/Common/AddWorkspaceDialog.h"
#include "MantidQtWidgets/Common/IndexTypes.h"
#include "MantidQtWidgets/Common/TableWidgetValidators.h"
#include "MantidQtWidgets/Spectroscopy/InterfaceUtils.h"
#include "MantidQtWidgets/Spectroscopy/SettingsWidget/SettingsHelper.h"

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

constexpr auto NUMERICAL_PRECISION = 6;
const std::string MASK_LIST = getRegexValidatorString(RegexValidatorStrings::MaskValidator);
namespace {
QStringList defaultHeaders() {
  QStringList headers;
  headers << "Workspace"
          << "WS Index"
          << "StartX"
          << "EndX"
          << "Mask X Range";
  return headers;
}
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {

FitDataView::FitDataView(QWidget *parent) : FitDataView(defaultHeaders(), parent) {}

FitDataView::FitDataView(const QStringList &headers, QWidget *parent)
    : QTabWidget(parent), m_uiForm(new Ui::FitDataView) {
  m_uiForm->setupUi(this);

  setHorizontalHeaders(headers);

  connect(m_uiForm->pbAdd, SIGNAL(clicked()), this, SLOT(showAddWorkspaceDialog()));
  connect(m_uiForm->pbRemove, SIGNAL(clicked()), this, SLOT(notifyRemoveClicked()));
  connect(m_uiForm->pbUnify, SIGNAL(clicked()), this, SLOT(notifyUnifyClicked()));
  connect(m_uiForm->tbFitData, SIGNAL(cellChanged(int, int)), this, SLOT(notifyCellChanged(int, int)));
}

void FitDataView::subscribePresenter(IFitDataPresenter *presenter) { m_presenter = presenter; }

QTableWidget *FitDataView::getDataTable() const { return m_uiForm->tbFitData; }

void FitDataView::setHorizontalHeaders(const QStringList &headers) {
  m_uiForm->tbFitData->setColumnCount(headers.size());
  m_uiForm->tbFitData->setHorizontalHeaderLabels(headers);
  m_HeaderLabels = headers;

  auto header = m_uiForm->tbFitData->horizontalHeader();
  header->setSectionResizeMode(0, QHeaderView::Stretch);

  m_uiForm->tbFitData->setItemDelegateForColumn(getColumnIndexFromName("StartX"),
                                                new NumericInputDelegate(m_uiForm->tbFitData, NUMERICAL_PRECISION));
  m_uiForm->tbFitData->setItemDelegateForColumn(getColumnIndexFromName("EndX"),
                                                new NumericInputDelegate(m_uiForm->tbFitData, NUMERICAL_PRECISION));
  m_uiForm->tbFitData->setItemDelegateForColumn(getColumnIndexFromName("Mask X Range"),
                                                new RegexInputDelegate(m_uiForm->tbFitData, MASK_LIST));

  m_uiForm->tbFitData->verticalHeader()->setVisible(false);
}

void FitDataView::validate(IUserInputValidator *validator) {
  if (m_uiForm->tbFitData->rowCount() == 0)
    validator->addErrorMessage("No input data has been provided.");
}

void FitDataView::displayWarning(const std::string &warning) {
  QMessageBox::warning(parentWidget(), "MantidPlot - Warning", QString::fromStdString(warning));
}

void FitDataView::addTableEntry(size_t row, FitDataRow const &newRow) {
  m_uiForm->tbFitData->insertRow(static_cast<int>(row));

  auto cell = std::make_unique<QTableWidgetItem>(QString::fromStdString(newRow.name));
  auto flags = cell->flags();
  flags ^= Qt::ItemIsEditable;
  cell->setFlags(flags);
  setCell(std::move(cell), row, 0);

  cell = std::make_unique<QTableWidgetItem>(QString::number(newRow.workspaceIndex));
  cell->setFlags(flags);
  setCell(std::move(cell), row, getColumnIndexFromName("WS Index"));

  cell = std::make_unique<QTableWidgetItem>(makeQStringNumber(newRow.startX, NUMERICAL_PRECISION));
  setCell(std::move(cell), row, getColumnIndexFromName("StartX"));

  cell = std::make_unique<QTableWidgetItem>(makeQStringNumber(newRow.endX, NUMERICAL_PRECISION));
  setCell(std::move(cell), row, getColumnIndexFromName("EndX"));

  cell = std::make_unique<QTableWidgetItem>(QString::fromStdString(newRow.exclude));
  setCell(std::move(cell), row, getColumnIndexFromName("Mask X Range"));
}

void FitDataView::updateNumCellEntry(double numEntry, size_t row, size_t column) {
  QTableWidgetItem *selectedItem;
  selectedItem = m_uiForm->tbFitData->item(static_cast<int>(row), static_cast<int>(column));
  selectedItem->setText(makeQStringNumber(numEntry, NUMERICAL_PRECISION));
}

bool FitDataView::isTableEmpty() const { return m_uiForm->tbFitData->rowCount() == 0; }

int FitDataView::getColumnIndexFromName(std::string const &ColName) {
  return m_HeaderLabels.indexOf(QString::fromStdString(ColName));
}

void FitDataView::clearTable() { m_uiForm->tbFitData->setRowCount(0); }

void FitDataView::setCell(std::unique_ptr<QTableWidgetItem> cell, size_t row, size_t column) {
  m_uiForm->tbFitData->setItem(static_cast<int>(row), static_cast<int>(column), cell.release());
}

bool FitDataView::dataColumnContainsText(std::string const &columnText) const {
  return !m_uiForm->tbFitData->findItems(QString::fromStdString(columnText), Qt::MatchContains).isEmpty();
}

QString FitDataView::getText(int row, int column) const {
  return m_uiForm->tbFitData->item(static_cast<int>(row), column)->text();
}

QModelIndexList FitDataView::getSelectedIndexes() const {
  return m_uiForm->tbFitData->selectionModel()->selectedIndexes();
}

void FitDataView::showAddWorkspaceDialog() {
  auto dialog = new MantidWidgets::AddWorkspaceDialog(parentWidget());
  connect(dialog, SIGNAL(addData(MantidWidgets::IAddWorkspaceDialog *)), this,
          SLOT(notifyAddData(MantidWidgets::IAddWorkspaceDialog *)));

  auto tabName = m_presenter->tabName();
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWSSuffices(InterfaceUtils::getSampleWSSuffixes(tabName));
  dialog->setFBSuffices(InterfaceUtils::getSampleFBSuffixes(tabName));
  dialog->setLoadProperty("LoadHistory", SettingsHelper::loadHistory());
  dialog->updateSelectedSpectra();
  dialog->show();
}

void FitDataView::notifyAddData(MantidWidgets::IAddWorkspaceDialog *dialog) { m_presenter->handleAddData(dialog); }

void FitDataView::notifyRemoveClicked() { m_presenter->handleRemoveClicked(); }

void FitDataView::notifyUnifyClicked() { m_presenter->handleUnifyClicked(); }

void FitDataView::notifyCellChanged(int row, int column) { m_presenter->handleCellChanged(row, column); }

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
