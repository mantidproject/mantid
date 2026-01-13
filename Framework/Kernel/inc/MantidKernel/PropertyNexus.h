// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/System.h"
#include <memory>
#include <string>

// Forward declare
namespace Mantid::Nexus {
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

/**
 * @brief Opens a NXlog group in a nexus file and creates the correct Property object from it.
 * @param file currently opened NeXus file
 * @param group current group (relative name)
 * @param prefix indicates current group location in file (absolute name)
 * @return std::unique_ptr<Property>
 */
DLLExport std::unique_ptr<Property> loadProperty(Nexus::File *file, const std::string &group,
                                                 const std::string &prefix);

DLLExport std::unique_ptr<Property> loadProperty(Nexus::File *file, const std::string &group);

} // namespace PropertyNexus

} // namespace Kernel
} // namespace Mantid
