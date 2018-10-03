// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_DetermineChunking_H_
#define MANTID_DATAHANDLING_DetermineChunking_H_

#include "MantidAPI/IEventWorkspace_fwd.h"
#include "MantidAPI/ParallelAlgorithm.h"
#include "MantidKernel/System.h"
#include <string>
#include <vector>

namespace Mantid {
namespace DataHandling {

/** DetermineChunking : Workflow algorithm to determine chunking

  @date 2012-01-30
*/
/// Make the code clearer by having this an explicit type
using PixelType = int;
/// Type for the DAS time of flight (data file)
using DasTofType = int;

/// Structure that matches the form in the binary event list.
#pragma pack(push, 4) // Make sure the structure is 8 bytes.
struct DasEvent {
  /// Time of flight.
  DasTofType tof;
  /// Pixel identifier as published by the DAS/DAE/DAQ.
  PixelType pid;
};
#pragma pack(pop)
/// Allowed file types

enum FileType {
  PRENEXUS_FILE,    ///< PreNeXus files
  EVENT_NEXUS_FILE, ///< Event NeXus files
  HISTO_NEXUS_FILE, ///< Histogram NeXus files
  RAW_FILE          ///< ISIS raw files
};

class DLLExport DetermineChunking : public API::ParallelAlgorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Workflow algorithm to determine chunking strategy for event nexus, "
           "runinfo.xml, raw, or histo nexus files.";
  }

  int version() const override;
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
  std::string setTopEntryName(std::string filename);
  FileType getFileType(const std::string &filename);
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_DetermineChunking_H_ */
