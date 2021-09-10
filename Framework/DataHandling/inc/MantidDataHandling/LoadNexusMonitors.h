// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DeprecatedAlgorithm.h"

namespace Mantid {
namespace DataHandling {

/** @class LoadNexusMonitors LoadNexusMonitors.h
DataHandling/LoadNexusMonitors.h

Load Monitors from NeXus files.

Required Properties:
<UL>
<LI> Filename - The name of and path to the input NeXus file </LI>
<LI> Workspace - The name of the workspace to output</LI>
</UL>

@author Michael Reuter, SNS
@date October 25, 2010
*/
class DLLExport LoadNexusMonitors : public API::Algorithm, public API::DeprecatedAlgorithm {
public:
  /// Constructor
  LoadNexusMonitors() { this->useAlgorithm("LoadNexusMonitors", 2); }

  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LoadNexusMonitors"; }

  /// Summary of algorithms purpose
  const std::string summary() const override { return "Load all monitors from a NeXus file into a workspace."; }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Nexus"; }

protected:
  /// Intialisation code
  void init() override;

  /// Execution code
  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid
