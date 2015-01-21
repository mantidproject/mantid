#ifndef MANTID_KERNEL_MANTIDVERSION_H_
#define MANTID_KERNEL_MANTIDVERSION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"

#include <string>

namespace Mantid {
namespace Kernel {
/** Class containing static methods to return the Mantid version number and
   date.

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_KERNEL_DLL MantidVersion {
public:
  static const char *version(); ///< The full version number
  static std::string
  releaseNotes(); ///< The url to the most applicable release notes
  static const char *revision(); ///< The abbreviated SHA-1 of the last commit
  static const char *revisionFull();  ///< The full SHA-1 of the last commit
  static const char *releaseDate();   ///< The date of the last commit
  static std::string doi();           ///< The DOI for this release of Mantid.
  static std::string paperCitation(); ///< The citation for the Mantid paper

private:
  MantidVersion(); ///< Private, unimplemented constructor. Not a class that can
  /// be instantiated.
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_MANTIDVERSION_H_ */
