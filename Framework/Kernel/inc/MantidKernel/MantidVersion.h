// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"

#include <string>

namespace Mantid {
namespace Kernel {
/** Class containing static methods to return the Mantid version number and
   date.
 */
class MANTID_KERNEL_DLL MantidVersion {
public:
  static const char *version();       ///< The full version number
  static const char *versionShort();  ///< The version number of the last full version
  static std::string releaseNotes();  ///< The url to the most applicable release notes
  static const char *revision();      ///< The abbreviated SHA-1 of the last commit
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
