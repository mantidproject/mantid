// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidKernel/PropertyManager.h"

namespace Mantid {
namespace WorkflowAlgorithms {
/** Beam Finder for SANS instruments
 */

class DLLExport SANSBeamFinder : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "SANSBeamFinder"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Beam finder workflow algorithm for SANS instruments."; }
  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override { return "Workflow\\SANS\\UsesPropertyManager"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  API::MatrixWorkspace_sptr loadBeamFinderFile(const std::string &beamCenterFile);
  void maskEdges(const API::MatrixWorkspace_sptr &beamCenterWS, int high, int low, int left, int right,
                 const std::string &componentName = "detector1");

  std::shared_ptr<Kernel::PropertyManager> m_reductionManager;
  std::string m_output_message;
};

} // namespace WorkflowAlgorithms
} // namespace Mantid
