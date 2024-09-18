// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "QtSearchModel.h"
#include "MantidAPI/TableRow.h"
#include <QColor>
#include <QSize>

namespace MantidQt::CustomInterfaces::ISISReflectometry {
using namespace Mantid::API;

QtSearchModel::QtSearchModel() : m_runDetails(), m_hasUnsavedChanges{false} {}

/** Merge new results into the existing results list. Keep the existing row if
 * a run already exists.
 */
void QtSearchModel::mergeNewResults(SearchResults const &source) {
  if (source.empty())
    return;

  // Extract the results that are not already in our list
  SearchResults newResults;
  std::copy_if(source.begin(), source.end(), std::back_inserter(newResults), [this](const auto &searchResult) {
    return std::find(m_runDetails.cbegin(), m_runDetails.cend(), searchResult) == m_runDetails.cend();
  });

  // Append the new results to our list. We need to tell the Qt model where we
  // are inserting and how many items we're adding
  const auto first = static_cast<int>(m_runDetails.size());
  const auto last = static_cast<int>(m_runDetails.size() + newResults.size() - 1);
  beginInsertRows(QModelIndex(), first, last);
  m_runDetails.insert(m_runDetails.end(), newResults.begin(), newResults.end());
  endInsertRows();
}

/** Clear the existing results list and replace it with a new one
 */
void QtSearchModel::replaceResults(SearchResults const &source) {
  clear();

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
int QtSearchModel::rowCount(const QModelIndex &) const { return static_cast<int>(m_runDetails.size()); }

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

  const int rowNumber = index.row();
  const auto column = static_cast<Column>(index.column());

  if (rowNumber < 0 || rowNumber >= static_cast<int>(m_runDetails.size()))
    return QVariant();

  auto const &run = m_runDetails[rowNumber];

  /*SETTING TOOL TIP AND BACKGROUND FOR INVALID RUNS*/
  if (role != Qt::DisplayRole) {
    if (role == Qt::ToolTipRole) {
      // setting the tool tips for any unsuccessful transfers or user
      // annotations
      if (run.hasError())
        return QString::fromStdString(std::string("Invalid transfer: ") + run.error());
      else if (run.exclude())
        return QString::fromStdString(std::string("Excluded by user: ") + run.excludeReason());
      else if (run.hasComment())
        return QString::fromStdString(std::string("User comment: ") + run.comment());
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
  if (column == Column::RUN)
    return QString::fromStdString(run.runNumber());
  if (column == Column::TITLE)
    return QString::fromStdString(run.title());
  if (column == Column::EXCLUDE)
    return QString::fromStdString(run.excludeReason());
  if (column == Column::COMMENT)
    return QString::fromStdString(run.comment());

  return QVariant();
}

bool QtSearchModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  if (role != Qt::EditRole)
    return true;

  const int rowNumber = index.row();
  const auto column = static_cast<Column>(index.column());

  if (rowNumber < 0 || rowNumber >= static_cast<int>(m_runDetails.size()))
    return false;

  auto &run = m_runDetails[rowNumber];

  if (column == Column::EXCLUDE)
    run.addExcludeReason(value.toString().toStdString());
  else if (column == Column::COMMENT)
    run.addComment(value.toString().toStdString());
  else
    return false;

  setUnsaved();
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
QVariant QtSearchModel::headerData(int section, Qt::Orientation orientation, int role) const {
  const auto column = static_cast<Column>(section);
  if (role == Qt::DisplayRole) {
    if (orientation == Qt::Horizontal) {
      switch (column) {
      case Column::RUN:
        return QString("Run");
      case Column::TITLE:
        return QString("Description");
      case Column::EXCLUDE:
        return QString("Exclude");
      case Column::COMMENT:
        return QString("Comment");
      default:
        return "";
      }
    }
  } else if (role == Qt::ToolTipRole) {
    if (orientation == Qt::Horizontal) {
      switch (column) {
      case Column::RUN:
        return QString("The run number from the catalog (not editable)");
      case Column::TITLE:
        return QString("The run title from the catalog (not editable)");
      case Column::EXCLUDE:
        return QString("User-specified exclude reason. Double-click to edit. "
                       "If set, the run will be excluded from autoprocessing "
                       "and/or transfers to the main table");
      case Column::COMMENT:
        return QString("User-specified annotation. Double-click to edit. Does "
                       "not affect the reduction.");
      default:
        return "";
      }
    }
  }

  return QVariant();
}

/**
Provide flags on an index by index basis
@param index: To generate a flag for.
*/
Qt::ItemFlags QtSearchModel::flags(const QModelIndex &index) const {
  const auto column = static_cast<Column>(index.column());
  if (!index.isValid())
    return {};
  else if (column == Column::EXCLUDE || column == Column::COMMENT)
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
  // Reset the unsaved changes flag
  setSaved();
}

bool QtSearchModel::hasUnsavedChanges() const { return m_hasUnsavedChanges; }

void QtSearchModel::setUnsaved() { m_hasUnsavedChanges = true; }

void QtSearchModel::setSaved() { m_hasUnsavedChanges = false; }

SearchResult const &QtSearchModel::getRowData(int index) const { return m_runDetails[index]; }

SearchResults const &QtSearchModel::getRows() const { return m_runDetails; }

std::string QtSearchModel::getSearchResultsCSV() const { return makeSearchResultsCSV(getRows()); }

std::string QtSearchModel::makeSearchResultsCSV(SearchResults const &results) const {
  if (results.empty()) {
    return "";
  }
  std::string csv = makeSearchResultsCSVHeaders();
  for (SearchResult const &result : results) {
    csv += result.runNumber() + "," + result.title() + "," + result.excludeReason() + "," + result.comment() + "\n";
  }
  return csv;
}

std::string QtSearchModel::makeSearchResultsCSVHeaders() const {
  std::string header;
  header +=
      headerData(static_cast<int>(Column::RUN), Qt::Orientation::Horizontal, Qt::DisplayRole).toString().toStdString() +
      ",";
  header += headerData(static_cast<int>(Column::TITLE), Qt::Orientation::Horizontal, Qt::DisplayRole)
                .toString()
                .toStdString() +
            ",";
  header += headerData(static_cast<int>(Column::EXCLUDE), Qt::Orientation::Horizontal, Qt::DisplayRole)
                .toString()
                .toStdString() +
            ",";
  header += headerData(static_cast<int>(Column::COMMENT), Qt::Orientation::Horizontal, Qt::DisplayRole)
                .toString()
                .toStdString() +
            "\n";
  return header;
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
