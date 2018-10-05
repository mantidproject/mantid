// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_REFLSEARCHMODEL_H_
#define MANTID_ISISREFLECTOMETRY_REFLSEARCHMODEL_H_

#include "DllConfig.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "SearchResult.h"
#include <QAbstractTableModel>
#include <boost/shared_ptr.hpp>
#include <map>
#include <memory>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

/** ReflSearchModel : Provides a QAbstractTableModel for a Mantid
ITableWorkspace of Reflectometry search results.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL ReflSearchModel
    : public QAbstractTableModel {
  Q_OBJECT
public:
  ReflSearchModel(Mantid::API::ITableWorkspace_sptr tableWorkspace,
                  const std::string &instrument);
  void addDataFromTable(Mantid::API::ITableWorkspace_sptr tableWorkspace,
                        const std::string &instrument);
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
  /// clear the model
  void clear();
  bool knownFileType(std::string const &filename) const;
  /// Add details of an error
  std::vector<SearchResult> const &results() const;

  SearchResult const &operator[](int index) const;

  void setError(int index, std::string const &error);

protected:
  // map of run numbers to search result details
  std::vector<SearchResult> m_runDetails;

private:
  bool runHasError(const SearchResult &run) const;
};

/// Typedef for a shared pointer to \c ReflSearchModel
using ReflSearchModel_sptr = boost::shared_ptr<ReflSearchModel>;

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_ISISREFLECTOMETRY_REFLSEARCHMODEL_H_ */
