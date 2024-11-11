// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataHandling/LoadNexusProcessed.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidIndexing/SpectrumNumber.h"
#include "MantidKernel/NexusDescriptor.h"
#include <string>
#include <unordered_map>

namespace Mantid {
namespace API {
class Workspace;
class MatrixWorkspace;
} // namespace API
namespace NeXus {
class NXEntry;
}
namespace DataHandling {
/// Layout information relating to detector-spectra mappings
enum class InstrumentLayout { Mantid, NexusFormat, NotRecognised };

/** LoadNexusProcessed2 : Second variation of LoadNexusProcess, built to handle
 * ESS file specifics in addition to existing behaviour for standard Mantid
 * Processed files.
 *
 * The majority of the implementation consists of function overrides for
 * specific virtual functions in the base Algorithm LoadNexusProcessed
 */
class MANTID_DATAHANDLING_DLL LoadNexusProcessed2 : public LoadNexusProcessed {
public:
  // algorithm "name" is still "LoadNexusProcessed" (not "LoadNexusProcessed2"):
  //   `cppcheck` has an issue with any "useless" override.
  // const std::string name() const override;

  int version() const override;
  int confidence(Kernel::NexusHDF5Descriptor &descriptor) const override;

private:
  void readSpectraToDetectorMapping(Mantid::NeXus::NXEntry &mtd_entry, Mantid::API::MatrixWorkspace &ws) override;

  /// Load nexus geometry and apply to workspace
  bool loadNexusGeometry(Mantid::API::Workspace &ws, size_t entryNumber, Kernel::Logger &logger,
                         const std::string &filePath) override;

  /// Extract mapping information where it is build across NXDetectors
  void extractMappingInfoNew(const Mantid::NeXus::NXEntry &mtd_entry);

  InstrumentLayout m_instrumentLayout = InstrumentLayout::Mantid;

  // Local cache vectors:
  //   spectral mapping information is accumulated before
  //   the instrument geometry has been completely loaded.
  //
  // The key is the NXentry-group name (in order to allow for group workspaces).
  std::unordered_map<std::string, std::vector<Indexing::SpectrumNumber>> m_spectrumNumberss;
  std::unordered_map<std::string, std::vector<Mantid::detid_t>> m_detectorIdss;
  std::unordered_map<std::string, std::vector<int>> m_detectorCountss;
};

} // namespace DataHandling
} // namespace Mantid
