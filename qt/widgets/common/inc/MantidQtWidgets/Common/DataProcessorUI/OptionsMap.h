#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSOROPTIONSMAP_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSOROPTIONSMAP_H
/** This file defines the OptionsData type alias used by
   the DataProcessor widget.

    Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

#include <QString>
#include <map>

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
/// A map of key=value pairs
using OptionsMap = std::map<QString, QString>;
/// A map of column name to options that are applicable to that column
using ColumnOptionsMap = std::map<QString, OptionsMap>;
}
}
}
#endif // MANTIDQTMANTIDWIDGETS_DATAPROCESSOROPTIONSMAP_H
