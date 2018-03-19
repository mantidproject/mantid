#ifndef MANTID_ISISREFLECTOMETRY_REFLSEARCHMODEL_H_
#define MANTID_ISISREFLECTOMETRY_REFLSEARCHMODEL_H_

#include "MantidAPI/ITableWorkspace_fwd.h"
#include <QAbstractTableModel>
#include <boost/shared_ptr.hpp>
#include <map>
#include <vector>
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {
// Forward declaration
class ReflTransferStrategy;

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
class ReflSearchModel : public QAbstractTableModel {
  Q_OBJECT
public:
  ReflSearchModel(const ReflTransferStrategy &transferMethod,
                  Mantid::API::ITableWorkspace_sptr tableWorkspace,
                  const std::string &instrument);
  ~ReflSearchModel() override;
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
  /// maps each run number to why it was unusable in the process table
  std::vector<std::map<std::string, std::string>> m_errors;
  /// clear the model
  void clear();

protected:
  // vector of the run numbers
  std::vector<std::string> m_runs;

  /// maps each run number to its description
  std::map<std::string, std::string> m_descriptions;

  /// maps each run number to its location
  std::map<std::string, std::string> m_locations;
};

/// Typedef for a shared pointer to \c ReflSearchModel
using ReflSearchModel_sptr = boost::shared_ptr<ReflSearchModel>;

} // namespace CustomInterfaces
} // namespace Mantid

#endif /* MANTID_ISISREFLECTOMETRY_REFLSEARCHMODEL_H_ */
