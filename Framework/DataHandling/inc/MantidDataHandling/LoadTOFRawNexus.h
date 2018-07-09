#ifndef MANTID_DATAHANDLING_LOADTOFRAWNEXUS_H_
#define MANTID_DATAHANDLING_LOADTOFRAWNEXUS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFileLoader.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectraDetectorTypes.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidNexus/NexusClasses.h"

#include <mutex>

namespace Mantid {

namespace DataHandling {
/**
 Loads a NeXus file that conforms to the TOFRaw instrument definition format and
 stores it in a 2D workspace.

 Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
 */
class DLLExport LoadTOFRawNexus
    : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  /// Default Constructor
  LoadTOFRawNexus();

  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LoadTOFRawNexus"; }

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads a NeXus file confirming to the TOFRaw format";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"LoadNexus"};
  }

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Nexus"; }

  static std::string getEntryName(const std::string &filename);

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const override;

  void countPixels(const std::string &nexusfilename,
                   const std::string &entry_name,
                   std::vector<std::string> &bankNames);

  /// Number of pixels
  size_t m_numPixels;

  /// Signal # to load. Default 1
  int m_signalNo;

protected:
  void init() override;
  void exec() override;

  /// Validate the optional input properties
  void checkOptionalProperties();

  /// Run LoadInstrument as a ChildAlgorithm
  void runLoadInstrument(DataObjects::Workspace2D_sptr);

  /// Load in details about the sample
  void loadSampleData(DataObjects::Workspace2D_sptr,
                      Mantid::NeXus::NXEntry &entry);

  void loadBank(const std::string &nexusfilename, const std::string &entry_name,
                const std::string &bankName, API::MatrixWorkspace_sptr WS,
                const detid2index_map &id_to_wi);

  /// List of the absolute time of each pulse
  std::vector<Types::Core::DateAndTime> pulseTimes;

  /// Number of bins
  size_t m_numBins;

  /// Interval of chunk
  specnum_t m_spec_min, m_spec_max;

  /// Name of the 'data' field to load (depending on Signal)
  std::string m_dataField;

  /// Name of the 'axis' field to load (depending on Signal)
  std::string m_axisField;

  /// Units of the X axis found
  std::string m_xUnits;

  /// Mutex to avoid simultaneous file access
  std::mutex m_fileMutex;

  /// Flag for whether or not to assume the data is old SNS raw files;
  bool m_assumeOldFile;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLINGLOADTOFRAWNEXUS_H_ */
