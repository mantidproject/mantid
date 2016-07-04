#ifndef CURVEFITTING_RAL_NLLS_TRUST_REGION_H_
#define CURVEFITTING_RAL_NLLS_TRUST_REGION_H_

#include "MantidCurveFitting/DllConfig.h"
#include "MantidCurveFitting/FortranDefs.h"
#include "MantidCurveFitting/RalNlls/Workspaces.h"

namespace Mantid {
namespace CurveFitting {
namespace NLLS {

void matmult_inner(const DoubleFortranMatrix &J, DoubleFortranMatrix &A);
void get_svd_J(const DoubleFortranMatrix &J, double &s1, double &sn);
double norm2(const DoubleFortranVector &v);
void mult_J(const DoubleFortranMatrix &J, const DoubleFortranVector &x,
            DoubleFortranVector &Jx);
void mult_Jt(const DoubleFortranMatrix &J, const DoubleFortranVector &x,
             DoubleFortranVector &Jtx);
double evaluate_model(const DoubleFortranVector &f,
                      const DoubleFortranMatrix &J,
                      const DoubleFortranMatrix &hf,
                      const DoubleFortranVector &d, const nlls_options &options,
                      evaluate_model_work &w);
double calculate_rho(double normf, double normfnew, double md,
                     const nlls_options &options);
void update_trust_region_radius(double &rho, const nlls_options &options,
                                nlls_inform &inform, NLLS_workspace &w);
void rank_one_update(DoubleFortranMatrix &hf, NLLS_workspace &w);
void test_convergence(double normF, double normJF, double normF0,
                      double normJF0, const nlls_options &options,
                      nlls_inform &inform);
void apply_scaling(const DoubleFortranMatrix &J, DoubleFortranMatrix &A,
                   DoubleFortranVector &v, apply_scaling_work &w,
                   const nlls_options &options, nlls_inform &inform);
void all_eig_symm(const DoubleFortranMatrix &A, DoubleFortranVector &ew,
                  DoubleFortranMatrix &ev);
// void apply_second_order_info(int n, int m, const DoubleFortranVector& X,
// NLLS_workspace& w, eval_hf_type eval_HF, params_base_type params,
//  const nlls_options& options, nlls_inform& inform, const DoubleFortranVector&
//  weights);

} // NLLS
} // CurveFitting
} // Mantid

#endif // CURVEFITTING_RAL_NLLS_TRUST_REGION_H_
