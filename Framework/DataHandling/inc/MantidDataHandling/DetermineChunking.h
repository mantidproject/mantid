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

  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
