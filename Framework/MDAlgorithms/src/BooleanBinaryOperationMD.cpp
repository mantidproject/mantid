// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/BooleanBinaryOperationMD.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid::MDAlgorithms {

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string BooleanBinaryOperationMD::name() const { return "BooleanBinaryOperationMD"; }

/// Algorithm's version for identification. @see Algorithm::version
int BooleanBinaryOperationMD::version() const { return 1; }

//----------------------------------------------------------------------------------------------
///
const std::string BooleanBinaryOperationMD::summary() const {
  std::string algo = this->name();
  algo = algo.substr(0, algo.size() - 2);
  return "Perform the " + algo + " boolean operation on two MDHistoWorkspaces";
}

//----------------------------------------------------------------------------------------------
/// Is the operation commutative?
bool BooleanBinaryOperationMD::commutative() const { return true; }

//----------------------------------------------------------------------------------------------
/// Check the inputs and throw if the algorithm cannot be run
void BooleanBinaryOperationMD::checkInputs() {
  if (m_lhs_event || m_rhs_event)
    throw std::runtime_error("Cannot perform the " + this->name() + " operation on a MDEventWorkspace.");
  if (!acceptScalar() && (m_lhs_scalar || m_rhs_scalar))
    throw std::runtime_error("Cannot perform the " + this->name() + " operation on a WorkspaceSingleValue.");
  if (!this->commutative() && m_lhs_scalar)
    throw std::runtime_error("Cannot perform the " + this->name() + " operation with a scalar on the left-hand side.");
}

//----------------------------------------------------------------------------------------------
/// Run the algorithm with an MDEventWorkspace as output
void BooleanBinaryOperationMD::execEvent() {
  throw std::runtime_error("Cannot perform the " + this->name() + " operation on a MDEventWorkspace.");
}

//----------------------------------------------------------------------------------------------
/// Run the algorithm with a MDHisotWorkspace as output, scalar and operand
void BooleanBinaryOperationMD::execHistoScalar(Mantid::DataObjects::MDHistoWorkspace_sptr /*out*/,
                                               Mantid::DataObjects::WorkspaceSingleValue_const_sptr /*scalar*/) {
  throw std::runtime_error("Cannot perform the " + this->name() + " operation on a WorkspaceSingleValue.");
}

} // namespace Mantid::MDAlgorithms
