// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataHandling/DllConfig.h"

#include <map>
#include <string>
#include <vector>

namespace Mantid {
namespace Nexus {
class File;
}
namespace DataHandling {

/**
 * Saves a single-period muon workspace to an ISIS Muon NeXus V2 (HDF5) file, which can be loaded again with
 * LoadMuonNexusV2 and used in the Muon Analysis interface.
 */
class MANTID_DATAHANDLING_DLL SaveNexusMuon final : public API::Algorithm {
public:
  const std::string name() const override { return "SaveNexusMuon"; }
  const std::string summary() const override {
    return "Saves a muon workspace to a Muon NeXus V2 file that can be loaded by LoadMuonNexus and the Muon Analysis "
           "interface.";
  }
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"LoadMuonNexusV2", "LoadMuonNexus"}; }
  const std::string category() const override { return R"(DataHandling\Nexus;Muon\DataHandling)"; }
  std::map<std::string, std::string> validateInputs() override;

private:
  void init() override;
  void exec() override;
  /// Workspace groups are rejected in validateInputs rather than processed member by member
  bool checkGroups() override { return false; }

  void validateWorkspace(const API::MatrixWorkspace &ws, std::map<std::string, std::string> &issues) const;
  void validateDeadTimeTable(const API::MatrixWorkspace &ws, std::map<std::string, std::string> &issues) const;
  void validateGroupingTable(const API::MatrixWorkspace &ws, std::map<std::string, std::string> &issues) const;
  double timeZeroIf(const std::string &flagProperty) const;
  double absoluteFirstGoodData(const API::MatrixWorkspace &ws) const;
  double absoluteLastGoodData(const API::MatrixWorkspace &ws) const;
  std::string mainFieldDirection(const API::MatrixWorkspace &ws) const;

  void writeRunDetails(Nexus::File &file, const API::MatrixWorkspace &ws) const;
  void writeInstrument(Nexus::File &file, const API::MatrixWorkspace &ws) const;
  void writeSample(Nexus::File &file, const API::MatrixWorkspace &ws) const;
  void writePeriods(Nexus::File &file, const API::MatrixWorkspace &ws) const;
  void writeMeasurement(Nexus::File &file, const API::MatrixWorkspace &ws) const;
  void writeLogs(Nexus::File &file, const API::MatrixWorkspace &ws) const;
};

} // namespace DataHandling
} // namespace Mantid
