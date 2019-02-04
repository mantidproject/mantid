// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_PROPERTYNEXUS_H_
#define MANTID_KERNEL_PROPERTYNEXUS_H_

#include "MantidKernel/System.h"
#include <memory>
#include <string>

// Forward declare
namespace NeXus {
class File;
}

namespace Mantid {
namespace Kernel {
// Forward declare
class Property;

/** Namespace with helper methods for loading and saving Property's (logs)
 * to NXS files.

  @author Janik Zikovsky
  @date 2011-09-08
*/
namespace PropertyNexus {

DLLExport std::unique_ptr<Property> loadProperty(::NeXus::File *file,
                                                 const std::string &group);
}

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_PROPERTYNEXUS_H_ */
