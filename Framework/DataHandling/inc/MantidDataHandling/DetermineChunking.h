// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IEventWorkspace_fwd.h"
#include "MantidDataHandling/DllConfig.h"
#include <string>
#include <vector>

namespace Mantid {
namespace DataHandling {

/** DetermineChunking : Workflow algorithm to determine chunking

  @date 2012-01-30
*/
/// Allowed file types

enum FileType {
  EVENT_NEXUS_FILE, ///< Event NeXus files
  HISTO_NEXUS_FILE, ///< Histogram NeXus files
  RAW_FILE          ///< ISIS raw files
};

class MANTID_DATAHANDLING_DLL DetermineChunking : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Workflow algorithm to determine chunking strategy for event nexus, "
           "raw, or histo nexus files.";
  }

  int version() const override;
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
  std::string setTopEntryName(const std::string &filename);
  FileType getFileType(const std::string &filename);
  bool filenameHasExtension(const std::string &filename, const std::vector<std::string> &fileExtensions);
};

} // namespace DataHandling
} // namespace Mantid
