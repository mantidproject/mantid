// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataHandling/DllConfig.h"
#include "MantidDataHandling/SaveNXcanSASBase.h"

namespace Mantid {
namespace DataHandling {

/** SavePolarizedNXcanSAS : Extends SaveNXcanSAS adding metadata for polarized SANS measurements.
 */
class MANTID_DATAHANDLING_DLL SavePolarizedNXcanSAS final : public SaveNXcanSASBase {
public:
  /// Constructor
  SavePolarizedNXcanSAS();
  /// Virtual dtor
  ~SavePolarizedNXcanSAS() override = default;
  const std::string name() const override { return "SavePolarizedNXcanSAS"; }
  const std::string summary() const override {
    return "Save a Group Workspace with reduced SANS polarized data in NXcanSAS Format.";
  }

  /// InputWorkspace property only accepts workspace groups so we don't want the base algorithm class to process
  /// them as it will done in exec.
  bool checkGroups() override { return false; }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"SaveNXcanSAS", "LoadNXcanSAS"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling\\Nexus"; }

  std::map<std::string, std::string> validateInputs() override;

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid
