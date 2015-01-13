#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/Column.h"

namespace Mantid {

namespace DataObjects {

/** @class MementoTableWorkspace

  Variation on the TableWorkspace with a set of pre-defined columns used to
  store diffs on Workspaces.

  @author Owen Arnold
  @date 31/08/2011

  Copyright &copy; 2011-12 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport MementoTableWorkspace : public TableWorkspace {
public:
  static bool isMementoWorkspace(const Mantid::API::ITableWorkspace &candidate);
  MementoTableWorkspace(int nRows = 0);
  ~MementoTableWorkspace();

private:
  static bool expectedColumn(Mantid::API::Column_const_sptr expected,
                             Mantid::API::Column_const_sptr candidate);
};
}
}
