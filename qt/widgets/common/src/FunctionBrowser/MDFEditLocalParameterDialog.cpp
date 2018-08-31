#include "MDFEditLocalParameterDialog.h"
#include "MultiDatasetFit/MDFLocalParameterItemDelegate.h"

#include "MantidKernel/make_unique.h"

#include <boost/optional.hpp>

#include <QApplication>
#include <QClipboard>

namespace {
static constexpr int VALUE_COLUMN = 0;
static constexpr int ROLE_COLUMN = 1;

QString makeNumber(double d) { return QString::number(d, 'g', 16); }

std::unique_ptr<QTableWidgetItem> createCell(QString const &value) {
  return Mantid::Kernel::make_unique<QTableWidgetItem>(value);
}

std::unique_ptr<QTableWidgetItem> createRoleCell(QString const &value) {
  auto cell = createCell(value);
  auto flags = cell->flags();
  flags ^= Qt::ItemIsEditable;
  flags ^= Qt::ItemIsSelectable;
  flags ^= Qt::ItemIsEnabled;
  cell->setFlags(flags);
  return std::move(cell);
}

boost::optional<double> toDouble(QString const &text) {
  bool ok = false;
  double value = text.toDouble(&ok);
  return ok ? value : boost::optional<double>();
}
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace QENS {

EditLocalParameterDialog::EditLocalParameterDialog()
    : EditLocalParameterDialog(nullptr) {}

EditLocalParameterDialog::EditLocalParameterDialog(QWidget *parent)
    : QDialog(parent) {
  m_uiForm.setupUi(this);
  m_uiForm.logValueSelector->setCheckboxShown(true);

  auto deleg = new MDF::LocalParameterItemDelegate(this);
  m_uiForm.tableWidget->setItemDelegateForColumn(VALUE_COLUMN, deleg);

  auto header = m_uiForm.tableWidget->horizontalHeader();
  header->setResizeMode(0, QHeaderView::Stretch);

  connect(deleg, SIGNAL(setAllValues(double)), this,
          SLOT(valuesChanged(double)));
  connect(deleg, SIGNAL(fixParameter(int, bool)), this,
          SLOT(fixChanged(int, bool)));
  connect(deleg, SIGNAL(setAllFixed(bool)), this, SLOT(fixChanged(bool)));
  connect(deleg, SIGNAL(setTie(int, QString)), this,
          SLOT(tieChanged(int, QString const &)));
  connect(deleg, SIGNAL(setTieAll(QString)), this,
          SLOT(tieChanged(QString const &)));
  connect(deleg, SIGNAL(setValueToLog(int)), this, SLOT(logValueChanged(int)));
  connect(deleg, SIGNAL(setAllValuesToLog()), this, SLOT(logValueChanged()));

  m_uiForm.tableWidget->installEventFilter(this);
}

void EditLocalParameterDialog::subscribe(
    MDF::EditLocalParameterDialogSubscriber *subscriber) {
  m_subscriber = subscriber;
}

void EditLocalParameterDialog::setParameterNameTitle(std::string const &name) {
  m_uiForm.lblParameterName->setText("Parameter: " +
                                     QString::fromStdString(name));
}

void EditLocalParameterDialog::addFixedParameter(std::string const &datasetName,
                                                 double value) {
  auto const row =
      addRowToTable(QString::fromStdString(datasetName), makeNumber(value));
  setParameterToFixed(row);
}

void EditLocalParameterDialog::addTiedParameter(std::string const &datasetName,
                                                double value,
                                                std::string const &expression) {
  auto const row =
      addRowToTable(QString::fromStdString(datasetName), makeNumber(value));
  setParameterToTied(row);
  setTie(expression, row);
}

void EditLocalParameterDialog::addFittedParameter(
    std::string const &datasetName, double value) {
  auto const row =
      addRowToTable(QString::fromStdString(datasetName), makeNumber(value));
  setParameterToFitted(row);
}

void EditLocalParameterDialog::setParameterToFixed(int index) {
  setRoleItemAt("fixed", QBrush(Qt::red), index);
}

void EditLocalParameterDialog::setParameterToTied(int index) {
  setRoleItemAt("tied", QBrush(Qt::blue), index);
}

void EditLocalParameterDialog::setParameterToFitted(int index) {
  setRoleItemAt("fitted", QBrush(Qt::darkGreen), index);
}

void EditLocalParameterDialog::addLogsToMenu(
    std::vector<std::string> const &logNames) {
  auto const logComboBox = m_uiForm.logValueSelector->getLogComboBox();
  for (auto const &logName : logNames)
    logComboBox->addItem(QString::fromStdString(logName));
}

void EditLocalParameterDialog::clearLogsInMenu() {
  m_uiForm.logValueSelector->getLogComboBox()->clear();
}

void EditLocalParameterDialog::setParameterValues(double value) {
  for (int i = 0; i < m_uiForm.tableWidget->rowCount(); ++i)
    setParameterValue(value, i);
}

void EditLocalParameterDialog::setTies(std::string const &tie) {
  for (int i = 0; i < m_uiForm.tableWidget->rowCount(); ++i)
    setTie(tie, i);
}

void EditLocalParameterDialog::setParameterValue(double value, int index) {
  setValueItemAt(makeNumber(value), index);
}

void EditLocalParameterDialog::setTie(std::string const &tie, int index) {
  setValueItemAt(QString::fromStdString(tie), index);
}

void EditLocalParameterDialog::copyToClipboard(std::string const &text) {
  QApplication::clipboard()->setText(QString::fromStdString(text));
}

void EditLocalParameterDialog::cellChanged(int row, int column) {
  if (VALUE_COLUMN == column) {
    auto const value = getValueAt(row);
    if (auto const numericValue = toDouble(value))
      m_subscriber->setParameter(*numericValue, row);
    else
      m_subscriber->setTies(value.toStdString());
  }
}

void EditLocalParameterDialog::valuesChanged(double value) {
  m_subscriber->setParameters(value);
}

void EditLocalParameterDialog::fixChanged(bool fixed) {
  m_subscriber->setFixed(fixed);
}

void EditLocalParameterDialog::tieChanged(QString const &tie) {
  m_subscriber->setTies(tie.toStdString());
}

void EditLocalParameterDialog::fixChanged(int index, bool fixed) {
  m_subscriber->fixParameter(fixed, index);
}

void EditLocalParameterDialog::tieChanged(int index, QString const &tie) {
  m_subscriber->setTie(tie.toStdString(), index);
}

void EditLocalParameterDialog::copyClicked() {
  m_subscriber->copyValuesToClipboard();
}

void EditLocalParameterDialog::pasteClicked(std::string const &text) {
  m_subscriber->pasteValuesFromClipboard(text);
}

void EditLocalParameterDialog::logValueChanged(int index) {
  auto const name = m_uiForm.logValueSelector->getLog();
  auto const mode = m_uiForm.logValueSelector->getFunctionText();
  m_subscriber->setValueToLog(name.toStdString(), mode.toStdString(), index);
}

void EditLocalParameterDialog::logValueChanged() {
  auto const name = m_uiForm.logValueSelector->getLog();
  auto const mode = m_uiForm.logValueSelector->getFunctionText();
  m_subscriber->setValuesToLog(name.toStdString(), mode.toStdString());
}

QString EditLocalParameterDialog::getValueAt(int row) const {
  return m_uiForm.tableWidget->item(row, VALUE_COLUMN)->text();
}

int EditLocalParameterDialog::addRowToTable(QString const &datasetName,
                                            QString const &value) {
  auto const row = m_uiForm.tableWidget->rowCount();
  addRowToTable(datasetName, value, row);
  return row;
}

void EditLocalParameterDialog::addRowToTable(QString const &datasetName,
                                             QString const &value, int row) {
  m_uiForm.tableWidget->insertRow(row);
  m_uiForm.tableWidget->setVerticalHeaderItem(
      row, createCell(datasetName).release());
  m_uiForm.tableWidget->setItem(row, VALUE_COLUMN, createCell(value).release());
  m_uiForm.tableWidget->setItem(row, ROLE_COLUMN,
                                createRoleCell(value).release());
}

QTableWidgetItem *EditLocalParameterDialog::getRoleItemAt(int row) {
  return m_uiForm.tableWidget->item(static_cast<int>(row), ROLE_COLUMN);
}

QTableWidgetItem *EditLocalParameterDialog::getValueItemAt(int row) {
  return m_uiForm.tableWidget->item(static_cast<int>(row), VALUE_COLUMN);
}

void EditLocalParameterDialog::setRoleItemAt(QString const &value,
                                             QBrush const &foreground,
                                             int row) {
  auto cell = getRoleItemAt(row);
  cell->setText(value);
  cell->setForeground(foreground);
}

void EditLocalParameterDialog::setValueItemAt(QString const &value, int row) {
  getValueItemAt(row)->setText(value);
}

} // namespace QENS
} // namespace CustomInterfaces
} // namespace MantidQt
