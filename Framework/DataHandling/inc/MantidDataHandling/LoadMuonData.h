// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/DllConfig.h"

namespace Mantid {
namespace DataHandling {

class DLLExport LoadMuonData : public API::Algorithm {
  const std::string name() const override { return "LoadMuonData"; };
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"LoadMuonNexus", "LoadMuonNexusV2", "LoadPSIMuonBin"};
  }
  const std::string category() const override { return "DataHandling;Muon\\DataHandling"; };
  const std::string summary() const override {
    return "The LoadMuonData algorithm will read the given file name using LoadMuonNexus or LoadPSIMuonBin depending "
           "on the file type. The result will be used to populate the named output workspace.";
  };

private:
  void init() override;
  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid
