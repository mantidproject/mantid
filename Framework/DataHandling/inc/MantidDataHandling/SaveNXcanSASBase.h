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
#include <H5Cpp.h>

namespace Mantid {
namespace DataHandling {

/** SaveNXcanSASBase : Base class to save a reduced workspace in the NXcanSAS format. Currently
 * only MatrixWorkspaces resulting from
 * 1D and 2D reductions are supported.
 */
class MANTID_DATAHANDLING_DLL SaveNXcanSASBase : public API::Algorithm {
protected:
  void saveSingleWorkspaceFile(const API::MatrixWorkspace_sptr &workspace, const std::string &filename);
  void addStandardMetadata(const Mantid::API::MatrixWorkspace_sptr &workspace, H5::Group &sasEntry);
  void addData(H5::Group &group, const Mantid::API::MatrixWorkspace_sptr &workspace);
  H5::Group addSasEntry(H5::H5File &file, const Mantid::API::MatrixWorkspace_sptr &workspace,
                        const std::string &suffix);
  void initStandardProperties();
  std::map<std::string, std::string> validateStandardInputs();

  std::unique_ptr<API::Progress> m_progress = nullptr;
};

} // namespace DataHandling
} // namespace Mantid
