// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidAPI/IFunction.h"
#include "MantidCurveFitting/CostFunctions/CostFuncLeastSquares.h"
#include "MantidCurveFitting/EigenFortranDefs.h"
#include "MantidCurveFitting/EigenJacobian.h"
#include "MantidCurveFitting/RalNlls/Workspaces.h"

namespace Mantid {
namespace CurveFitting {
namespace FuncMinimisers {
/** Trust Region minimizer class using the DTRS method of GALAHAD.
 */
class MANTID_CURVEFITTING_DLL TrustRegionMinimizer : public API::IFuncMinimizer {
public:
  /// constructor and destructor
  TrustRegionMinimizer();
  /// Initialize minimizer, i.e. pass a function to minimize.
  void initialize(API::ICostFunction_sptr costFunction, size_t maxIterations = 0) override;
  /// Do one iteration.
  bool iterate(size_t) override;
  /// Return current value of the cost function
  double costFunctionVal() override;
  /// Name of the minimizer.
  std::string name() const override;

private:
  /// Evaluate the fitting function and calculate the residuals.
  void evalF(const DoubleFortranVector &x, DoubleFortranVector &f) const;
  /// Evaluate the Jacobian
  void evalJ(const DoubleFortranVector &x, DoubleFortranMatrix &J) const;
  /// Evaluate the Hessian
  void evalHF(const DoubleFortranVector &x, const DoubleFortranVector &f, DoubleFortranMatrix &h) const;
  /// Find a correction vector to the parameters.
  void calculateStep(const DoubleFortranMatrix &J, const DoubleFortranVector &f, const DoubleFortranMatrix &hf,
                     double Delta, DoubleFortranVector &d, double &normd, const NLLS::nlls_options &options);

  /// Stored cost function
  std::shared_ptr<CostFunctions::CostFuncLeastSquares> m_leastSquares;
  /// Stored to access IFunction interface in iterate()
  API::IFunction_sptr m_function;
  /// Fitting data weights
  DoubleFortranVector m_weights;
  /// Fitting parameters
  DoubleFortranVector m_x;
  /// The Jacobian
  mutable JacobianImpl1 m_J;
  /// Options
  NLLS::nlls_options m_options;
  /// Information about the fitting
  NLLS::nlls_inform m_inform;
  /// Temporary and helper objects
  NLLS::NLLS_workspace m_workspace;

  // Used for calculating step in DTRS method
  DoubleFortranMatrix m_A, m_ev;
  DoubleFortranVector m_ew, m_v, m_v_trans, m_d_trans;
  NLLS::all_eig_symm_work m_all_eig_symm_ws;
  DoubleFortranVector m_scale;
};

} // namespace FuncMinimisers
} // namespace CurveFitting
} // namespace Mantid
