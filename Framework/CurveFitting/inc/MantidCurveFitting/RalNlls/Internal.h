#ifndef CURVEFITTING_RAL_NLLS_INTERNAL_H_
#define CURVEFITTING_RAL_NLLS_INTERNAL_H_

#include "MantidCurveFitting/DllConfig.h"
#include "MantidCurveFitting/RalNlls/NLLS.h"

#include <limits>
#include <functional>

namespace Mantid {
namespace CurveFitting {
namespace RalNlls {

///  Given an (m x n)  matrix J, this routine returns the largest 
///  and smallest singular values of J.
void get_svd_J(const DoubleFortranMatrix& J, double &s1, double &sn);

/// Calculate the 2-norm of a vector: sqrt(||V||^2)
double norm2(const DoubleFortranVector& v);

void mult_J(const DoubleFortranMatrix& J, const DoubleFortranVector& x, DoubleFortranVector& Jx);
void mult_Jt(const DoubleFortranMatrix& J, const DoubleFortranVector& x, DoubleFortranVector& Jtx);
void calculate_step(const DoubleFortranMatrix& J, const DoubleFortranVector& f, const DoubleFortranMatrix& hf, const DoubleFortranVector& g,
  int n, int m, double Delta, DoubleFortranVector& d, double& normd, const nlls_options& options, nlls_inform& inform, calculate_step_work& w);
void evaluate_model(const DoubleFortranVector& f, const DoubleFortranMatrix& J, const DoubleFortranMatrix& hf,
  const DoubleFortranVector& d, double& md, int m, int n, const nlls_options options, evaluate_model_work& w);
void calculate_rho(double normf, double normfnew,double md, double& rho, const nlls_options& options);
void update_trust_region_radius(double& rho, const nlls_options& options, nlls_inform& inform, NLLS_workspace& w);
void rank_one_update(DoubleFortranMatrix& hf, NLLS_workspace w, int n);
void apply_second_order_info(int n, int m, const DoubleFortranVector& X,
  NLLS_workspace& w, eval_hf_type eval_HF, params_base_type params, 
  const nlls_options& options, nlls_inform& inform, const DoubleFortranVector& weights);
void test_convergence(double normF, double normJF, double normF0, double normJF0,
  const nlls_options& options, nlls_inform& inform);

} // RalNlls
} // CurveFitting
} // Mantid

#endif // CURVEFITTING_RAL_NLLS_INTERNAL_H_
