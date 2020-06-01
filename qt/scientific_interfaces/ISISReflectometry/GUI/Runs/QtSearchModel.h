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
class MANTIDQT_ISISREFLECTOMETRY_DLL QtSearchModel : public QAbstractTableModel,
                                                     public ISearchModel {
  Q_OBJECT
public:
  QtSearchModel();
  // ISearchModel overrides
  void mergeNewResults(SearchResults const &source) override;
  virtual SearchResult const &getRowData(int index) const override;
  void clear() override;

  // QAbstractTableModel overrides
  // row and column counts
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  // get data from a cell
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  // get header data for the table
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role) const override;
  // get flags for a cell
  Qt::ItemFlags flags(const QModelIndex &index) const override;

protected:
  // Details about each run returned from the search
  SearchResults m_runDetails;

private:
  bool runHasError(const SearchResult &run) const;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
