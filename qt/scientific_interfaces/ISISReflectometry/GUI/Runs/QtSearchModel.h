// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "ISearchModel.h"
#include "SearchResult.h"
#include <QAbstractTableModel>
#include <map>
#include <memory>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

/** @class QtSearchModel

Provides a QAbstractTableModel for the search results widget on the QtRunsView.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL QtSearchModel : public QAbstractTableModel, public ISearchModel {
  Q_OBJECT
public:
  QtSearchModel();
  // ISearchModel overrides
  void mergeNewResults(SearchResults const &source) override;
  void replaceResults(SearchResults const &source) override;
  virtual SearchResult const &getRowData(int index) const override;
  virtual SearchResults const &getRows() const override;
  void clear() override;
  bool hasUnsavedChanges() const override;
  void setUnsaved() override;
  void setSaved() override;

  // QAbstractTableModel overrides
  // row and column counts
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  // get/set data for a cell
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
  // get header data for the table
  QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
  // get flags for a cell
  Qt::ItemFlags flags(const QModelIndex &index) const override;

protected:
  // Details about each run returned from the search
  SearchResults m_runDetails;
  // Flag to indicate whether there are unsaved changes
  bool m_hasUnsavedChanges;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
