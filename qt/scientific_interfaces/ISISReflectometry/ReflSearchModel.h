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

Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
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
