#ifndef MANTID_KERNEL_PROPERTYNEXUS_H_
#define MANTID_KERNEL_PROPERTYNEXUS_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Property.h"
#include <nexus/NeXusFile.hpp>

namespace Mantid {
namespace API {

/** Namespace with helper methods for loading and saving Property's (logs)
 * to NXS files.

  @author Janik Zikovsky
  @date 2011-09-08

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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
namespace PropertyNexus {

DLLExport Mantid::Kernel::Property *loadProperty(::NeXus::File *file,
                                                 const std::string &group);

DLLExport void saveProperty(::NeXus::File *file,
                            Mantid::Kernel::Property *prop);
}

} // namespace API
} // namespace Mantid

#endif /* MANTID_KERNEL_PROPERTYNEXUS_H_ */
