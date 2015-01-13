#ifndef MANTID_KERNEL_ENVIRONMENTHISTORY_H_
#define MANTID_KERNEL_ENVIRONMENTHISTORY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#include <string>

namespace Mantid {
namespace Kernel {
/** This class stores information about the Environment of the computer used by
 the framework.

  @author Dickon Champion, ISIS, RAL
  @date 21/01/2008

  Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_KERNEL_DLL EnvironmentHistory {
public:
  /// returns the framework version
  std::string frameworkVersion() const;
  /// returns the os name
  std::string osName() const;
  /// returns the os version
  std::string osVersion() const;
  /// print contents of object
  void printSelf(std::ostream &, const int indent = 0) const;

private:
  /// Private, unimplemented copy assignment operator
  EnvironmentHistory &operator=(const EnvironmentHistory &);
};

MANTID_KERNEL_DLL std::ostream &operator<<(std::ostream &,
                                           const EnvironmentHistory &);

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_ENVIRONMENTHISTORY_H_*/
