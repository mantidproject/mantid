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

/** Merge new results into the existing results list. Keep the existing row if
 * a run already exists.
 */
void QtSearchModel::mergeNewResults(SearchResults const &source) {
  if (source.empty())
    return;

  // Extract the results that are not already in our list
  SearchResults newResults;
  std::copy_if(source.begin(), source.end(), std::back_inserter(newResults),
               [this](const auto &searchResult) {
                 return std::find(m_runDetails.cbegin(), m_runDetails.cend(),
                                  searchResult) == m_runDetails.cend();
               });

  // Append the new results to our list. We need to tell the Qt model where we
  // are inserting and how many items we're adding
  const auto first = static_cast<int>(m_runDetails.size());
  const auto last =
      static_cast<int>(m_runDetails.size() + newResults.size() - 1);
  beginInsertRows(QModelIndex(), first, last);
  m_runDetails.insert(m_runDetails.end(), newResults.begin(), newResults.end());
  endInsertRows();
}

/** Clear the existing results list and replace it with a new one
 */
void QtSearchModel::replaceResults(SearchResults const &source) {
  m_runDetails.clear();

  if (source.empty())
    return;

  // We need to tell the Qt model where we are inserting and how many items
  // we're adding
  const auto first = 0;
  const auto last = static_cast<int>(source.size());
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
int QtSearchModel::columnCount(const QModelIndex &) const { return 4; }

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
      // setting the tool tips for any unsuccessful transfers or user
      // annotations
      if (run.hasError())
        return QString::fromStdString(std::string("Invalid transfer: ") +
                                      run.error());
      else if (run.exclude())
        return QString::fromStdString(std::string("Excluded by user: ") +
                                      run.excludeReason());
      else if (run.hasComment())
        return QString::fromStdString(std::string("User comment: ") +
                                      run.comment());
    } else if (role == Qt::BackgroundRole) {
      // setting the background colour for any unsuccessful transfers / excluded
      // runs
      if (run.hasError() || run.exclude())
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
  if (colNumber == 2)
    return QString::fromStdString(run.excludeReason());
  if (colNumber == 3)
    return QString::fromStdString(run.comment());

  return QVariant();
}

bool QtSearchModel::setData(const QModelIndex &index, const QVariant &value,
                            int role) {
  if (role != Qt::EditRole)
    return true;

  const int colNumber = index.column();
  const int rowNumber = index.row();

  if (rowNumber < 0 || rowNumber >= static_cast<int>(m_runDetails.size()))
    return false;

  auto &run = m_runDetails[rowNumber];

  if (colNumber == 2)
    run.addExcludeReason(value.toString().toStdString());
  else if (colNumber == 3)
    run.addComment(value.toString().toStdString());
  else
    return false;

  emit dataChanged(index, index);
  return true;
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
    case 2:
      return "Exclude reason";
    case 3:
      return "Comment";
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
  else if (index.column() == 2 || index.column() == 3)
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
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

SearchResult const &QtSearchModel::getRowData(int index) const {
  return m_runDetails[index];
}

SearchResults const &QtSearchModel::getRows() const { return m_runDetails; }
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
