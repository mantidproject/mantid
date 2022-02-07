// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidAPI/IFileLoader.h"
#include "MantidKernel/FileDescriptor.h"

namespace Mantid {
namespace DataHandling {
/**
  Loads an SPE format file into a Mantid workspace.

  Required properties:
  <UL>
  <LI> Filename - The SPE format file to be read </LI>
  <LI> Workspace - The name to give to the output workspace </LI>
  </UL>

  @author Russell Taylor, Tessella plc
  @date 02/02/2010
 */
class DLLExport LoadSPE : public API::IFileLoader<Kernel::FileDescriptor> {
public:
  /// Algorithm's name
  const std::string name() const override { return "LoadSPE"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Loads a file written in the spe format."; }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"SaveSPE"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling\\SPE;Inelastic\\DataHandling"; }
  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::FileDescriptor &descriptor) const override;

private:
  // Initialisation code
  void init() override;
  // Execution code
  void exec() override;

  void readHistogram(FILE *speFile, const API::MatrixWorkspace_sptr &workspace, size_t index);
  void reportFormatError(const std::string &what);

  std::string m_filename; ///< The file to load
};

} // namespace DataHandling
} // namespace Mantid
