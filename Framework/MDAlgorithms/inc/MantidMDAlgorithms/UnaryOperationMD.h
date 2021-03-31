// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace MDAlgorithms {

/** Abstract base class for unary operations (e.g. Log or Exp)
 * on MDWorkspaces.
 */
class DLLExport UnaryOperationMD : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override { return "Abstract base class for unary operations on MDWorkspaces."; }

protected:
  /// The name of the input workspace property
  virtual const std::string inputPropName() const { return "InputWorkspace"; }
  /// The name of the output workspace property
  virtual const std::string outputPropName() const { return "OutputWorkspace"; }

  void init() override;
  virtual void initExtraProperties();
  void exec() override;

  /// Check the inputs and throw if the algorithm cannot be run
  virtual void checkInputs() = 0;

  /// Run the algorithm on a MDEventWorkspace
  virtual void execEvent(Mantid::API::IMDEventWorkspace_sptr out) = 0;

  /// Run the algorithm with a MDHistoWorkspace
  virtual void execHisto(Mantid::DataObjects::MDHistoWorkspace_sptr out) = 0;

  /// Input workspace
  Mantid::API::IMDWorkspace_sptr m_in;

  /// Input workspace (MDEvent)
  Mantid::API::IMDEventWorkspace_sptr m_in_event;

  /// Input workspace (MDHisto)
  Mantid::DataObjects::MDHistoWorkspace_sptr m_in_histo;

  /// Output workspace
  Mantid::API::IMDWorkspace_sptr m_out;
};

} // namespace MDAlgorithms
} // namespace Mantid
