// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IArchiveSearch.h"
#include "MantidDataHandling/DllConfig.h"

#include <sstream>
#include <string>

namespace Mantid {
namespace DataHandling {

/**
This class is for searching the ISIS data archive

@author Roman Tolchenov, Tessella plc
@date 27/07/2010
*/
class MANTID_DATAHANDLING_DLL ISISDataArchive : public API::IArchiveSearch {
public:
  /// Returns the path to a filename given the list of extensions to try
  const API::Result<std::string> getArchivePath(const std::set<std::string> &filenames,
                                                const std::vector<std::string> &exts) const override;

  /// Public and virtual for testing purposes
  virtual std::string getCorrectExtension(const std::string &path, const std::vector<std::string> &exts) const;
  std::string getPath(const std::string &fName) const;

protected:
  /// Queries the archive & returns the path to a single file.
  virtual std::ostringstream sendRequest(const std::string &fName) const;
  virtual bool fileExists(const std::string &path) const;
};
} // namespace DataHandling
} // namespace Mantid
