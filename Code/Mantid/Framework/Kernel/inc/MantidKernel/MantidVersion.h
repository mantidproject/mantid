#ifndef MANTID_KERNEL_MANTIDVERSION_H_
#define MANTID_KERNEL_MANTIDVERSION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"

namespace Mantid
{
namespace Kernel
{
/** Class containing static methods to return the Mantid version number and date.

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport MantidVersion
{
public:
  static const char* version();     ///< The full version number
  static const char* revision();    ///< The SVN revision number
  static const char* releaseDate(); ///< The date of the last commit

private:
  MantidVersion(); ///< Private, unimplemented constructor. Not a class that can be instantiated.
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_MANTIDVERSION_H_ */
