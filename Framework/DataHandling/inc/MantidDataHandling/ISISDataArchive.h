// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_ISISDATAARCHIVE_H_
#define MANTID_DATAHANDLING_ISISDATAARCHIVE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IArchiveSearch.h"
#include "MantidKernel/System.h"

#include <string>

namespace Mantid {
namespace DataHandling {

/**
This class is for searching the ISIS data archive

@author Roman Tolchenov, Tessella plc
@date 27/07/2010
*/
class DLLExport ISISDataArchive : public API::IArchiveSearch {
public:
  /// Returns the path to a filename given the list of extensions to try
  std::string
  getArchivePath(const std::set<std::string> &filenames,
                 const std::vector<std::string> &exts) const override;

private:
  /// Queries the archive & returns the path to a single file.
  std::string getPath(const std::string &fName) const;
};
} // namespace DataHandling
} // namespace Mantid

#endif // MANTID_DATAHANDLING_ISISDATAARCHIVE_H_
