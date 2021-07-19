// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
//#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IFileLoader.h"
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/FileDescriptor.h"

namespace Mantid {
namespace DataHandling {

/**
Is a wrapper class to load NeXuS and bin (PSI) files using the LoadMuonNexus and LoadPSIMuonBin algorithms.
*/
class DLLExport LoadMuonData : public API::IFileLoader<Kernel::FileDescriptor> {
  const std::string name() const override { return "LoadMuonData"; }
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"LoadMuonNexus", "LoadMuonNexusV2", "LoadPSIMuonBin"};
  }
  const std::string category() const override { return "DataHandling;Muon\\DataHandling"; }
  const std::string summary() const override {
    return "The LoadMuonData algorithm will read the given file name using LoadMuonNexus or LoadPSIMuonBin depending "
           "on the file type. The result will be used to populate the named output workspace.";
  }
  int confidence(Kernel::FileDescriptor &descriptor) const override;
  bool loadMutipleAsOne() override;

private:
  void init() override;
  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid
