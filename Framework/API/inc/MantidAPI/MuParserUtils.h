#ifndef MANTID_API_MUPARSERUTILS_H_
#define MANTID_API_MUPARSERUTILS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidGeometry/muParser_Silent.h"

namespace Mantid {
namespace API {
namespace MuParserUtils {

/** Defines convenience methods to be used with the muParser mathematical
    expression parser.

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

/// A map from constants to their muParser names.
extern const MANTID_API_DLL std::map<double, std::string> MUPARSER_CONSTANTS;

/// Add a set of default constants to a muParser.
void MANTID_API_DLL addDefaultConstants(mu::Parser &parser);

using oneVarFun = double (*)(double); // pointer to a function of one variable
extern const MANTID_API_DLL std::map<std::string, oneVarFun>
    MUPARSER_ONEVAR_FUNCTIONS;

void MANTID_API_DLL extraOneVarFunctions(mu::Parser &parser);

} // namespace MuParserUtils
} // namespace API
} // namespace Mantid

#endif /* MANTID_API_MUPARSERUTILS_H_ */
