// This code was originally translated from Fortran code on
// https://ccpforge.cse.rl.ac.uk/gf/project/ral_nlls June 2016
#include "MantidCurveFitting/RalNlls/Workspaces.h"

namespace Mantid {
namespace CurveFitting {
namespace NLLS {

/// Constructor of the workspace.
NLLS_workspace::NLLS_workspace()
    : first_call(0), iter(0), normF0(), normJF0(), normF(), normJF(),
      normJFold(), normJF_Newton(), Delta(), normd(),
      use_second_derivatives(false), hybrid_count(0), hybrid_tol(1.0), tr_p(7) {
}

/// Initialize the workspace.
/// @param n :: The number of fitting parameters.
/// @param m :: The number of data points.
/// @param options :: The options.
void NLLS_workspace::initialize(int n, int m, const nlls_options &options) {

  tr_nu = options.radius_increase;

  J.allocate(m, n);
  f.allocate(m);
  fnew.allocate(m);
  hf.allocate(n, n);
  d.allocate(n);
  g.allocate(n);
  Xnew.allocate(n);
  y.allocate(n);
  y.zero();
  y_sharp.allocate(n);
  y_sharp.zero();

  if (!options.exact_second_derivatives) {
    g_old.allocate(n);
    g_mixed.allocate(n);
    Sks.allocate(n);
    ysharpSks.allocate(n);
  }

  if (options.output_progress_vectors) {
    resvec.allocate(options.maxit + 1);
    gradvec.allocate(options.maxit + 1);
  }

  if (options.calculate_svd_J) {
    largest_sv.allocate(options.maxit + 1);
    smallest_sv.allocate(options.maxit + 1);
  }

  if (options.model == 3) {
    hf_temp.allocate(n, n);
  }
}

} // namespace NLLS
} // namespace CurveFitting
} // namespace Mantid
