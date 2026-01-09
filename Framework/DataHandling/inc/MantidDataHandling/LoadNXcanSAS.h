// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFileLoader.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataHandling/NXcanSASUtil.h"
#include "MantidNexus/NexusDescriptorLazy.h"

namespace H5 {
class Group;
class DataSet;
} // namespace H5

namespace Mantid::DataHandling::NXcanSAS {
/** LoadNXcanSAS : Tries to load an NXcanSAS file type into a Workspace2D.
 *  This can load either 1D or 2D data
 */
class MANTID_DATAHANDLING_DLL LoadNXcanSAS : public API::IFileLoader<Nexus::NexusDescriptorLazy> {
public:
  /// Constructor
  LoadNXcanSAS();
  /// Virtual dtor
  ~LoadNXcanSAS() override = default;
  const std::string name() const override { return "LoadNXcanSAS"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Loads an HDF5 NXcanSAS file into a MatrixWorkspace."; }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"LoadCanSAS1D", "SaveNXcanSAS"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling\\Nexus"; }

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Nexus::NexusDescriptorLazy &descriptor) const override;

private:
  /// Loads the transmission runs
  void loadTransmission(const H5::Group &entry, const std::string &name, const InstrumentNameInfo &instrumentInfo);
  /// General data loader, uses m_dataDims
  void loadData(const H5::Group &dataGroup, const Mantid::API::MatrixWorkspace_sptr &workspace,
                const std::pair<size_t, size_t> &spinIndexPair) const;
  void loadMetadata(const H5::Group &group, const Mantid::API::MatrixWorkspace_sptr &workspace,
                    const InstrumentNameInfo &instrumentInfo, const std::optional<API::Sample> &sample,
                    bool hasPolarizedData = false) const;

  /// Prepares data to be loaded based on type and dimensionality
  Mantid::API::WorkspaceGroup_sptr transferFileDataIntoWorkspace(const H5::Group &group,
                                                                 const DataSpaceInformation &dataInfo,
                                                                 const InstrumentNameInfo &instrumentInfo);
  std::vector<SpinState> prepareDataDimensions(const H5::Group &group, const DataSpaceInformation &dataInfo);

  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  std::unique_ptr<API::Progress> m_progress;
  std::unique_ptr<NXcanSAS::DataDimensions> m_dataDims;
};
} // namespace Mantid::DataHandling::NXcanSAS
