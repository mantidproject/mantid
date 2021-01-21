// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidCurveFitting/DllConfig.h"
#include "MantidCurveFitting/FortranDefs.h"
#include "MantidCurveFitting/RalNlls/Workspaces.h"

namespace Mantid {
namespace CurveFitting {
namespace NLLS {

void matmultInner(const DoubleFortranMatrix &J, DoubleFortranMatrix &A);
void getSvdJ(const DoubleFortranMatrix &J, double &s1, double &sn);
double norm2(const DoubleFortranVector &v);
void multJ(const DoubleFortranMatrix &J, const DoubleFortranVector &x, DoubleFortranVector &Jx);
void multJt(const DoubleFortranMatrix &J, const DoubleFortranVector &x, DoubleFortranVector &Jtx);
double evaluateModel(const DoubleFortranVector &f, const DoubleFortranMatrix &J, const DoubleFortranMatrix &hf,
                     const DoubleFortranVector &d, const nlls_options &options, evaluate_model_work &w);
double calculateRho(double normf, double normfnew, double md, const nlls_options &options);
void updateTrustRegionRadius(double &rho, const nlls_options &options, NLLS_workspace &w);
void rankOneUpdate(DoubleFortranMatrix &hf, NLLS_workspace &w);
void testConvergence(double normF, double normJF, double normF0, double normJF0, const nlls_options &options,
                     nlls_inform &inform);
void applyScaling(const DoubleFortranMatrix &J, DoubleFortranMatrix &A, DoubleFortranVector &v,
                  DoubleFortranVector &scale, const nlls_options &options);
void allEigSymm(const DoubleFortranMatrix &A, DoubleFortranVector &ew, DoubleFortranMatrix &ev);
// void apply_second_order_info(int n, int m, const DoubleFortranVector& X,
// NLLS_workspace& w, eval_hf_type evalHF, params_base_type params,
//  const nlls_options& options, nlls_inform& inform, const DoubleFortranVector&
//  weights);

} // namespace NLLS
} // namespace CurveFitting
} // namespace Mantid
