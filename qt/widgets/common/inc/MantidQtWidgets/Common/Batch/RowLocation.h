/**
See the developer documentation for Batch Widget at
developer.mantidproject.org/BatchWidget/index.html

Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
#ifndef MANTIDQTMANTIDWIDGETS_ROWLOCATION_H_
#define MANTIDQTMANTIDWIDGETS_ROWLOCATION_H_
#include "MantidQtWidgets/Common/DllOption.h"
#include <boost/optional.hpp>
#include <ostream>
#include <vector>
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

using RowPath = std::vector<int>;

class EXPORT_OPT_MANTIDQT_COMMON RowLocation {
public:
  RowLocation() = default;
  RowLocation(RowPath path);
  RowPath const &path() const;
  int rowRelativeToParent() const;
  bool isRoot() const;
  int depth() const;
  bool isChildOf(RowLocation const &other) const;
  bool isSiblingOf(RowLocation const &other) const;
  bool isChildOrSiblingOf(RowLocation const &other) const;
  bool isDescendantOf(RowLocation const &other) const;
  RowLocation parent() const;
  RowLocation relativeTo(RowLocation const &ancestor) const;
  RowLocation child(int n) const;
  bool operator==(RowLocation const &other) const;
  bool operator!=(RowLocation const &other) const;
  bool operator<(RowLocation const &other) const;
  bool operator<=(RowLocation const &other) const;
  bool operator>(RowLocation const &other) const;
  bool operator>=(RowLocation const &other) const;

private:
  RowPath m_path;
};

EXPORT_OPT_MANTIDQT_COMMON std::ostream &
operator<<(std::ostream &os, RowLocation const &location);
EXPORT_OPT_MANTIDQT_COMMON bool
pathsSameUntilDepth(int depth, RowLocation const &locationA,
                    RowLocation const &locationB);

} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
#endif
