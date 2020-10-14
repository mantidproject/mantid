// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "QtSearchModel.h"
#include "MantidAPI/TableRow.h"
#include <QColor>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {
using namespace Mantid::API;

QtSearchModel::QtSearchModel() : m_runDetails() {}

void QtSearchModel::mergeNewResults(SearchResults const &source) {
  if (source.empty())
    return;

  // To append, insert the new runs after the last element in the model
  const auto first = static_cast<int>(m_runDetails.size());
  const auto last = static_cast<int>(m_runDetails.size() + source.size() - 1);
  beginInsertRows(QModelIndex(), first, last);

  m_runDetails.insert(m_runDetails.end(), source.begin(), source.end());

  endInsertRows();
}

/**
@return the row count.
*/
int QtSearchModel::rowCount(const QModelIndex &) const {
  return static_cast<int>(m_runDetails.size());
}

/**
@return the number of columns in the model.
*/
int QtSearchModel::columnCount(const QModelIndex &) const { return 2; }

/**
Overrident data method, allows consuming view to extract data for an index and
role.
@param index : For which to extract the data
@param role : Role mode
*/
QVariant QtSearchModel::data(const QModelIndex &index, int role) const {

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
        auto errorMessage = "Invalid transfer: " + run.error();
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
    return QString::fromStdString(run.runNumber());

  if (colNumber == 1)
    return QString::fromStdString(run.title());

  return QVariant();
}

/**
Get the heading for a given section, orientation and role.
@param section : Column index
@param orientation : Heading orientation
@param role : Role mode of table.
@return HeaderData.
*/
QVariant QtSearchModel::headerData(int section, Qt::Orientation orientation,
                                   int role) const {
  if (role != Qt::DisplayRole)
    return QVariant();

  if (orientation == Qt::Horizontal) {
    switch (section) {
    case 0:
      return "Run";
    case 1:
      return "Description";
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
Qt::ItemFlags QtSearchModel::flags(const QModelIndex &index) const {
  if (!index.isValid())
    return nullptr;
  else
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

/**
Clear the model
*/
void QtSearchModel::clear() {
  beginResetModel();
  m_runDetails.clear();
  endResetModel();
}

/** Check whether a run has any error messages
@param run : the run number
@return : true if there is at least one error for this run
*/
bool QtSearchModel::runHasError(const SearchResult &run) const {
  return !(run.error().empty());
}

SearchResult const &QtSearchModel::getRowData(int index) const {
  return m_runDetails[index];
}
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
