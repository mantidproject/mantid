// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_SETUPEQSANSREDUCTION_H_
#define MANTID_ALGORITHMS_SETUPEQSANSREDUCTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/PropertyManager.h"

namespace Mantid {
namespace WorkflowAlgorithms {
/**
    Set up the reduction options for EQSANS reduction.
*/

class DLLExport SetupEQSANSReduction : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "SetupEQSANSReduction"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Set up EQSANS SANS reduction options.";
  }
  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override { return "Workflow\\SANS"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  std::string _findFile(std::string dataRun);
  void
  setupSensitivity(boost::shared_ptr<Kernel::PropertyManager> reductionManager);
  void setupTransmission(
      boost::shared_ptr<Kernel::PropertyManager> reductionManager);
  void
  setupBackground(boost::shared_ptr<Kernel::PropertyManager> reductionManager);
};

} // namespace WorkflowAlgorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_SETUPEQSANSREDUCTION_H_*/
