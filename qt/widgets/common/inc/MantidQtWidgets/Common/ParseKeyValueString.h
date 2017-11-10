#ifndef MANTIDQTMANTIDWIDGETS_PARSEKEYVALUESTRING_H
#define MANTIDQTMANTIDWIDGETS_PARSEKEYVALUESTRING_H

/** parseKeyValueString

Parses a string in the format `a = 1,b=2, c = "1,2,3,4", d = 5.0, e='a,b,c'`
into a map of key/value pairs.

Copyright &copy; 2011-14 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

#include "MantidKernel/System.h"

#include <string>
#include <map>

namespace MantidQt {
namespace MantidWidgets {

std::map<std::string, std::string> DLLExport
parseKeyValueString(const std::string &str);
}
}

#endif // MANTIDQTMANTIDWIDGETS_PARSEKEYVALUESTRING_H
