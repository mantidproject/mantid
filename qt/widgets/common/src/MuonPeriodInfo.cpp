// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/MuonPeriodInfo.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/Logger.h"

#include "boost/algorithm/string.hpp"

#include <QString>
#include <bitset>

namespace MantidQt {
namespace MantidWidgets {

namespace {
const std::string PERIOD_INFO_NOT_FOUND = "Not found";
const std::string NOT_DAQ_STRING = "-";
const std::string DAQ = "1";
const std::string DWELL = "2";
const std::string CYCLES_NOT_FOUND = "Number of period cycles not found";
const std::string DEFAULT_TITLE = "Period Information for Run(s) ";
const QStringList HEADERS{"Period Count", "Period Name",  "Type",   "DAQ Number",
                          "Frames",       "Total Frames", "Counts", "Tag"};
const QString HEADER_STYLE = "QHeaderView { font-weight: bold; }";
} // namespace

using namespace Mantid::Kernel;
using namespace Mantid::API;

/**
 * Reads a sample log from a workspace
 * @param ws :: The workspace to read the sample log from
 * @param logName :: The name of the sample log to read
 * @returns :: String value of the sample log, or empty string if log cannot be found
 */
std::string MuonPeriodInfo::readSampleLog(MatrixWorkspace_sptr ws, const std::string &logName) {
  try {
    return ws->run().getLogData(logName)->value();
  } catch (const std::runtime_error &) {
    Logger("MuonPeriodInfo").warning("Workspace does not contain " + logName);
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
  if (log.empty())
    return std::vector<std::string>();
  std::vector<std::string> values;
  boost::split(values, log, boost::is_any_of(delim));
  return values;
}

/**
 * Makes sure each log vector is the same length taking the longest length as the standard. Adds in a period info not
 * found string to pad each vector to the correct length
 * @param logs :: Vector of Vectors which hold the values for each period info
 * @return :: A corrected vector which contains vectors of unified length
 */
std::vector<std::vector<std::string>> MuonPeriodInfo::makeCorrections(std::vector<std::vector<std::string>> &logs) {
  // Find max size of logs and assume to be the correct size
  size_t maxSize = 0;
  for (const auto &log : logs) {
    if (log.size() > maxSize)
      maxSize = log.size();
  }

  // Check size of each log and make corrections where needed
  for (size_t i = 0; i < logs.size(); ++i) {
    if (logs[i].empty()) {
      logs[i] = std::vector<std::string>(maxSize, PERIOD_INFO_NOT_FOUND);
    } else {
      while (logs[i].size() < maxSize) {
        logs[i].emplace_back(PERIOD_INFO_NOT_FOUND);
      }
    }
  }
  return logs;
}

/**
 * Constructor
 * @param parent :: A pointer to the parent of this widget
 */
MuonPeriodInfo::MuonPeriodInfo(QWidget *parent) : QWidget(parent), m_numberOfSequences(-1), m_DAQCount(0) {
  m_uiForm.setupUi(this);

  // Set default label text and default title and set up the table columns
  m_uiForm.label->setText(QString::fromStdString(CYCLES_NOT_FOUND));
  setWidgetTitleRuns("");
  setUpTable();
}

/**
 * Reads all period information from the workspace provided
 * @param ws :: Workspace to read sample logs from
 * @return :: A vector containing vectors for each period info read
 */
std::vector<std::vector<std::string>> MuonPeriodInfo::getInfo(MatrixWorkspace_sptr ws) {
  auto names = parseSampleLog(readSampleLog(ws, "period_labels"), ";");
  auto types = parseSampleLog(readSampleLog(ws, "period_type"), ";");
  auto frames = parseSampleLog(readSampleLog(ws, "frames_period_requested"), ";");
  auto total_frames = parseSampleLog(readSampleLog(ws, "frames_period_Raw"), ";");
  auto counts = parseSampleLog(readSampleLog(ws, "total_counts_period"), ";");
  auto tags = parseSampleLog(readSampleLog(ws, "period_output"), ";");
  std::vector<std::vector<std::string>> logs = {names, types, frames, total_frames, counts, tags};
  return makeCorrections(logs);
}

/**
 * Adds all period information from the workspace provided to the widgets table in a readable format
 * @param ws :: The workspace to read the period information from
 */
void MuonPeriodInfo::addInfo(const Mantid::API::Workspace_sptr ws) {
  if (auto matrixWS = std::dynamic_pointer_cast<MatrixWorkspace>(ws)) {
    // Read in period sequences
    const auto numSeq = readSampleLog(matrixWS, "period_sequences");
    if (!numSeq.empty())
      setNumberOfSequences(std::stoi(numSeq));
    else
      setNumberOfSequences(-1);

    // Get remaining logs and add to table
    auto logs = getInfo(matrixWS);
    for (size_t i = 0; i < logs[0].size(); ++i) {
      addPeriodToTable(logs[0][i], logs[1][i], logs[2][i], logs[3][i], logs[4][m_DAQCount], logs[5][i]);
    }
  } else {
    Logger("MuonPeriodInfo").warning("Could not read workspace");
  }
}

/**
 * Adds a new row to the table with the information provided
 * @param name :: Name of the period
 * @param type :: Whether the period is DAQ (1) or DWELL (2)
 * @param frames :: Frames period requested
 * @param totalFrames :: Total frames period requested
 * @param counts :: Total counts per period, only applies to DAQ periods
 * @param tag :: String value to convert to binary tag for the period
 */
void MuonPeriodInfo::addPeriodToTable(const std::string &name, const std::string &type, const std::string &frames,
                                      const std::string &totalFrames, const std::string &counts,
                                      const std::string &tag) {
  const auto row = m_uiForm.table->rowCount();
  m_uiForm.table->insertRow(row);
  m_uiForm.table->setItem(row, 0, createNewItem(std::to_string(row + 1)));
  m_uiForm.table->setItem(row, 1, createNewItem(name));
  if (type == DAQ) {
    ++m_DAQCount;
    m_uiForm.table->setItem(row, 2, createNewItem("DAQ"));
    m_uiForm.table->setItem(row, 3, createNewItem(std::to_string(m_DAQCount)));
    m_uiForm.table->setItem(row, 6, createNewItem(counts));
  } else if (type == DWELL) {
    m_uiForm.table->setItem(row, 2, createNewItem("DWELL"));
    m_uiForm.table->setItem(row, 3, createNewItem(NOT_DAQ_STRING));
    m_uiForm.table->setItem(row, 6, createNewItem(NOT_DAQ_STRING));
  }
  m_uiForm.table->setItem(row, 4, createNewItem(frames));
  m_uiForm.table->setItem(row, 5, createNewItem(totalFrames));
  try {
    m_uiForm.table->setItem(row, 7, createNewItem(std::bitset<4>(std::stoi(tag)).to_string()));
  } catch (...) {
    m_uiForm.table->setItem(row, 7, createNewItem(PERIOD_INFO_NOT_FOUND));
  }
}

/**
 * Sets the title of the widget with the string provided (typically instrument + runs)
 * @param title :: The new string value to add to the default title
 */
void MuonPeriodInfo::setWidgetTitleRuns(const std::string &title) {
  if (title.empty())
    this->setWindowTitle(QString::fromStdString(DEFAULT_TITLE));
  else
    this->setWindowTitle(QString::fromStdString(DEFAULT_TITLE + title));
}

std::string MuonPeriodInfo::getWidgetTitleRuns() const { return this->windowTitle().toStdString(); }

/**
 * Sets the numberOfSequences and updates the label of the widget.
 * A sequence is a complete recording of all periods once, and the number of sequences is how many times this happens
 * during a run
 * @param numberOfSequences :: The new value for the number of sequences
 */
void MuonPeriodInfo::setNumberOfSequences(const int numberOfSequences) {
  m_numberOfSequences = numberOfSequences;
  if (numberOfSequences < 0)
    m_uiForm.label->setText(QString::fromStdString(CYCLES_NOT_FOUND));
  else
    m_uiForm.label->setText(
        QString::fromStdString("Run contains " + std::to_string(numberOfSequences) + " cycles of periods"));
}

int MuonPeriodInfo::getNumberOfSequences() const { return m_numberOfSequences; }

std::string MuonPeriodInfo::getNumberOfSequencesString() const { return m_uiForm.label->text().toStdString(); }

int MuonPeriodInfo::getDAQCount() const { return m_DAQCount; }

/**
 * Clears all period information by removing all rows from the table and resetting the label
 */
void MuonPeriodInfo::clear() {
  m_DAQCount = 0;
  setNumberOfSequences(-1);
  setWidgetTitleRuns("");
  for (int i = m_uiForm.table->rowCount(); i >= 0; --i)
    m_uiForm.table->removeRow(i);
}

/**
 * @return :: True if the table is empty, False otherwise
 */
bool MuonPeriodInfo::isEmpty() const { return m_uiForm.table->rowCount() <= 0; }

QTableWidget *MuonPeriodInfo::getTable() const { return m_uiForm.table; }

/**
 * Set up properties of the widgets table
 */
void MuonPeriodInfo::setUpTable() {
  m_uiForm.table->setColumnCount(HEADERS.size());
  m_uiForm.table->setHorizontalHeaderLabels(QStringList(HEADERS));
  m_uiForm.table->horizontalHeader()->setStyleSheet(HEADER_STYLE);
  m_uiForm.table->verticalHeader()->setVisible(false);
  auto header = m_uiForm.table->horizontalHeader();
  for (int i = 0; i < HEADERS.size(); ++i)
    header->setSectionResizeMode(i, QHeaderView::ResizeToContents);
}

/**
 * Creates a new table widget item from a string to add to the table
 * @param value :: String value to display on the table item
 * @return :: Table widget item which stores the value provided
 */
QTableWidgetItem *MuonPeriodInfo::createNewItem(const std::string &value) const {
  auto item = new QTableWidgetItem();
  item->setText(QString::fromStdString(value));
  item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  return item;
}

} // namespace MantidWidgets
} // namespace MantidQt
