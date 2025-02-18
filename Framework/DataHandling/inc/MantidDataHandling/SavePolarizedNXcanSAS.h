// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataHandling/DllConfig.h"
#include "MantidDataHandling/SaveNXcanSASBase.h"

namespace Mantid {
namespace DataHandling {

/** SavePolarizedNXcanSAS : Saves a polarized reduced workspace in the NXcanSAS format. Currently
 * only MatrixWorkspaces resulting from 1D and 2D reductions are supported.
 */
class MANTID_DATAHANDLING_DLL SavePolarizedNXcanSAS final : public SaveNXcanSASBase {
public:
  /// Constructor
  SavePolarizedNXcanSAS();
  /// Virtual dtor
  ~SavePolarizedNXcanSAS() override = default;
  const std::string name() const override { return "SavePolarizedNXcanSAS"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Save a MatrixWorkspace to a file in the NXcanSAS format (for both 1D and 2D data).";
  }
  /// Override processGroups
  bool processGroups() override;

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"SaveCanSAS1D", "LoadNXcanSAS"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling\\Nexus"; }

  std::map<std::string, std::string> validateInputs() override;

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  void processAllWorkspaces();

  std::vector<API::MatrixWorkspace_sptr> m_workspaces;
};

} // namespace DataHandling
} // namespace Mantid
