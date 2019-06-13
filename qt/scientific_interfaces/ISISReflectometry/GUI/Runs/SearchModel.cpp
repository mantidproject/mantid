// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "SearchModel.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include <QColor>
#include <boost/regex.hpp>

namespace MantidQt {
namespace CustomInterfaces {
using namespace Mantid::API;

bool SearchModel::knownFileType(std::string const &filename) const {
  boost::regex pattern("raw$", boost::regex::icase);
  boost::smatch match; // Unused.
  return boost::regex_search(filename, match, pattern);
}

std::vector<SearchResult> const &SearchModel::results() const {
  return m_runDetails;
}

/**
@param tableWorkspace : The table workspace to copy data from
@param instrument : instrument name
*/
SearchModel::SearchModel(ITableWorkspace_sptr tableWorkspace,
                         const std::string &instrument) {
  if (tableWorkspace)
    addDataFromTable(tableWorkspace, instrument);
}

void SearchModel::setError(int i, std::string const &error) {
  m_runDetails[i].issues = error;
  emit dataChanged(index(i, 0), index(i, 2));
}

void SearchModel::addDataFromTable(ITableWorkspace_sptr tableWorkspace,
                                   const std::string &instrument) {

  // Copy the data from the input table workspace
  std::vector<SearchResult> newRunDetails;
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

    if (!knownFileType(runFile))
      continue;

    // Ok, add the run details to the list
    const std::string description = tableWorkspace->String(i, 6);
    const std::string location = tableWorkspace->String(i, 1);
    newRunDetails.emplace_back(run, description, location);
  }

  if (newRunDetails.empty()) {
    return;
  }

  // To append, insert the new runs after the last element in the model
  const auto first = static_cast<int>(m_runDetails.size());
  const auto last =
      static_cast<int>(m_runDetails.size() + newRunDetails.size() - 1);
  beginInsertRows(QModelIndex(), first, last);

  m_runDetails.insert(m_runDetails.end(), newRunDetails.begin(),
                      newRunDetails.end());

  endInsertRows();
}

/**
@return the row count.
*/
int SearchModel::rowCount(const QModelIndex &) const {
  return static_cast<int>(m_runDetails.size());
}

/**
@return the number of columns in the model.
*/
int SearchModel::columnCount(const QModelIndex &) const { return 3; }

/**
Overrident data method, allows consuming view to extract data for an index and
role.
@param index : For which to extract the data
@param role : Role mode
*/
QVariant SearchModel::data(const QModelIndex &index, int role) const {

  const int colNumber = index.column();
  const int rowNumber = index.row();

  if (rowNumber < 0 || rowNumber >= static_cast<int>(m_runDetails.size()))
    return QVariant();

  auto const &run = m_runDetails[rowNumber];

  /*SETTING TOOL TIP AND BACKGROUND FOR INVALID RUNS*/
  if (role != Qt::DisplayRole) {
    if (role == Qt::ToolTipRole) {
      // setting the tool tips for any unsuccessful transfers
      if (runHasError(run)) {
        auto errorMessage = "Invalid transfer: " + run.issues;
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
    return QString::fromStdString(run.runNumber);

  if (colNumber == 1)
    return QString::fromStdString(run.description);

  if (colNumber == 2)
    return QString::fromStdString(run.location);

  return QVariant();
}

/**
Get the heading for a given section, orientation and role.
@param section : Column index
@param orientation : Heading orientation
@param role : Role mode of table.
@return HeaderData.
*/
QVariant SearchModel::headerData(int section, Qt::Orientation orientation,
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
Qt::ItemFlags SearchModel::flags(const QModelIndex &index) const {
  if (!index.isValid())
    return nullptr;
  else
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

/**
Clear the model
*/
void SearchModel::clear() {
  beginResetModel();
  m_runDetails.clear();
  endResetModel();
}

/** Check whether a run has any error messages
@param run : the run number
@return : true if there is at least one error for this run
*/
bool SearchModel::runHasError(const SearchResult &run) const {
  return !(run.issues.empty());
}

SearchResult const &SearchModel::getRowData(int index) const {
  return m_runDetails[index];
}

} // namespace CustomInterfaces
} // namespace MantidQt
