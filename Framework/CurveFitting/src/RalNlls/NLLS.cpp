//! ral_nlls_double :: a nonlinear least squares solver
#include "MantidCurveFitting/FortranDefs.h"
#include "MantidCurveFitting/RalNlls/Internal.h"

#include <functional>

#define for_do(i, a, n) for(int i=a; i <=n; ++i) {
#define then {
#define end_if }
#define end_do }

namespace Mantid {
namespace CurveFitting {
namespace RalNlls {

///    ! Perform a single iteration of the RAL_NLLS loop
void nlls_iterate(int n, int m, DoubleFortranVector& X,
                          NLLS_workspace w,
                          eval_f_type eval_F, eval_j_type eval_J, eval_hf_type eval_HF,
                          params_base_type params,
                          nlls_inform &inform, const nlls_options& options, const DoubleFortranVector& weights) {

//    INTEGER, INTENT( IN ) :: n, m
//    REAL( wp ), DIMENSION( n ), INTENT( INOUT ) :: X
//    TYPE( nlls_inform ), INTENT( INOUT ) :: inform
//    TYPE( nlls_options ), INTENT( IN ) :: options
//    type( NLLS_workspace ), INTENT( INOUT ) :: w
//    procedure( eval_f_type ) :: eval_F
//    procedure( eval_j_type ) :: eval_J
//    procedure( eval_hf_type ) :: eval_HF
//    class( params_base_type ) :: params
//    REAL( wp ), DIMENSION( m ), INTENT( IN ), optional :: weights
//      
//    integer :: svdstatus = 0
    integer max_tr_decrease = 100;
    real rho, normFnew, md, Jmax, JtJdiag;
//    real(wp) :: FunctionValue
//    logical success;
//    character :: second
//    
//    ! todo: make max_tr_decrease a control variable
//
//    
    if (w.first_call == 1) {
//       !!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!!
//       !! This is the first call...allocate arrays, and get initial !!
//       !! function evaluations                                      !!
//       !!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!!
//       ! first, check if n < m
       if (n > m) {
         throw std::runtime_error("More parameters than data.");
       }
       
//       ! allocate space for vectors that will be used throughout the algorithm
//       call setup_workspaces(w,n,m,options,inform)

//       ! evaluate the residual
       eval_F(inform.external_return, n, m, X, w.f, params);
       inform.f_eval = inform.f_eval + 1;
//          ! set f -> Wf
       gsl_vector_mul(w.f.gsl(), weights.gsl());

//       ! and evaluate the jacobian
       eval_J(inform.external_return, n, m, X, w.J, params);
       inform.g_eval = inform.g_eval + 1;
//          ! set J -> WJ
          for_do(i, 1, n)
             //w.J( (i-1)*m + 1 : i*m) = weights(1:m)*w.J( (i-1)*m + 1 : i*m);
            // TODO: optimise
            for_do(j, 1, m)
              w.J(j, i) *= weights(j);
            end_do
          end_do
       
       if (options.relative_tr_radius == 1) then
//          ! first, let's get diag(J^TJ)
          Jmax = 0.0;
          for_do(i, 1, n)
//             ! note:: assumes column-storage of J
             //JtJdiag = norm2( w.J( (i-1)*m + 1 : i*m ) );
            JtJdiag = 0.0;
            for_do(j, 1, m)
              JtJdiag += pow(w.J(j, i), 2);
            end_do
            JtJdiag = sqrt(JtJdiag);
            if (JtJdiag > Jmax) Jmax = JtJdiag;
          end_do
          w.Delta = options.initial_radius_scale * (pow(Jmax, 2));
       } else {
          w.Delta = options.initial_radius;
       end_if

       if ( options.calculate_svd_J ) then
//          ! calculate the svd of J (if needed)
          get_svd_J(w.J,w.smallest_sv(1), w.largest_sv(1));
       end_if

       w.normF = norm2(w.f);
       w.normF0 = w.normF;

//       !    g = -J^Tf
       mult_Jt(w.J, w.f, w.g);
       w.g *= -1.0;
       w.normJF = norm2(w.g);
       w.normJF0 = w.normJF;
       w.normJFold = w.normJF;

//       ! save some data 
       inform.obj = 0.5 * ( pow(w.normF, 2) );
       inform.norm_g = w.normJF;
       inform.scaled_g = w.normJF / w.normF;

//       ! if we need to output vectors of the history of the residual
//       ! and gradient, the set the initial values
       if (options.output_progress_vectors) then
          w.resvec(1) = inform.obj;
          w.gradvec(1) = inform.norm_g;
       end_if
       
//       !! Select the order of the model to be used..
       switch (options.model) {
       case 1: // ! first-order
       {
          w.hf.zero();
          w.use_second_derivatives = false;
          break;
       }
       case 2: // ! second order
       {
          if ( options.exact_second_derivatives ) then
            DoubleFortranVector tmp = w.f;
            tmp *= weights;
             eval_HF(inform.external_return, n, m, X, tmp, w.hf, params);
             inform.h_eval = inform.h_eval + 1;
          }else{
             //! S_0 = 0 (see Dennis, Gay and Welsch)
             w.hf.zero();
          end_if
          w.use_second_derivatives = true;
          break;
       }
       case 3: // ! hybrid (MNT)
       {
          //! set the tolerance :: make this relative
          w.hybrid_tol = options.hybrid_tol * ( w.normJF/(0.5*(pow(w.normF, 2))) );
          //! use first-order method initially
          w.hf.zero();
          w.use_second_derivatives = false;
          if (! options.exact_second_derivatives) then 
             //! initialize hf_temp too 
             w.hf_temp.zero();
          end_if
          break;
       }
       default:
          throw std::logic_error("Unsupported model.");
       }
 
    }

    w.iter = w.iter + 1;
    inform.iter = w.iter;
    
    rho  = -one; //! intialize rho as a negative value
    bool success = false;
    int no_reductions = 0;

    while (! success) { //! loop until successful
       no_reductions = no_reductions + 1;
       if (no_reductions > max_tr_decrease+1) goto L4050;
//
//       !+++++++++++++++++++++++++++++++++++++++++++!
//       ! Calculate the step                        !
//       !    d                                      !   
//       ! that the model thinks we should take next !
//       !+++++++++++++++++++++++++++++++++++++++++++!
       calculate_step(w.J,w.f,w.hf,w.g,n,m,w.Delta,w.d,w.normd,options,inform,
            w.calculate_step_ws);
       
//       !++++++++++++++++++!
//       ! Accept the step? !
//       !++++++++++++++++++!
       w.Xnew = X;
       w.Xnew += w.d;
       eval_F(inform.external_return, n, m, w.Xnew, w.fnew, params);
       inform.f_eval = inform.f_eval + 1;
       w.fnew *= weights;
       normFnew = norm2(w.fnew);

//       !++++++++++++++++++++++++++++!
//       ! Get the value of the model !
//       !      md :=   m_k(d)        !
//       ! evaluated at the new step  !
//       !++++++++++++++++++++++++++++!
       evaluate_model(w.f,w.J,w.hf,w.d,md,m,n,options,w.evaluate_model_ws);
       
//       !++++++++++++++++++++++++++++++++++++++++++++++++++++++++++!
//       ! Calculate the quantity                                   ! 
//       !   rho = 0.5||f||^2 - 0.5||fnew||^2 =   actual_reduction  !
//       !         --------------------------   ------------------- !
//       !             m_k(0)  - m_k(d)         predicted_reduction !
//       !                                                          !
//       ! if model is good, rho should be close to one             !
//       !+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
       calculate_rho(w.normF,normFnew,md,rho,options);
       if (rho > options.eta_successful) success = true;

//       !++++++++++++++++++++++!
//       ! Update the TR radius !
//       !++++++++++++++++++++++!
       update_trust_region_radius(rho,options,inform,w);
       
       if (! success) then
//          ! finally, check d makes progress
          if ( norm2(w.d) < epsmch * norm2(w.Xnew) ) goto L4060;
       end_if
    end_do
//    ! if we reach here, a successful step has been found
    
//    ! update X and f
    X = w.Xnew;
    w.f = w.fnew;

    if (! options.exact_second_derivatives) then 
//       ! first, let's save some old values...
//       ! g_old = -J_k^T r_k
       w.g_old = w.g;
//       ! g_mixed = -J_k^T r_{k+1}
       mult_Jt(w.J, w.fnew, w.g_mixed);
       w.g_mixed *= -1.0;
    end_if
//
//    ! evaluate J and hf at the new point
    eval_J(inform.external_return, n, m, X, w.J, params);
    inform.g_eval = inform.g_eval + 1;
//    if (inform.external_return .ne. 0) goto 4010
    for_do(i, 1, n)
      //w.J( (i-1)*m + 1 : i*m) = weights(1:m)*w.J( (i-1)*m + 1 : i*m)
        for_do(j, 1, m)
          w.J(j, i) *= weights(j);
        end_do
    end_do
    
    if ( options.calculate_svd_J ) then
       get_svd_J(w.J, w.smallest_sv(w.iter + 1), w.largest_sv(w.iter + 1));
    end_if
    
//    ! g = -J^Tf
    mult_Jt(w.J, w.f, w.g);
    w.g *= -1.0;

    w.normJFold = w.normJF;
    w.normF = normFnew;
    w.normJF = norm2(w.g);
    
//    ! setup the vectors needed if second derivatives are not available
    if (! options.exact_second_derivatives) then 
       w.y  = w.g_old;
       w.y -= w.g;
       w.y_sharp = w.g_mixed;
       w.y_sharp -= w.g;
    end_if
    
    if (options.model == 3) then
//       ! hybrid method -- check if we need second derivatives
       
       if (w.use_second_derivatives) then 
          if (w.normJF > w.normJFold) then 
             //! switch to Gauss-Newton             
             w.use_second_derivatives = false;
             //! save hf as hf_temp
             w.hf_temp = w.hf;
             w.hf.zero();
          end_if
       } else {
          auto FunctionValue = 0.5 * (pow(w.normF, 2));
          if ( w.normJF/FunctionValue < w.hybrid_tol ) then 
             w.hybrid_count = w.hybrid_count + 1;
             if (w.hybrid_count == options.hybrid_switch_its) then
                //! use (Quasi-)Newton
                w.use_second_derivatives = true;
                w.hybrid_count = 0;
                //! copy hf from hf_temp
                if (! options.exact_second_derivatives) then
                   w.hf = w.hf_temp;
                end_if
             end_if
          } else {
             w.hybrid_count = 0;
          end_if
       end_if

       if( ! w.use_second_derivatives) then
//          ! call apply_second_order_info anyway, so that we update the
//          ! second order approximation
          if (! options.exact_second_derivatives) then
             rank_one_update(w.hf_temp, w, n);
          end_if
       end_if

    end_if

    if ( w.use_second_derivatives ) then 
          apply_second_order_info(n,m,
               X,w,
               eval_HF,
               params,options,inform, weights);
    end_if

//    ! update the stats 
    inform.obj = 0.5*(pow(w.normF, 2));
    inform.norm_g = w.normJF;
    inform.scaled_g = w.normJF/w.normF;
    if (options.output_progress_vectors) then
       w.resvec (w.iter + 1) = inform.obj;
       w.gradvec(w.iter + 1) = inform.norm_g;
    end_if
    
//    !++++++++++++++++++!
//    ! Test convergence !
//    !++++++++++++++++++!
    test_convergence(w.normF,w.normJF,w.normF0,w.normJF0,options,inform);
    if (inform.convergence_normf == 1) goto L5000; // ! <----converged!!
    if (inform.convergence_normg == 1) goto L5010; // ! <----converged!!

//! error returns
L4000:
    //! generic end of algorithm
    //! all (final) exits should pass through here...
    inform.iter = w.iter;
    //if (w.resvec.size() > 0) then
       //if( allocated(inform.resvec)) deallocate(inform.resvec)
       //inform.resvec.allocate(w.iter + 1);
       //if (inform.alloc_status > 0) goto L4080;
       inform.resvec = w.resvec;
    //end_if
    //if (allocated(w.gradvec)) then
       //if (allocated(inform.gradvec)) deallocate(inform.gradvec)
       //allocate(inform.gradvec(w.iter + 1), stat = inform.alloc_status)
       //if (inform.alloc_status > 0) goto 4080
       inform.gradvec = w.gradvec;
    //end_if
    //if (options.calculate_svd_J) then
    //   if (allocated(inform.smallest_sv) ) deallocate(inform.smallest_sv)
    //   allocate(inform.smallest_sv(w.iter + 1))
    //   if (inform.alloc_status > 0) goto 4080
    //   if (allocated(inform.largest_sv) ) deallocate(inform.largest_sv)
    //   allocate(inform.largest_sv(w.iter + 1))
    //   if (inform.alloc_status > 0) goto 4080
    //end_if
    return;

L4010:
    //! Error in eval_J
    inform.external_name = "eval_J";
    inform.status = NLLS_ERROR::EVALUATION;
    goto L4000;

L4020:
    //! Error in eval_F
    inform.external_name = "eval_F";
    inform.status = NLLS_ERROR::EVALUATION;
    goto L4000;

L4030:
    //! Error in eval_HF
    inform.external_name = "eval_HF";
    inform.status = NLLS_ERROR::EVALUATION;
    goto L4000;

L4040:
    inform.status = NLLS_ERROR::UNSUPPORTED_MODEL;
    goto L4000;

L4050:
    //! max tr reductions exceeded
    inform.status = NLLS_ERROR::MAX_TR_REDUCTIONS;
    goto L4000;

L4060:
    //! x makes no progress
    inform.status = NLLS_ERROR::X_NO_PROGRESS;
    goto L4000;

L4070:
//    ! n > m on entry
    inform.status = NLLS_ERROR::N_GT_M;
    goto L4000;

L4080:
    //! bad allocation
    inform.status = NLLS_ERROR::ALLOCATION;
    inform.bad_alloc = "nlls_iterate";
    goto L4000;

//! convergence 
L5000:
    //! convegence test satisfied
    goto L4000;

L5010:
    goto L4000;

} // subroutine nlls_iterate

//  subroutine nlls_finalize(w,options)
//    
//    type( nlls_workspace ) :: w
//    type( nlls_options ) :: options
//    
//    w.first_call = 1
//
//    call remove_workspaces(w,options)   
//    
//  end subroutine nlls_finalize
//
//  subroutine nlls_strerror(inform)!,error_string)
//    type( nlls_inform ), intent(inout) :: inform
//    
//    if ( inform.status == NLLS_ERROR::MAXITS ) then
//       inform.error_message = 'Maximum number of iterations reached'
//    elseif ( inform.status == NLLS_ERROR::EVALUATION ) then
//       write(inform.error_message,'(a,a,a,i0)') & 
//            'Error code from user-supplied subroutine ',trim(inform.external_name), & 
//            ' passed error = ', inform.external_return
//    elseif ( inform.status == NLLS_ERROR::UNSUPPORTED_MODEL ) then
//       inform.error_message = 'Unsupported model passed in options'
//    elseif ( inform.status == NLLS_ERROR::FROM_EXTERNAL ) then
//       write(inform.error_message,'(a,a,a,i0)') & 
//            'The external subroutine ',trim(inform.external_name), & 
//            ' passed error = ', inform.external_return
//    elseif ( inform.status == NLLS_ERROR::UNSUPPORTED_METHOD ) then
//       inform.error_message = 'Unsupported nlls_method passed in options'
//    elseif ( inform.status == NLLS_ERROR::ALLOCATION ) then
//       write(inform.error_message,'(a,a)') &
//            'Bad allocation of memory in ', trim(inform.bad_alloc)
//    elseif ( inform.status == NLLS_ERROR::MAX_TR_REDUCTIONS ) then
//       inform.error_message = 'The trust region was reduced the maximum number of times'
//    elseif ( inform.status == NLLS_ERROR::X_NO_PROGRESS ) then
//       inform.error_message = 'No progress made in X'
//    elseif ( inform.status == NLLS_ERROR::N_GT_M ) then
//       inform.error_message = 'The problem is overdetermined'
//    elseif ( inform.status == NLLS_ERROR::BAD_TR_STRATEGY ) then
//       inform.error_message = 'Unsupported tr_update_stategy passed in options'
//    elseif ( inform.status == NLLS_ERROR::FIND_BETA ) then
//       inform.error_message = 'Unable to find suitable scalar in findbeta subroutine'
//    elseif ( inform.status == NLLS_ERROR::BAD_SCALING ) then
//       inform.error_message = 'Unsupported value of scale passed in options'
//    elseif ( inform.status == NLLS_ERROR::DOGLEG_MODEL ) then
//       inform.error_message = 'Model not supported in dogleg (nlls_method=1)'
//    elseif ( inform.status == NLLS_ERROR::AINT_EIG_IMAG ) then
//       inform.error_message = 'All eigenvalues are imaginary (nlls_method=2)'
//    elseif ( inform.status == NLLS_ERROR::AINT_EIG_ODD ) then
//       inform.error_message = 'Odd matrix sent to max_eig subroutine (nlls_method=2)'
//    elseif ( inform.status == NLLS_ERROR::MS_MAXITS ) then
//       inform.error_message = 'Maximum iterations reached in more_sorensen (nlls_method=3)'
//    elseif ( inform.status == NLLS_ERROR::MS_TOO_MANY_SHIFTS ) then
//       inform.error_message = 'Too many shifts taken in more_sorensen (nlls_method=3)'
//    elseif ( inform.status == NLLS_ERROR::MS_NO_PROGRESS ) then
//       inform.error_message = 'No progress being made in more_sorensen (nlls_method=3)'
//    else 
//       inform.error_message = 'Unknown error number'           
//    end if
//    
//  end subroutine nlls_strerror
//      
//  
//end module ral_nlls_double


///!  -----------------------------------------------------------------------------
///!  RAL_NLLS, a fortran subroutine for finding a first-order critical
///!   point (most likely, a local minimizer) of the nonlinear least-squares 
///!   objective function 1/2 ||F(x)||_2^2.
///!
///!  Authors: RAL NA Group (Iain Duff, Nick Gould, Jonathan Hogg, Tyrone Rees, 
///!                         Jennifer Scott)
///!  -----------------------------------------------------------------------------
void NLLS_SOLVE(int n, int m, DoubleFortranVector& X,
  eval_f_type eval_F, eval_j_type eval_J, eval_hf_type eval_HF,
  params_base_type params,
  const nlls_options &options, nlls_inform &inform, const DoubleFortranVector& weights ) {


  //INTEGER, INTENT( IN ) :: n, m
  //REAL( wp ), DIMENSION( n ), INTENT( INOUT ) :: X
  //TYPE( NLLS_inform ), INTENT( OUT ) :: inform
  //TYPE( NLLS_options ), INTENT( IN ) :: options
  //procedure( eval_f_type ) :: eval_F
  //procedure( eval_j_type ) :: eval_J
  //procedure( eval_hf_type ) :: eval_HF
  //class( params_base_type ) :: params
  //real( wp ), dimension( m ), intent(in), optional :: weights
  //integer  :: i
  NLLS_workspace w(n, m, options, inform);
  //character (len = 80) :: error_string

  //!!$    write(*,*) 'Controls in:'
  //!!$    write(*,*) control
  //!!$    write(*,*) 'error = ',options.error
  //!!$    write(*,*) 'out = ', options.out
  //!!$    write(*,*) 'print_level = ', options.print_level
  //!!$    write(*,*) 'maxit = ', options.maxit
  //!!$    write(*,*) 'model = ', options.model
  //!!$    write(*,*) 'nlls_method = ', options.nlls_method
  //!!$    write(*,*) 'lls_solver = ', options.lls_solver
  //!!$    write(*,*) 'stop_g_absolute = ', options.stop_g_absolute
  //!!$    write(*,*) 'stop_g_relative = ', options.stop_g_relative     
  //!!$    write(*,*) 'initial_radius = ', options.initial_radius
  //!!$    write(*,*) 'maximum_radius = ', options.maximum_radius
  //!!$    write(*,*) 'eta_successful = ', options.eta_successful
  //!!$    write(*,*) 'eta_very_successful = ',options.eta_very_successful
  //!!$    write(*,*) 'eta_too_successful = ',options.eta_too_successful
  //!!$    write(*,*) 'radius_increase = ',options.radius_increase
  //!!$    write(*,*) 'radius_reduce = ',options.radius_reduce
  //!!$    write(*,*) 'radius_reduce_max = ',options.radius_reduce_max
  //!!$    write(*,*) 'hybrid_switch = ',options.hybrid_switch
  //!!$    write(*,*) 'subproblem_eig_fact = ',options.subproblem_eig_fact
  //!!$    write(*,*) 'more_sorensen_maxits = ',options.more_sorensen_maxits
  //!!$    write(*,*) 'more_sorensen_shift = ',options.more_sorensen_shift
  //!!$    write(*,*) 'more_sorensen_tiny = ',options.more_sorensen_tiny
  //!!$    write(*,*) 'more_sorensen_tol = ',options.more_sorensen_tol
  //!!$    write(*,*) 'hybrid_tol = ', options.hybrid_tol
  //!!$    write(*,*) 'hybrid_switch_its = ', options.hybrid_switch_its
  //!!$    write(*,*) 'output_progress_vectors = ',options.output_progress_vectors

  for_do(i, 1,options.maxit)

    nlls_iterate(n, m, X,
      w,
      eval_F, eval_J, eval_HF,
      params,
      inform, options, weights);
      //! test the returns to see if we've converged

      if (inform.status != NLLS_ERROR::OK) {
          //nlls_strerror(inform);
      goto L1000; // ! error -- exit
    } else if ((inform.convergence_normf == 1) || (inform.convergence_normg == 1)) {
      goto L1000; // ! converged -- exit
    }
  end_do

  //! If we reach here, then we're over maxits     
  inform.status = NLLS_ERROR::MAXITS;

  L1000:
  //nlls_finalize(w,options);

} //   END SUBROUTINE NLLS_SOLVE

} // namespace RalNlls
} // CurveFitting
} // Mantid
