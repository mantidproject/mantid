// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ReflSearchModel.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "ReflTransferStrategy.h"
#include <QColor>

namespace MantidQt {
namespace CustomInterfaces {
using namespace Mantid::API;

//----------------------------------------------------------------------------------------------
/** Constructor
@param transferMethod : Transfer strategy
@param tableWorkspace : The table workspace to copy data from
@param instrument : instrument name
*/
ReflSearchModel::ReflSearchModel(const ReflTransferStrategy &transferMethod,
                                 ITableWorkspace_sptr tableWorkspace,
                                 const std::string &instrument) {
  if (tableWorkspace)
    addDataFromTable(transferMethod, tableWorkspace, instrument);
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ReflSearchModel::~ReflSearchModel() {}

void ReflSearchModel::addDataFromTable(
    const ReflTransferStrategy &transferMethod,
    ITableWorkspace_sptr tableWorkspace, const std::string &instrument) {

  // Copy the data from the input table workspace
  SearchResultMap newRunDetails;
  for (size_t i = 0; i < tableWorkspace->rowCount(); ++i) {
    const std::string runFile = tableWorkspace->String(i, 0);

    // If this isn't the right instrument, remove it
    auto run = runFile;
    if (run.substr(0, instrument.size()) != instrument) {
      continue; // Don't show runs that appear to be from other instruments.
    }

    // It's a valid run, so let's trim the instrument prefix and ".raw"
    // suffix
    run = run.substr(instrument.size(), run.size() - (instrument.size() + 4));

    // Let's also get rid of any leading zeros
    size_t numZeros = 0;
    while (run[numZeros] == '0')
      numZeros++;
    run = run.substr(numZeros, run.size() - numZeros);

    if (!transferMethod.knownFileType(runFile))
      continue;

    // Ignore if the run already exists
    if (runHasDetails(run))
      continue;

    // Ok, add the run details to the list
    const std::string description = tableWorkspace->String(i, 6);
    const std::string location = tableWorkspace->String(i, 1);
    newRunDetails[run] = SearchResult{description, location};
  }

  if (newRunDetails.empty()) {
    return;
  }

  // To append, insert the new runs after the last element in the model
  const auto first = static_cast<int>(m_runs.size());
  const auto last = static_cast<int>(m_runs.size() + newRunDetails.size() - 1);
  beginInsertRows(QModelIndex(), first, last);

  for (auto &runKvp : newRunDetails)
    m_runs.push_back(runKvp.first);

  m_runDetails.insert(newRunDetails.begin(), newRunDetails.end());

  endInsertRows();
}

/**
@return the row count.
*/
int ReflSearchModel::rowCount(const QModelIndex & /*parent*/) const {
  return static_cast<int>(m_runs.size());
}

/**
@return the number of columns in the model.
*/
int ReflSearchModel::columnCount(const QModelIndex & /*parent*/) const { return 3; }

/**
Overrident data method, allows consuming view to extract data for an index and
role.
@param index : For which to extract the data
@param role : Role mode
*/
QVariant ReflSearchModel::data(const QModelIndex &index, int role) const {

  const int colNumber = index.column();
  const int rowNumber = index.row();

  if (rowNumber < 0 || rowNumber >= static_cast<int>(m_runs.size()))
    return QVariant();

  const auto run = m_runs[rowNumber];

  /*SETTING TOOL TIP AND BACKGROUND FOR INVALID RUNS*/
  if (role != Qt::DisplayRole) {
    if (role == Qt::ToolTipRole) {
      // setting the tool tips for any unsuccessful transfers
      if (runHasError(run)) {
        auto errorMessage = "Invalid transfer: " + runError(run);
        return QString::fromStdString(errorMessage);
      }
    } else if (role == Qt::BackgroundRole) {
      // setting the background colour for any unsuccessful transfers
      if (runHasError(run))
        return QColor("#accbff");
    } else {
      // we have no unsuccessful transfers so return empty QVariant
      return QVariant();
    }
  }
  /*SETTING DATA FOR RUNS*/
  if (colNumber == 0)
    return QString::fromStdString(run);

  if (colNumber == 1)
    return QString::fromStdString(runDescription(run));

  if (colNumber == 2)
    return QString::fromStdString(runLocation(run));

  return QVariant();
}

/**
Get the heading for a given section, orientation and role.
@param section : Column index
@param orientation : Heading orientation
@param role : Role mode of table.
@return HeaderData.
*/
QVariant ReflSearchModel::headerData(int section, Qt::Orientation orientation,
                                     int role) const {
  if (role != Qt::DisplayRole)
    return QVariant();

  if (orientation == Qt::Horizontal) {
    switch (section) {
    case 0:
      return "Run";
    case 1:
      return "Description";
    case 2:
      return "Location";
    default:
      return "";
    }
  }
  return QVariant();
}

/**
Provide flags on an index by index basis
@param index: To generate a flag for.
*/
Qt::ItemFlags ReflSearchModel::flags(const QModelIndex &index) const {
  if (!index.isValid())
    return nullptr;
  else
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

/**
Clear the model
*/
void ReflSearchModel::clear() {

  beginResetModel();

  m_runs.clear();
  m_runDetails.clear();

  endResetModel();
}

/**
Add details about errors
@param run : the run number to set the error for
@param errorMessage : the error message
*/
void ReflSearchModel::addError(const std::string &run,
                               const std::string &errorMessage) {
  // Add the error if we have details for this run (ignore it if not)
  if (runHasDetails(run))
    m_runDetails[run].issues = errorMessage;
}

/** Clear any error messages for the given run
@param run : the run number to clear the error for
 */
void ReflSearchModel::clearError(const std::string &run) {
  if (runHasError(run))
    m_runDetails[run].issues = "";
}

bool ReflSearchModel::runHasDetails(const std::string &run) const {
  return (m_runDetails.find(run) != m_runDetails.end());
}

/** Get the details for a given run.
@param run : the run number
@return : the details associated with this run
*/
SearchResult ReflSearchModel::runDetails(const std::string &run) const {
  if (!runHasDetails(run))
    return SearchResult();

  return m_runDetails.find(run)->second;
}

/** Check whether a run has any error messages
@param run : the run number
@return : true if there is at least one error for this run
*/
bool ReflSearchModel::runHasError(const std::string &run) const {
  if (!runHasDetails(run))
    return false;

  if (runDetails(run).issues.empty())
    return false;

  return true;
}

/** Get the error message for a given run.
@param run : the run number
@return : the error associated with this run, or an empty string
if there is no error
*/
std::string ReflSearchModel::runError(const std::string &run) const {
  if (!runHasError(run))
    return std::string();

  return runDetails(run).issues;
}

/** Get the description for a given run.
@param run : the run number
@return : the description associated with this run, or an empty string
if there is no error
*/
std::string ReflSearchModel::runDescription(const std::string &run) const {
  if (!runHasDetails(run))
    return std::string();

  return runDetails(run).description;
}

/** Get the file location for a given run.
@param run : the run number
@return : the location associated with this run, or an empty string
if there is no error
*/
std::string ReflSearchModel::runLocation(const std::string &run) const {
  if (!runHasDetails(run))
    return std::string();

  return runDetails(run).location;
}

} // namespace CustomInterfaces
} // namespace MantidQt
