// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataHandling/DllConfig.h"

namespace Mantid {
namespace DataHandling {
/** @class Mantid::DataHandling::LoadPreNeXusMonitors

    A data loading routine for SNS PreNeXus beam monitor (histogram) files.

    @author Stuart Campbell, SNS ORNL
    @date 20/08/2010
*/
class MANTID_DATAHANDLING_DLL LoadPreNexusMonitors : public Mantid::API::Algorithm, public API::DeprecatedAlgorithm {
public:
  /// (Empty) Constructor
  LoadPreNexusMonitors();
  /// Algorithm's name
  const std::string name() const override { return "LoadPreNexusMonitors"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "This is a routine to load in the beam monitors from SNS preNeXus "
           "files into a workspace.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"LoadEventPreNexus", "LoadPreNexus"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling\\PreNexus"; }
  /// Algorithm's aliases
  const std::string alias() const override { return "LoadPreNeXusMonitors"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  /// Number of monitors
  int nMonitors;

  /// Set to true when instrument geometry was loaded.
  bool instrument_loaded_correctly;

  void runLoadInstrument(const std::string &instrument, const API::MatrixWorkspace_sptr &localWorkspace);
};

} // namespace DataHandling
} // namespace Mantid
