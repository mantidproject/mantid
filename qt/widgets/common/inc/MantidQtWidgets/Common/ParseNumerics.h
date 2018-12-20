#ifndef MANTID_MANTIDWIDGETS_DATAPOCESSORPARSENUMERICS_H
#define MANTID_MANTIDWIDGETS_DATAPOCESSORPARSENUMERICS_H
#include "MantidQtWidgets/Common/DllOption.h"
#include <QString>
/**

This file defines functions for parsing numbers from QStrings.

Copyright &copy; 2011-16 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
namespace MantidQt {
namespace MantidWidgets {
// Converts a string denoting a floating point value to a double.
double EXPORT_OPT_MANTIDQT_COMMON parseDouble(QString const &in);

/// Converts a string denoting a denary integer to it
int EXPORT_OPT_MANTIDQT_COMMON parseDenaryInteger(QString const &in);
} // namespace MantidWidgets
} // namespace MantidQt
#endif // MANTID_MANTIDWIDGETS_DATAPOCESSORPARSENUMERICS_H
