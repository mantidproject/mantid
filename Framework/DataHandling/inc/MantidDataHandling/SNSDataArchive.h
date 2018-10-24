// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_SNSDATAARCHIVE_H_
#define MANTID_DATAHANDLING_SNSDATAARCHIVE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IArchiveSearch.h"
#include "MantidKernel/System.h"

#include <string>

namespace Mantid {
namespace DataHandling {
/**
 This class is for searching the SNS data archive
 @date 02/22/2012
 */

class DLLExport SNSDataArchive : public API::IArchiveSearch {
public:
  /// Find the archive location of a set of files.
  std::string
  getArchivePath(const std::set<std::string> &filenames,
                 const std::vector<std::string> &exts) const override;
};
} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_SNSDATAARCHIVE_H_ */
