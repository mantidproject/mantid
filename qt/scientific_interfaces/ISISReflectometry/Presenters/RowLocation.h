/**
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
#ifndef MANTID_ISISREFLECTOMETRY_ROWLOCATION_H
#define MANTID_ISISREFLECTOMETRY_ROWLOCATION_H
#include "MantidQtWidgets/Common/Batch/RowLocation.h"

namespace MantidQt {
namespace CustomInterfaces {

std::vector<int> groupIndexesFromSelection(
    std::vector<MantidWidgets::Batch::RowLocation> const &selected);
std::vector<int> mapToContainingGroups(std::vector<
    MantidQt::MantidWidgets::Batch::RowLocation> const &mustNotContainRoot);
bool containsGroups(
    std::vector<MantidQt::MantidWidgets::Batch::RowLocation> const &locations);
bool isGroupLocation(
    MantidQt::MantidWidgets::Batch::RowLocation const &location);
int groupOf(MantidQt::MantidWidgets::Batch::RowLocation const &groupLocation);
bool isRowLocation(MantidWidgets::Batch::RowLocation const &location);
int rowOf(MantidQt::MantidWidgets::Batch::RowLocation const &rowLocation);
}
}
#endif // MANTID_ISISREFLECTOMETRY_ROWLOCATION_H
