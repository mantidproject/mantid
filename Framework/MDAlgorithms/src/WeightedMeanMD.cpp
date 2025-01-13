// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/WeightedMeanMD.h"
#include "MantidDataObjects/MDHistoWorkspaceIterator.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid::MDAlgorithms {

DECLARE_ALGORITHM(WeightedMeanMD)

//----------------------------------------------------------------------------------------------
/// Is the operation commutative?
bool WeightedMeanMD::commutative() const { return true; }

//----------------------------------------------------------------------------------------------
/// Check the inputs and throw if the algorithm cannot be run
void WeightedMeanMD::checkInputs() {
  if (!m_lhs_histo || !m_rhs_histo)
    throw std::invalid_argument(this->name() + " can only be run on a MDHistoWorkspace.");
}

//----------------------------------------------------------------------------------------------
/// Run the algorithm with a MDHisotWorkspace as output and operand
void WeightedMeanMD::execHistoHisto(Mantid::DataObjects::MDHistoWorkspace_sptr out,
                                    Mantid::DataObjects::MDHistoWorkspace_const_sptr operand) {
  using DataObjects::MDHistoWorkspaceIterator;
  auto lhs = out->createIterator();
  auto lhs_it = dynamic_cast<MDHistoWorkspaceIterator *>(lhs.get());
  auto rhs = operand->createIterator();
  auto rhs_it = dynamic_cast<MDHistoWorkspaceIterator *>(rhs.get());

  if (!lhs_it || !rhs_it) {
    throw std::logic_error("Histo iterators have wrong type.");
  }

  do {
    double lhs_s = lhs_it->getSignal();
    double lhs_err = lhs_it->getError();
    double rhs_s = rhs_it->getSignal();
    double rhs_err = rhs_it->getError();
    double signal = 0;
    double error_sq = 0;
    if ((lhs_err > 0.0) && (rhs_err > 0.0)) {
      double rhs_err_sq = rhs_err * rhs_err;
      double lhs_err_sq = lhs_err * lhs_err;
      double s = (rhs_s / rhs_err_sq) + (lhs_s / lhs_err_sq);
      double e = rhs_err_sq * lhs_err_sq / (rhs_err_sq + lhs_err_sq);
      signal = s * e;
      error_sq = e;
    } else if ((rhs_err > 0) && (lhs_err == 0)) {
      signal = rhs_s;
      error_sq = rhs_err * rhs_err;
    } else if ((lhs_err > 0) && (rhs_err == 0)) {
      signal = lhs_s;
      error_sq = lhs_err * lhs_err;
    }

    size_t pos = lhs_it->getLinearIndex();
    out->setSignalAt(pos, signal);
    out->setErrorSquaredAt(pos, error_sq);
  } while (lhs_it->next() && rhs_it->next());
}

//----------------------------------------------------------------------------------------------
/// Run the algorithm with a MDHisotWorkspace as output, scalar and operand
void WeightedMeanMD::execHistoScalar(Mantid::DataObjects::MDHistoWorkspace_sptr /*out*/,
                                     Mantid::DataObjects::WorkspaceSingleValue_const_sptr /*scalar*/) {
  throw std::runtime_error(this->name() + " can only be run with two MDHistoWorkspaces as inputs");
}

//----------------------------------------------------------------------------------------------
/// Run the algorithm on a MDEventWorkspace
void WeightedMeanMD::execEvent() { throw std::runtime_error(this->name() + " can only be run on a MDHistoWorkspace."); }

} // namespace Mantid::MDAlgorithms
