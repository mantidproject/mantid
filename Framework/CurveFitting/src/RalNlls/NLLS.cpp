#include "MantidCurveFitting/FortranDefs.h"
#include "MantidCurveFitting/RalNlls/Internal.h"

#include <functional>
#include <iostream>

namespace Mantid {
namespace CurveFitting {
namespace RalNlls {

///    ! Perform a single iteration of the RAL_NLLS loop
void nlls_iterate(int n, int m, DoubleFortranVector &X, NLLS_workspace& w,
                  eval_f_type eval_F, eval_j_type eval_J, eval_hf_type eval_HF,
                  params_base_type params, nlls_inform &inform,
                  const nlls_options &options,
                  const DoubleFortranVector &weights) {

  int max_tr_decrease = 100;
  double rho, normFnew, md, Jmax, JtJdiag;

  if (w.first_call == 0) {
    if (n > m) {
      throw std::runtime_error("More parameters than data.");
    }

    w.first_call = 1; // ?

    // evaluate the residual
    eval_F(inform.external_return, n, m, X, w.f, params);
    inform.f_eval = inform.f_eval + 1;
    //          ! set f -> Wf
    gsl_vector_mul(w.f.gsl(), weights.gsl());

    // and evaluate the jacobian
    eval_J(inform.external_return, n, m, X, w.J, params);
    inform.g_eval = inform.g_eval + 1;
    // set J -> WJ
    for (int i = 1; i <= n; ++i) {
      // w.J( (i-1)*m + 1 : i*m) = weights(1:m)*w.J( (i-1)*m + 1 : i*m);
      // TODO: optimise
      for (int j = 1; j <= m; ++j) {
        w.J(j, i) *= weights(j);
      }
    }

    if (options.relative_tr_radius == 1) {
      // first, let's get diag(J^TJ)
      Jmax = 0.0;
      for (int i = 1; i <= n; ++i) {
        // note:: assumes column-storage of J
        // JtJdiag = norm2( w.J( (i-1)*m + 1 : i*m ) );
        JtJdiag = 0.0;
        for (int j = 1; j <= m; ++j) { // for_do(j, 1, m)
          JtJdiag += pow(w.J(j, i), 2);
        }
        JtJdiag = sqrt(JtJdiag);
        if (JtJdiag > Jmax)
          Jmax = JtJdiag;
      }
      w.Delta = options.initial_radius_scale * (pow(Jmax, 2));
    } else {
      w.Delta = options.initial_radius;
    }

    if (options.calculate_svd_J) {
      // calculate the svd of J (if needed)
      get_svd_J(w.J, w.smallest_sv(1), w.largest_sv(1));
    }

    w.normF = norm2(w.f);
    w.normF0 = w.normF;

    // g = -J^Tf
    mult_Jt(w.J, w.f, w.g);
    w.g *= -1.0;
    w.normJF = norm2(w.g);
    w.normJF0 = w.normJF;
    w.normJFold = w.normJF;

    // save some data
    inform.obj = 0.5 * (pow(w.normF, 2));
    inform.norm_g = w.normJF;
    inform.scaled_g = w.normJF / w.normF;

    // if we need to output vectors of the history of the residual
    // and gradient, the set the initial values
    if (options.output_progress_vectors) {
      w.resvec(1) = inform.obj;
      w.gradvec(1) = inform.norm_g;
    }

    // Select the order of the model to be used..
    switch (options.model) {
    case 1: // first-order
    {
      w.hf.zero();
      w.use_second_derivatives = false;
      break;
    }
    case 2: // second order
    {
      if (options.exact_second_derivatives) {
        DoubleFortranVector tmp = w.f;
        tmp *= weights;
        eval_HF(inform.external_return, n, m, X, tmp, w.hf, params);
        inform.h_eval = inform.h_eval + 1;
      } else {
        // S_0 = 0 (see Dennis, Gay and Welsch)
        w.hf.zero();
      }
      w.use_second_derivatives = true;
      break;
    }
    case 3: // hybrid (MNT)
    {
      // set the tolerance :: make this relative
      w.hybrid_tol =
          options.hybrid_tol * (w.normJF / (0.5 * (pow(w.normF, 2))));
      // use first-order method initially
      w.hf.zero();
      w.use_second_derivatives = false;
      if (!options.exact_second_derivatives) {
        // initialize hf_temp too
        w.hf_temp.zero();
      }
      break;
    }
    default:
      throw std::logic_error("Unsupported model.");
    }
  }

  w.iter = w.iter + 1;
  inform.iter = w.iter;

  rho = -one; // intialize rho as a negative value
  bool success = false;
  int no_reductions = 0;

  while (!success) { // loop until successful
    no_reductions = no_reductions + 1;
    if (no_reductions > max_tr_decrease + 1) {
      inform.status = NLLS_ERROR::MAX_TR_REDUCTIONS;
      return;
    }
    std::cerr << "w.Delta=" << w.Delta << std::endl;
    // Calculate the step d that the model thinks we should take next
    calculate_step(w.J, w.f, w.hf, w.g, n, m, w.Delta, w.d, w.normd, options,
                   inform, w.calculate_step_ws);

    std::cerr << "Corrections: " << w.d << std::endl;

    // Accept the step?
    w.Xnew = X;
    w.Xnew += w.d;
    eval_F(inform.external_return, n, m, w.Xnew, w.fnew, params);
    inform.f_eval = inform.f_eval + 1;
    w.fnew *= weights;
    normFnew = norm2(w.fnew);

    // Get the value of the model
    //      md :=   m_k(d)       
    // evaluated at the new step 
    evaluate_model(w.f, w.J, w.hf, w.d, md, m, n, options, w.evaluate_model_ws);

    // Calculate the quantity                                  
    //   rho = 0.5||f||^2 - 0.5||fnew||^2 =   actual_reduction 
    //         --------------------------   -------------------
    //             m_k(0)  - m_k(d)         predicted_reduction
    //                                                         
    // if model is good, rho should be close to one
    calculate_rho(w.normF, normFnew, md, rho, options);
    if (rho > options.eta_successful) {
      success = true;
    }
    std::cerr << "rho: " << w.normF << ' ' << normFnew << ' ' << md << ' ' << rho << std::endl;

    // Update the TR radius
    update_trust_region_radius(rho, options, inform, w);

    if (!success) {
      // finally, check d makes progress
      if (norm2(w.d) < std::numeric_limits<double>::epsilon() * norm2(w.Xnew)) {
        inform.status = NLLS_ERROR::X_NO_PROGRESS;
        return;
      }
    }
  }
  // if we reach here, a successful step has been found

  // update X and f
  X = w.Xnew;
  w.f = w.fnew;

  if (!options.exact_second_derivatives) {
    // first, let's save some old values...
    // g_old = -J_k^T r_k
    w.g_old = w.g;
    // g_mixed = -J_k^T r_{k+1}
    mult_Jt(w.J, w.fnew, w.g_mixed);
    w.g_mixed *= -1.0;
  }

  // evaluate J and hf at the new point
  eval_J(inform.external_return, n, m, X, w.J, params);
  inform.g_eval = inform.g_eval + 1;
  //    if (inform.external_return .ne. 0) goto 4010
  for (int i = 1; i <= n; ++i) { // for_do(i, 1, n)
    // w.J( (i-1)*m + 1 : i*m) = weights(1:m)*w.J( (i-1)*m + 1 : i*m)
    for (int j = 1; j <= m; ++j) { // for_do(j, 1, m)
      w.J(j, i) *= weights(j);
    }
  }

  if (options.calculate_svd_J) {
    get_svd_J(w.J, w.smallest_sv(w.iter + 1), w.largest_sv(w.iter + 1));
  }

  // g = -J^Tf
  mult_Jt(w.J, w.f, w.g);
  w.g *= -1.0;

  w.normJFold = w.normJF;
  w.normF = normFnew;
  w.normJF = norm2(w.g);

  // setup the vectors needed if second derivatives are not available
  if (!options.exact_second_derivatives) {
    w.y = w.g_old;
    w.y -= w.g;
    w.y_sharp = w.g_mixed;
    w.y_sharp -= w.g;
  }

  if (options.model == 3) {
    // hybrid method -- check if we need second derivatives

    if (w.use_second_derivatives) {
      if (w.normJF > w.normJFold) {
        // switch to Gauss-Newton
        w.use_second_derivatives = false;
        // save hf as hf_temp
        w.hf_temp = w.hf;
        w.hf.zero();
      }
    } else {
      auto FunctionValue = 0.5 * (pow(w.normF, 2));
      if (w.normJF / FunctionValue < w.hybrid_tol) {
        w.hybrid_count = w.hybrid_count + 1;
        if (w.hybrid_count == options.hybrid_switch_its) {
          // use (Quasi-)Newton
          w.use_second_derivatives = true;
          w.hybrid_count = 0;
          // copy hf from hf_temp
          if (!options.exact_second_derivatives) {
            w.hf = w.hf_temp;
          }
        }
      } else {
        w.hybrid_count = 0;
      }
    }

    if (!w.use_second_derivatives) {
      // call apply_second_order_info anyway, so that we update the
      // second order approximation
      if (!options.exact_second_derivatives) {
        rank_one_update(w.hf_temp, w, n);
      }
    }
  }

  if (w.use_second_derivatives) {
    apply_second_order_info(n, m, X, w, eval_HF, params, options, inform,
                            weights);
  }

  // update the stats
  inform.obj = 0.5 * (pow(w.normF, 2));
  inform.norm_g = w.normJF;
  inform.scaled_g = w.normJF / w.normF;
  if (options.output_progress_vectors) {
    w.resvec(w.iter + 1) = inform.obj;
    w.gradvec(w.iter + 1) = inform.norm_g;
  }

  // Test convergence
  test_convergence(w.normF, w.normJF, w.normF0, w.normJF0, options, inform);

  if (inform.convergence_normf == 1 || inform.convergence_normg == 1) {
    return; // true;
  }

  inform.iter = w.iter;
  inform.resvec = w.resvec;
  inform.gradvec = w.gradvec;

  return; // false;

} // subroutine nlls_iterate

///-----------------------------------------------------------------------------
///!  RAL_NLLS, a fortran subroutine for finding a first-order critical
///!   point (most likely, a local minimizer) of the nonlinear least-squares
///!   objective function 1/2 ||F(x)||_2^2.
///!
///!  Authors: RAL NA Group (Iain Duff, Nick Gould, Jonathan Hogg, Tyrone Rees,
///!                         Jennifer Scott)
///!
///-----------------------------------------------------------------------------
void nlls_solve(int n, int m, DoubleFortranVector &X, eval_f_type eval_F,
                eval_j_type eval_J, eval_hf_type eval_HF,
                params_base_type params, const nlls_options &options,
                nlls_inform &inform, const DoubleFortranVector &weights) {

  gsl_set_error_handler_off();

  NLLS_workspace w(n, m, options, inform);

  for (int i = 1; i <= options.maxit; ++i) { // for_do(i, 1,options.maxit)

    nlls_iterate(n, m, X, w, eval_F, eval_J, eval_HF, params, inform, options,
                 weights);

    // test the returns to see if we've converged
    if (inform.status != NLLS_ERROR::OK || (inform.convergence_normf == 1) ||
               (inform.convergence_normg == 1)) {
      break;
    }

    if (i == options.maxit) {
      //! If we reach here, we're over maxits
      inform.status = NLLS_ERROR::MAXITS;
    }
  }

}

} // namespace RalNlls
} // CurveFitting
} // Mantid
