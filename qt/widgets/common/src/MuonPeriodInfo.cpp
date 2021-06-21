// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/MuonPeriodInfo.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/Logger.h"

#include <QString>

namespace MantidQt {
namespace MantidWidgets {

namespace {
const std::string PERIOD_INFO_NOT_FOUND = "Not found";
const std::string NOT_DAQ_STRING = "-";
const std::string DAQ = "1";
const std::string DWELL = "2";
const std::string CYCLES_NOT_FOUND = "Number of period cycles not found";
const std::string DEFAULT_TITLE = "Period Information for Run(s) ";
const QStringList HEADERS{"Period Name"};
const QString HEADER_STYLE = "QHeaderView { font-weight: bold; }";
} // namespace

using namespace Mantid::Kernel;
using namespace Mantid::API;

/**
 * Reads a sample log from a workspace
 * @param ws :: The workspace to read the sample log from
 * @param logName :: The name of the sample log to read
 * @param interfaceName :: The name of the interface this function is being used in (used for the logger if sample log
 * cannot be read)
 * @returns :: String value of the sample log
 */
std::string MuonPeriodInfo::readSampleLog(MatrixWorkspace_const_sptr ws, const std::string &logName,
                                          const std::string &interfaceName) {
  try {
    return ws->run().getLogData(logName)->value();
  } catch (std::exception e) {
    Logger(interfaceName).warning("Workspace does not contain " + logName);
    return "";
  }
}

/**
 * Parses a string separated by a delimeter
 * @param log :: Sample log string to be parsed
 * @param delim :: Delimeter to be used when splitting the sample log
 * @returns :: Vector of string values from the sample log string
 */
std::vector<std::string> MuonPeriodInfo::parseSampleLog(const std::string &log, const std::string &delim) {
  std::vector<std::string> values;
  boost::split(values, log, boost::is_any_of(delim));
  return values;
}

MuonPeriodInfo::MuonPeriodInfo(QWidget *parent) : m_DAQCount(0), m_numberOfSequences(0) {
  m_uiForm.setupUi(this);

  // Set default label text and default title and set up the table columns
  m_uiForm.label->setText(QString::fromStdString(CYCLES_NOT_FOUND));
  setWidgetTitleRuns("");
  setUpTable();
}

void MuonPeriodInfo::addPeriodToTable(const std::string &name) {
  const auto row = m_uiForm.table->rowCount();
  m_uiForm.table->insertRow(row);
  m_uiForm.table->setItem(row, 0, createNewItem(name));
}

void MuonPeriodInfo::setWidgetTitleRuns(const std::string &title) {
  this->setWindowTitle(QString::fromStdString(DEFAULT_TITLE + title));
}

/**
 * Set's the numberOfSequences and updates the label of the widget
 * @param numberOfSequences :: The new string value for the number of sequences
 */
void MuonPeriodInfo::setNumberOfSequences(const int numberOfSequences) {
  m_numberOfSequences = numberOfSequences;
  m_uiForm.label->setText(
      QString::fromStdString("Run contains " + std::to_string(numberOfSequences) + " cycles of periods"));
}
int MuonPeriodInfo::getNumberOfSequences() const { return m_numberOfSequences; }
int MuonPeriodInfo::getDAQCount() const { return m_DAQCount; }

/**
 * Clears all period information by removing all rows from the table and resetting the label
 */
void MuonPeriodInfo::clear() {
  m_DAQCount = 0;
  m_numberOfSequences = 0;
  for (int i = m_uiForm.table->rowCount(); i >= 0; --i)
    m_uiForm.table->removeRow(i);
}

void MuonPeriodInfo::setUpTable() {
  m_uiForm.table->setColumnCount(1);
  m_uiForm.table->setHorizontalHeaderLabels(QStringList(HEADERS));
  m_uiForm.table->horizontalHeader()->setStyleSheet(HEADER_STYLE);
  m_uiForm.table->verticalHeader()->setVisible(false);
}

QTableWidgetItem *MuonPeriodInfo::createNewItem(const std::string &value) const {
  auto item = new QTableWidgetItem();
  item->setText(QString::fromStdString(value));
  item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  return item;
}

} // namespace MantidWidgets
} // namespace MantidQt
