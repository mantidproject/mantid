#ifndef MANTID_KERNEL_PROPERTYMANAGER_FWD_H
#define MANTID_KERNEL_PROPERTYMANAGER_FWD_H

#include <boost/shared_ptr.hpp>
#include <memory>

namespace Mantid {
namespace Kernel {
/**
  This file provides forward declarations for Mantid::Kernel::PropertyManager

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

  File change history is stored at: <https://github.com/mantidproject/mantid>.
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

/// forward declare of Mantid::Kernel::PropertyManager
class PropertyManager;
/// shared pointer to Mantid::Kernel::PropertyManager
using PropertyManager_sptr = boost::shared_ptr<PropertyManager>;
/// shared pointer to Mantid::Kernel::PropertyManager(const version)
using PropertyManager_const_sptr = boost::shared_ptr<const PropertyManager>;
/// unique pointer to Mantid::Kernel::PropertyManager
using PropertyManager_uptr = std::unique_ptr<PropertyManager>;
/// unique pointer to Mantid::Kernel::PropertyManager (const version)
using PropertyManager_const_uptr = std::unique_ptr<const PropertyManager>;

} // namespace Kernel
} // namespace Mantid

#endif // MANTID_KERNEL_PROPERTYMANAGER_FWD_H
