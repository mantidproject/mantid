#include "ReflSearchModel.h"
#include "ReflTransferStrategy.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include <boost/regex.hpp>
#include <QColor>

namespace MantidQt {
namespace CustomInterfaces {
using namespace Mantid::API;

bool ReflSearchModel::knownFileType(std::string const& filename) const {
  boost::regex pattern("raw$", boost::regex::icase);
  boost::smatch match; // Unused.
  return boost::regex_search(filename, match, pattern);
}

//----------------------------------------------------------------------------------------------
/** Constructor
@param transferMethod : Transfer strategy
@param tableWorkspace : The table workspace to copy data from
@param instrument : instrument name
*/
ReflSearchModel::ReflSearchModel(ITableWorkspace_sptr tableWorkspace,
                                 const std::string &instrument) {
  // Copy the data from the input table workspace
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

    if (knownFileType(runFile)) {
      m_runs.push_back(run);
      const std::string description = tableWorkspace->String(i, 6);
      m_descriptions[run] = description;
      const std::string location = tableWorkspace->String(i, 1);
      m_locations[run] = location;
    }
  }

  // By sorting the vector of runs, we sort the entire table
  std::sort(m_runs.begin(), m_runs.end());
}

/**
@return the row count.
*/
int ReflSearchModel::rowCount(const QModelIndex &) const {
  return static_cast<int>(m_runs.size());
}

/**
@return the number of columns in the model.
*/
int ReflSearchModel::columnCount(const QModelIndex &) const { return 3; }

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

  const std::string run = m_runs[rowNumber];

  /*SETTING TOOL TIP AND BACKGROUND FOR INVALID RUNS*/
  if (role != Qt::DisplayRole) {
    if (role == Qt::ToolTipRole) {
      // setting the tool tips for any unsuccessful transfers
      for (auto errorRow = m_errors.begin(); errorRow != m_errors.end();
           ++errorRow) {
        if (errorRow->find(run) != errorRow->end()) {
          // get the error message from the unsuccessful transfer
          std::string errorMessage =
              "Invalid transfer: " + errorRow->find(run)->second;
          // set the message as the tooltip
          return QString::fromStdString(errorMessage);
        }
      }
    } else if (role == Qt::BackgroundRole) {
      // setting the background colour for any unsuccessful transfers
      for (auto errorRow = m_errors.begin(); errorRow != m_errors.end();
           ++errorRow) {
        if (errorRow->find(run) != errorRow->end()) {
          // return the colour yellow for any successful runs
          return QColor("#FF8040");
        }
      }
    } else {
      // we have no unsuccessful transfers so return empty QVariant
      return QVariant();
    }
  }
  /*SETTING DATA FOR RUNS*/
  if (colNumber == 0)
    return QString::fromStdString(run);

  if (colNumber == 1)
    return QString::fromStdString(m_descriptions.find(run)->second);

  if (colNumber == 2)
    return QString::fromStdString(m_locations.find(run)->second);

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
  m_descriptions.clear();
  m_locations.clear();

  endResetModel();
}

} // namespace CustomInterfaces
} // namespace Mantid
