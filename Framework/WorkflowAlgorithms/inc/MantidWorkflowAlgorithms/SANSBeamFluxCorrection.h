// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DataProcessorAlgorithm.h"

namespace Mantid {
namespace WorkflowAlgorithms {
/**
    Performs beam flux correction on TOF SANS data.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport SANSBeamFluxCorrection : public API::DataProcessorAlgorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "SANSBeamFluxCorrection"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Performs beam flux correction on TOF SANS data."; }
  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"SANSSolidAngleCorrection"}; }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "Workflow\\SANS\\UsesPropertyManager;"
           "CorrectionFunctions\\InstrumentCorrections";
  }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  void execEvent();
  API::MatrixWorkspace_sptr loadReference();
  std::shared_ptr<Kernel::PropertyManager> m_reductionManager;
  std::string m_output_message;
};

} // namespace WorkflowAlgorithms
} // namespace Mantid
