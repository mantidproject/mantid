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
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"

namespace Mantid {
namespace MDAlgorithms {

/** Abstract base class for binary operations on IMDWorkspaces,
  e.g. A = B + C or A = B / C.

  Handles most of the validation and delegates to a handful of execXXX
  functions.

  This will be subclassed by, e.g. PlusMD, MinusMD, MultiplyMD, etc.

  @author Janik Zikovsky
  @date 2011-11-04
*/
class DLLExport BinaryOperationMD : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override {
    return "Abstract base class for binary operations on IMDWorkspaces, e.g. A "
           "= B + C or A = B / C.";
  }

protected:
  /// Is the operation commutative?
  virtual bool commutative() const = 0;

  /// Check the inputs and throw if the algorithm cannot be run
  virtual void checkInputs() = 0;

  /// Run the algorithm with an MDEventWorkspace as output
  virtual void execEvent() = 0;

  /// Run the algorithm with a MDHisotWorkspace as output and operand
  virtual void execHistoHisto(Mantid::DataObjects::MDHistoWorkspace_sptr out,
                              Mantid::DataObjects::MDHistoWorkspace_const_sptr operand) = 0;

  /// Run the algorithm with a MDHisotWorkspace as output, scalar and operand
  virtual void execHistoScalar(Mantid::DataObjects::MDHistoWorkspace_sptr out,
                               Mantid::DataObjects::WorkspaceSingleValue_const_sptr scalar) = 0;

  /// The name of the first input workspace property
  virtual std::string inputPropName1() const { return "LHSWorkspace"; }
  /// The name of the second input workspace property
  virtual std::string inputPropName2() const { return "RHSWorkspace"; }
  /// The name of the output workspace property
  virtual std::string outputPropName() const { return "OutputWorkspace"; }

  void init() override;
  virtual void initExtraProperties();
  void exec() override;

  /// LHS workspace
  Mantid::API::IMDWorkspace_sptr m_lhs;
  /// RHS workspace
  Mantid::API::IMDWorkspace_sptr m_rhs;
  /// Output workspace
  Mantid::API::IMDWorkspace_sptr m_out;

  /// For checkInputs
  Mantid::API::IMDEventWorkspace_sptr m_lhs_event;
  Mantid::API::IMDEventWorkspace_sptr m_rhs_event;
  Mantid::DataObjects::MDHistoWorkspace_sptr m_lhs_histo;
  Mantid::DataObjects::MDHistoWorkspace_sptr m_rhs_histo;
  Mantid::DataObjects::WorkspaceSingleValue_sptr m_lhs_scalar;
  Mantid::DataObjects::WorkspaceSingleValue_sptr m_rhs_scalar;

  /// Operand MDEventWorkspace
  Mantid::API::IMDEventWorkspace_sptr m_operand_event;
  /// Output MDEventWorkspace
  Mantid::API::IMDEventWorkspace_sptr m_out_event;

  /// Operand MDHistoWorkspace
  Mantid::DataObjects::MDHistoWorkspace_sptr m_operand_histo;
  /// Output MDHistoWorkspace
  Mantid::DataObjects::MDHistoWorkspace_sptr m_out_histo;

  /// Operand WorkspaceSingleValue
  Mantid::DataObjects::WorkspaceSingleValue_sptr m_operand_scalar;
};

} // namespace MDAlgorithms
} // namespace Mantid
