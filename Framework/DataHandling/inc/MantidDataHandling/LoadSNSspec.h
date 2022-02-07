// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidAPI/IFileLoader.h"
#include "MantidKernel/FileDescriptor.h"

namespace Mantid {
namespace DataHandling {
/**
Loads a workspace from an SNS spec file. Spectra must be stored in columns.

Properties:
<ul>
<li>Filename  - the name of the file to read from.</li>
<li>Workspace - the workspace name that will be created and hold the loaded
data.</li>
<li>Separator - the column separation character: comma
(default),tab,space,colon,semi-colon.</li>
<li>Unit      - the unit to assign to the X axis (default: Energy).</li>
</ul>

@author Jean Bilheux, ORNL
@date 08/27/10
*/
class DLLExport LoadSNSspec : public API::IFileLoader<Kernel::FileDescriptor>, API::DeprecatedAlgorithm {
public:
  LoadSNSspec();
  const std::string name() const override { return "LoadSNSspec"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads data from a text file and stores it in a 2D workspace "
           "(Workspace2D class).";
  }

  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"LoadSpec"}; }
  const std::string category() const override { return "DataHandling\\Text"; }

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::FileDescriptor &descriptor) const override;

private:
  void init() override;
  void exec() override;

  /// Allowed values for the cache property
  std::vector<std::string> m_seperator_options;
  std::map<std::string, const char *> m_separatormap;          ///< a map of seperators
  using separator_pair = std::pair<std::string, const char *>; ///< serparator pair type def
};

} // namespace DataHandling
} // namespace Mantid
