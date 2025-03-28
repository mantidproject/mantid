// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataHandling/DllConfig.h"
#include <MantidAPI/WorkspaceGroup_fwd.h>

#include <H5Cpp.h>
#include <filesystem>

namespace Mantid {
namespace DataHandling {

/** SaveNXcanSASBase : Base class to save a reduced workspace in the NXcanSAS format. Depending on the derived
 * algorithm, it contains member that store standard SANS reduced data in 1D or 2D from group or matrix workspaces or
 * polarized SANS reduced data in 1D or 2D from group workspaces.
 */
class MANTID_DATAHANDLING_DLL SaveNXcanSASBase : public API::Algorithm {
protected:
  void addStandardMetadata(const Mantid::API::MatrixWorkspace_sptr &workspace, H5::Group &sasEntry) const;
  void addPolarizedMetadata(const Mantid::API::MatrixWorkspace_sptr &workspace, H5::Group &sasEntry) const;
  void addData(H5::Group &group, const Mantid::API::MatrixWorkspace_sptr &workspace) const;
  void addPolarizedData(H5::Group &group, const Mantid::API::WorkspaceGroup_sptr &wsGroup) const;
  H5::Group addSasEntry(H5::H5File &file, const Mantid::API::MatrixWorkspace_sptr &workspace,
                        const std::string &suffix) const;

  void initStandardProperties();
  void initPolarizedProperties();
  std::map<std::string, std::string> validateStandardInputs() const;
  std::map<std::string, std::string> validatePolarizedInputs() const;

  void saveSingleWorkspaceFile(const API::MatrixWorkspace_sptr &workspace, const std::filesystem::path &path) const;
  void savePolarizedGroup(const API::WorkspaceGroup_sptr &wsGroup, const std::filesystem::path &path) const;

  std::unique_ptr<API::Progress> m_progress = nullptr;

private:
  std::map<std::string, std::vector<std::string>> createPolarizedComponentMap() const;
};

} // namespace DataHandling
} // namespace Mantid
