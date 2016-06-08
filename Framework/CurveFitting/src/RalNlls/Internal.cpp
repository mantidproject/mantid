#include "MantidCurveFitting/FortranDefs.h"
#include "MantidCurveFitting/RalNlls/DTRS.h"
#include "MantidCurveFitting/RalNlls/Internal.h"

#include <algorithm>
#include <limits>
#include <functional>
#include <string>
#include <iostream>

#include <gsl/gsl_blas.h>
#include <gsl/gsl_linalg.h>

#define for_do(i, a, n) for(int i=a; i <=n; ++i) {
#define then {
#define end_if }
#define end_do }

namespace Mantid {
namespace CurveFitting {
namespace RalNlls {

void more_sorensen( const DoubleFortranMatrix& J, const DoubleFortranVector& f, const DoubleFortranMatrix& hf,
  int n, int m, double Delta, DoubleFortranVector& d, double& nd, const nlls_options& options, nlls_inform& inform,
  more_sorensen_work& w);

void solve_dtrs(const DoubleFortranMatrix& J, const DoubleFortranVector& f, const DoubleFortranMatrix& hf,
  int n, int m, double Delta, DoubleFortranVector& d, double& normd, const nlls_options& options, nlls_inform& inform,
  solve_dtrs_work& w);

//! -------------------------------------------------------
//! calculate_step, find the next step in the optimization
//! -------------------------------------------------------
void calculate_step(const DoubleFortranMatrix& J, const DoubleFortranVector& f, const DoubleFortranMatrix& hf, const DoubleFortranVector& g,
  int n, int m, double Delta, DoubleFortranVector& d, double& normd, const nlls_options& options, nlls_inform& inform, calculate_step_work& w) {

     switch(options.nlls_method) {
     case 1: //! Powell's dogleg
        throw std::logic_error("Dog leg method isn't implemented.");
        break;
     case 2: //! The AINT method
       throw std::logic_error("AINT method isn't implemented.");;
        break;
     case 3: //! More-Sorensen
        more_sorensen(J,f,hf,n,m,Delta,d,normd,options,inform,w.more_sorensen_ws);
        break;
     case 4: //! Galahad
        solve_dtrs(J,f,hf,n,m,Delta,d,normd,options,inform,w.solve_dtrs_ws);
        break;
     default:
        inform.status = NLLS_ERROR::UNSUPPORTED_METHOD;
     }

} //   END SUBROUTINE calculate_step
   
  //       ! return the (ii,jj)th entry of a matrix 
  //       ! J held by columns....
void get_element_of_matrix(const DoubleFortranMatrix& J, int m, int ii, int jj, double& Jij) {
  //       Jij = J(ii + (jj-1)*m)
  Jij = J(ii, jj);
}

///     !-------------------------------
///     ! apply_scaling
///     ! input  Jacobian matrix, J
///     ! ouput  scaled Hessisan, H, and J^Tf, v.
///     !
///     ! Calculates a diagonal scaling W, stored in w.diag
///     ! updates v(i) -> (1/W_i) * v(i)
///     !         A(i,j) -> (1 / (W_i * W_j)) * A(i,j)
///     !-------------------------------
void apply_scaling(const DoubleFortranMatrix& J, int n, int m, DoubleFortranMatrix& A, 
  DoubleFortranVector& v, apply_scaling_work& w, const nlls_options options, nlls_inform inform) {

  if (w.diag.len() != n) {
    w.diag.allocate(n);
  }

     switch (options.scale) {
     case 1:
     case 2:
        for(int ii=1; ii<=n; ++ii) {//do ii = 1,n
           double temp = zero;
           double Jij = 0.0;
           if (options.scale == 1) then
              //! use the scaling present in gsl:
              //! scale by W, W_ii = ||J(i,:)||_2^2
              for_do(jj, 1,m)
                 //get_element_of_matrix(J,m,jj,ii,Jij);
                 Jij = J(jj, ii);
                 temp = temp + pow(Jij, 2);
              end_do
           }else if ( options.scale == 2) then 
              //! scale using the (approximate) hessian
              for_do(jj, 1,n)
                 temp = temp + pow(A(ii,jj), 2);
              end_do
           end_if
           if (temp < options.scale_min) then 
              if (options.scale_trim_min) then 
                 temp = options.scale_min;
              }else{
                 temp = one;
              end_if
           }else if (temp > options.scale_max) then
              if (options.scale_trim_max) then 
                 temp = options.scale_max;
              }else{
                 temp = one;
              end_if
           end_if
           temp = sqrt(temp);
           if (options.scale_require_increase) then
              w.diag(ii) = std::max(temp,w.diag(ii));
           }else{
              w.diag(ii) = temp;
           end_if
        }
        break;
     default:
        inform.status = NLLS_ERROR::BAD_SCALING;
        return;
     }
          
//     ! now we have the w.diagonal scaling matrix, actually scale the 
//     ! Hessian approximation and J^Tf
     for_do(ii, 1,n)
       double temp = w.diag(ii);
       v(ii) = v(ii) / temp;
       for_do(jj,1,n)
         A(ii,jj) = A(ii,jj) / temp;
         A(jj,ii) = A(jj,ii) / temp;
       end_do
     end_do
   
} //   end subroutine apply_scaling

/// Copy a column from a matrix.
DoubleFortranVector getColumn(const DoubleFortranMatrix& A, int col) {
  int n = static_cast<int>(A.size1());
  DoubleFortranVector column(n);
  for_do(i,1,n)
    column(i) = A(i, col);
  end_do
  return column;
}

/// Negate a vector
DoubleFortranVector negative(const DoubleFortranVector& v) {
  DoubleFortranVector neg = v;
  neg *= -1.0;
  return neg;
}

void mult_J(const DoubleFortranMatrix& J, const DoubleFortranVector& x, DoubleFortranVector& Jx) {
       //dgemv('N',m,n,alpha,J,m,x,1,beta,Jx,1);
  if (Jx.len() != J.len1()) {
    Jx.allocate(J.len1());
  }
  gsl_blas_dgemv(CblasNoTrans, 1.0, J.gsl(), x.gsl(), 0.0, Jx.gsl());

}

void mult_Jt(const DoubleFortranMatrix& J, const DoubleFortranVector& x, DoubleFortranVector& Jtx) {
//       call dgemv('T',m,n,alpha,J,m,x,1,beta,Jtx,1)
  if (Jtx.len() != J.len2()) {
    Jtx.allocate(J.len2());
  }
  gsl_blas_dgemv(CblasTrans, 1.0, J.gsl(), x.gsl(), 0.0, Jtx.gsl());

}

double norm2(const DoubleFortranVector &v) {
  if (v.size() == 0)
    return 0.0;
  return gsl_blas_dnrm2(v.gsl());
}

///     Takes an m x n matrix J and forms the 
///     n x n matrix A given by
///     A = J' * J
void matmult_inner(const DoubleFortranMatrix& J, int n, int m, DoubleFortranMatrix& A) {
  A.allocate(n, n);
  gsl_blas_dgemm(CblasTrans, CblasNoTrans, 1.0, J.gsl(), J.gsl(), 0.0, A.gsl());
} //     end subroutine matmult_inner

///     Takes an m x n matrix J and forms the 
///     m x m matrix A given by
///     A = J * J'
void matmult_outer( const DoubleFortranMatrix& J, int n, int m, DoubleFortranMatrix& A) {
    gsl_blas_dgemm(CblasNoTrans, CblasTrans, 1.0, J.gsl(), J.gsl(), 0.0, A.gsl());

} //     end subroutine matmult_outer

DoubleFortranVector matmul(const DoubleFortranMatrix& J, const DoubleFortranVector& x) {
  DoubleFortranVector y(int(x.size()));
  gsl_blas_dgemv(CblasNoTrans, 1.0, J.gsl(), x.gsl(), 0.0, y.gsl());
  return y;
}

DoubleFortranMatrix matmul(const DoubleFortranMatrix& A, const DoubleFortranMatrix& B) {
  int n = static_cast<int>(A.size1());
  int m = static_cast<int>(B.size2());
  DoubleFortranMatrix C(n, m);
  gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, A.gsl(), B.gsl(), 0.0, C.gsl());
  return C;
}

double dot_product(const DoubleFortranVector& x, const DoubleFortranVector& y) {
  return x.dot(y);
}

///  Takes an n vector x and forms the 
///  n x n matrix xtx given by
///  xtx = x * x'
void outer_product(const DoubleFortranVector& x, int n, DoubleFortranMatrix& xtx) {
  xtx.allocate(n, n);
  gsl_blas_dger(1.0, x.gsl(), x.gsl(), xtx.gsl());
}

///   Calculates norm_A_x = ||x||_A = sqrt(x'*A*x)
void matrix_norm(const DoubleFortranVector& x, const DoubleFortranMatrix& A, double &norm_A_x) {
       norm_A_x = sqrt(dot_product(x,matmul(A,x)));
}
void shift_matrix(const DoubleFortranMatrix &A, double sigma, DoubleFortranMatrix& AplusSigma, int n) {

  //real, intent(in)   A(:,:), sigma
  //real, intent(out)  AplusSigma(:,:)
  //integer, intent(in)  n 

  //! calculate AplusSigma = A + sigma * I

  AplusSigma = A;
  for_do(i,1,n) 
    AplusSigma(i,i) = AplusSigma(i,i) + sigma;
  }

} // subroutine shift_matrix

void solve_spd(const DoubleFortranMatrix& A, const DoubleFortranVector& b, 
  DoubleFortranMatrix& LtL, DoubleFortranVector& x, int n, nlls_inform& inform) {
  //       REAL, intent(in)  A(:,:)
  //       REAL, intent(in)  b(:)
  //       REAL, intent(out)  LtL(:,:)
  //       REAL, intent(out)  x(:)
  //       integer, intent(in)  n
  //       type( nlls_inform), intent(inout)  inform
  //
  //       ! A wrapper for the lapack subroutine dposv.f
  //       ! get workspace for the factors....
  LtL = A;
  //x = b;
  //       call dposv('L', n, 1, LtL, n, x, n, inform.external_return)
  //       if (inform.external_return .ne. 0) then
  //          inform.status = ERROR.FROM_EXTERNAL
  //          inform.external_name = 'lapack_dposv'
  //          return
  //       end if
  LtL.solve(b, x);
} //     end subroutine solve_spd

void all_eig_symm(const DoubleFortranMatrix& A, int n, DoubleFortranVector& ew, 
  DoubleFortranMatrix& ev,all_eig_symm_work& w,nlls_inform& inform) {
  //       ! calculate all the eigenvalues of A (symmetric)
  auto M = A;
  M.eigenSystem(ew, ev);
} //     end subroutine all_eig_symm

  ///     !--------------------------------------------------
///     ! get_pd_shift
///     !
///     ! Given an indefinite matrix w.A, find a shift sigma
///     ! such that (A + sigma I) is positive definite
///     !--------------------------------------------------
void get_pd_shift(int n, double& sigma, DoubleFortranVector& d, const nlls_options& options, nlls_inform& inform, more_sorensen_work& w) {
     int no_shifts = 0;
     bool successful_shift = false;
     while( ! successful_shift ) {
        shift_matrix(w.A,sigma,w.AplusSigma,n);
        solve_spd(w.AplusSigma,negative(w.v),w.LtL,d,n,inform);
        if ( inform.status != NLLS_ERROR::OK ) then
           //! reset the error calls -- handled in the code....
           inform.status = NLLS_ERROR::OK;
           inform.external_return = 0;
           inform.external_name = "";
           no_shifts = no_shifts + 1;
           if ( no_shifts == 10 ) {// goto 3000 ! too many shifts -- exit
             inform.status = NLLS_ERROR::MS_TOO_MANY_SHIFTS;
             return;
           }
           sigma =  sigma + (pow(10.0, no_shifts)) * options.more_sorensen_shift;
       }else{
           successful_shift = true;
        end_if
     }

//3000 continue
//     ! too many shifts
//     inform.status = ERROR.MS_TOO_MANY_SHIFTS
//     return     
} //   end subroutine get_pd_shift
   
///     !---------------------------------------------
///     ! solve_dtrs
///     ! Solve the trust-region subproblem using
///     ! the DTRS method from Galahad
///     ! 
///     ! This method needs H to be diagonal, so we need to 
///     ! pre-process
///     !
///     ! main output  d, the soln to the TR subproblem
///     !--------------------------------------------
void solve_dtrs(const DoubleFortranMatrix& J, const DoubleFortranVector& f, const DoubleFortranMatrix& hf,
  int n, int m, double Delta, DoubleFortranVector& d, double& normd, const nlls_options& options, nlls_inform& inform,
  solve_dtrs_work& w) {

     dtrs_control_type dtrs_options;
     dtrs_inform_type dtrs_inform;

//!     real, allocatable  diag(:)
//     integer  ii

//     ! The code finds 
//     !  d = arg min_p   w^T p + 0.5 * p^T D p
//     !       s.t. ||p|| \leq Delta
//     !
//     ! where D is diagonal
//     !
//     ! our probem in naturally in the form
//     ! 
//     ! d = arg min_p   v^T p + 0.5 * p^T H p
//     !       s.t. ||p|| \leq Delta
//     !
//     ! first, find the matrix H and vector v
//     ! Set A = J^T J
     matmult_inner(J,n,m,w.A);
//     ! add any second order information...
//     ! so A = J^T J + HF
     w.A += hf;

//     ! now form v = J^T f 
     mult_Jt(J,f,w.v);

//     ! if scaling needed, do it
     if ( options.scale != 0) then
        apply_scaling(J,n,m,w.A,w.v,w.apply_scaling_ws,options,inform);
     end_if

//     ! Now that we have the unprocessed matrices, we need to get an 
//     ! eigendecomposition to make A diagonal
//     !
     all_eig_symm(w.A,n,w.ew,w.ev,w.all_eig_symm_ws,inform);
     if (inform.status != NLLS_ERROR::OK) return; // goto 1000

//     ! We can now change variables, setting y = Vp, getting
//     ! Vd = arg min_(Vx) v^T p + 0.5 * (Vp)^T D (Vp)
//     !       s.t.  ||x|| \leq Delta
//     ! <=>
//     ! Vd = arg min_(Vx) V^Tv^T (Vp) + 0.5 * (Vp)^T D (Vp)
//     !       s.t.  ||x|| \leq Delta
//     ! <=>

//     ! we need to get the transformed vector v
     mult_Jt(w.ev,w.v,w.v_trans);

//     ! we've now got the vectors we need, pass to dtrs_solve
     dtrs_initialize( dtrs_options, dtrs_inform );

     if (w.v_trans.len() != n) {
       w.v_trans.allocate(n);
     }

     for_do(ii, 1,n)
        if (abs(w.v_trans(ii)) < epsmch) then
           w.v_trans(ii) = zero;
        end_if
        if (abs(w.ew(ii)) < epsmch) then
           w.ew(ii) = zero;
        end_if
     end_do

     dtrs_solve(n, Delta, zero, w.v_trans, w.ew, w.d_trans, dtrs_options, dtrs_inform );
     if ( dtrs_inform.status != ErrorCode::ral_nlls_ok) then
        inform.external_return = int(dtrs_inform.status);
        inform.external_name = "galahad_dtrs";
        inform.status = NLLS_ERROR::FROM_EXTERNAL;
        return; // goto 1000
     end_if
     
//     ! and return the un-transformed vector
     mult_J(w.ev,w.d_trans,d);

     normd = norm2(d); // ! ||d||_D
     
     if (options.scale != 0 ) then 
        for_do(ii, 1, n)
           d(ii) = d(ii) / w.apply_scaling_ws.diag(ii);
        end_do
     end_if

//1000 continue 
//     ! bad error return from external package
//     return
//     
} //   end subroutine solve_dtrs

//!  -----------------------------------------------------------------
//!  findbeta, a subroutine to find the optimal beta such that 
//!   || d || = Delta, where d = a + beta * b
//!   
//!   uses the approach from equation (3.20b), 
//!    "Methods for non-linear least squares problems" (2nd edition, 2004)
//!    by Madsen, Nielsen and Tingleff      
//!  -----------------------------------------------------------------
void findbeta(const DoubleFortranVector& a, const DoubleFortranVector& b, double Delta, double& beta, nlls_inform& inform) {
//
//     real, dimension(:), intent(in)  a, b 
//     real, intent(in)   Delta
//     real, intent(out)  beta
//     type( nlls_inform ), intent(out)  inform
//     
//     real  c, normb2, norma2, discrim, denom
     
     auto c = dot_product(a,b);

     auto norma2 = pow(norm2(a), 2);
     auto normb2 = pow(norm2(b), 2);

     double discrim = pow(c, 2) + (normb2)*(pow(Delta, 2) - norma2);
     if ( discrim < zero ) then
        inform.status = NLLS_ERROR::FIND_BETA;
        inform.external_name = "findbeta";
        return;
     end_if

     if (c <= 0) then
        beta = (-c + sqrt(discrim) ) / normb2;
     }else{
        beta = (pow(Delta,2) - norma2) / ( c + sqrt(discrim) );
     end_if

} //     END SUBROUTINE findbeta

//! --------------------------------------------------
//! Input:
//! f = f(x_k), J = J(x_k), 
//! hf = \sum_{i=1}^m f_i(x_k) \nabla^2 f_i(x_k) (or an approx)
//!
//! We have a model 
//!      m_k(d) = 0.5 f^T f  + d^T J f + 0.5 d^T (J^T J + HF) d
//!
//! This subroutine evaluates the model at the point d 
//! This value is returned as the scalar
//!       md :=m_k(d)
//! --------------------------------------------------       
void evaluate_model(const DoubleFortranVector& f, const DoubleFortranMatrix& J, const DoubleFortranMatrix& hf,
  const DoubleFortranVector& d, double& md, int m, int n, const nlls_options options, evaluate_model_work& w) {

       
//       !Jd = J*d
       mult_J(J,d,w.Jd);

//       ! First, get the base 
//       ! 0.5 (f^T f + f^T J d + d^T' J ^T J d )
       DoubleFortranVector temp = f;
       temp += w.Jd;
       md = 0.5 * pow(norm2(temp), 2);
       switch(options.model) {
       case 1: //! first-order (no Hessian)
          //! nothing to do here...
          break;
       default:
          //! these have a dynamic H -- recalculate
          //! H = J^T J + HF, HF is (an approx?) to the Hessian
          mult_J(hf,d,w.Hd);
          md = md + 0.5 * dot_product(d,w.Hd);
       }

} //     end subroutine evaluate_model

//       !++++++++++++++++++++++++++++++++++++++++++++++++++++++++++!
//       ! Calculate the quantity                                   ! 
//       !   rho = 0.5||f||^2 - 0.5||fnew||^2 =   actual_reduction  !
//       !         --------------------------   ------------------- !
//       !             m_k(0)  - m_k(d)         predicted_reduction !
//       !                                                          !
//       ! if model is good, rho should be close to one             !
//       !+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void calculate_rho(double normf, double normfnew,double md, double& rho, const nlls_options& options) {

       auto actual_reduction = ( 0.5 * pow(normf, 2) ) - ( 0.5 * pow(normfnew, 2) );
       auto predicted_reduction = ( ( 0.5 * pow(normf, 2) ) - md );
       
       if ( abs(actual_reduction) < 10*epsmch ) then 
          rho = one;
       }else if (abs( predicted_reduction ) < 10 * epsmch ) then 
          rho = one;
       }else{
          rho = actual_reduction / predicted_reduction;
       end_if

} //     end subroutine calculate_rho


void rank_one_update(DoubleFortranMatrix& hf, NLLS_workspace w, int n) {

//       real, intent(inout)  hf(:)
//       type( NLLS_workspace ), intent(inout)  w
//       integer, intent(in)  n
//      
//       real  yts, alpha, dSks

       auto yts = dot_product(w.d,w.y);
       if ( abs(yts) < 10 * epsmch ) then
          //! safeguard: skip this update
          return;
       end_if

       mult_J(hf,w.d,w.Sks); // ! hfs = S_k * d

       w.ysharpSks = w.y_sharp;
       w.ysharpSks -= w.Sks;

//       ! now, let's scale hd (Nocedal and Wright, Section 10.2)
       auto dSks = abs(dot_product(w.d,w.Sks));
       auto alpha = abs(dot_product(w.d,w.y_sharp))/ dSks;
       alpha = std::min(one,alpha);
       hf *= alpha;

//       ! update S_k (again, as in N&W, Section 10.2)

//       ! hf = hf + (1/yts) (y# - Sk d)^T y:
       alpha = 1/yts;
       //call dGER(n,n,alpha,w.ysharpSks,1,w.y,1,hf,n)
       gsl_blas_dger(alpha, w.ysharpSks.gsl(), w.y.gsl(), hf.gsl());
//       ! hf = hf + (1/yts) y^T (y# - Sk d):
       //call dGER(n,n,alpha,w.y,1,w.ysharpSks,1,hf,n)
       gsl_blas_dger(alpha, w.y.gsl(), w.ysharpSks.gsl(), hf.gsl());
//       ! hf = hf - ((y# - Sk d)^T d)/((yts)**2)) * y y^T
       alpha = -dot_product(w.ysharpSks,w.d)/(pow(yts, 2));
       //call dGER(n,n,alpha,w.y,1,w.y,1,hf,n)
       gsl_blas_dger(alpha, w.y.gsl(), w.y.gsl(), hf.gsl());

} //     end subroutine rank_one_update

void apply_second_order_info(int n, int m, const DoubleFortranVector& X,
  NLLS_workspace& w, eval_hf_type eval_HF, params_base_type params, 
  const nlls_options& options, nlls_inform& inform, const DoubleFortranVector& weights) {

  if (options.exact_second_derivatives) {
    DoubleFortranVector temp = w.f;
    temp *= weights;
    eval_HF(inform.external_return, n, m, X, temp, w.hf, params);
    inform.h_eval = inform.h_eval + 1;
  }else{
    //! use the rank-one approximation...
    rank_one_update(w.hf,w,n);
  }
} //     end subroutine apply_second_order_info


void update_trust_region_radius(double& rho, const nlls_options& options, nlls_inform& inform, NLLS_workspace& w) {
//
//       real, intent(inout)  rho ! ratio of actual to predicted reduction
//       type( nlls_options ), intent(in)  options
//       type( nlls_inform ), intent(inout)  inform
//       type( nlls_workspace ), intent(inout)  w

       switch(options.tr_update_strategy) {
       case 1: //! default, step-function
          if (rho < options.eta_success_but_reduce) {
//             ! unsuccessful....reduce Delta
             w.Delta = std::max( options.radius_reduce, options.radius_reduce_max) * w.Delta;
          }else if (rho < options.eta_very_successful) { 
//             ! doing ok...retain status quo
          }else if (rho < options.eta_too_successful ) {
//             ! more than very successful -- increase delta
             w.Delta = std::min(options.maximum_radius,  options.radius_increase * w.normd);
//             ! increase based on normd = ||d||_D
//             ! if d is on the tr boundary, this is Delta
//             ! otherwise, point was within the tr, and there's no point increasing 
//             ! the radius
          }else if (rho >= options.eta_too_successful) {
//             ! too successful....accept step, but don't change w.Delta
          }else{
//             ! just incase (NaNs and the like...)
             w.Delta = std::max( options.radius_reduce, options.radius_reduce_max) * w.Delta;
             rho = -one; //! set to be negative, so that the logic works....
          }
          break;
       case 2: // ! Continuous method
//          ! Based on that proposed by Hans Bruun Nielsen, TR IMM-REP-1999-05
//          ! http://www2.imm.dtu.dk/documents/ftp/tr99/tr05_99.pdf
          if (rho >= options.eta_too_successful) {
//             ! too successful....accept step, but don't change w.Delta
          }else if (rho > options.eta_successful) { 
             w.Delta = w.Delta * std::min(options.radius_increase, std::max(options.radius_reduce,
                  1 - ( (options.radius_increase - 1) * (pow((1 - 2*rho), w.tr_p))) ));
             w.tr_nu = options.radius_reduce;
          }else if ( rho <= options.eta_successful ) {
             w.Delta = w.Delta * w.tr_nu;
             w.tr_nu =  w.tr_nu * 0.5;
          }else{
//             ! just incase (NaNs and the like...)
             w.Delta = std::max( options.radius_reduce, options.radius_reduce_max) * w.Delta;
             rho = -one; // ! set to be negative, so that the logic works....
          }
          break;
       default:
          inform.status = NLLS_ERROR::BAD_TR_STRATEGY;
       }

} //     end subroutine update_trust_region_radius

void test_convergence(double normF, double normJF, double normF0, double normJF0,
  const nlls_options& options, nlls_inform& inform) {

       if ( normF <= std::max(options.stop_g_absolute, 
            options.stop_g_relative * normF0) ) then
          inform.convergence_normf = 1;
          return;
       end_if

       if ( (normJF/normF) <= std::max(options.stop_g_absolute, 
            options.stop_g_relative * (normJF0/normF0)) ) then
          inform.convergence_normg = 1;
       end_if

} //     end subroutine test_convergence

void solve_general(const DoubleFortranMatrix& A, const DoubleFortranVector& b,
   DoubleFortranVector& x, int n, nlls_inform& inform, solve_general_work& w) {
//       ! A wrapper for the lapack subroutine dposv.f
//       ! NOTE: A would be destroyed
       w.A = A;
       w.A.solve(b, x);
//       x(1:n) = b(1:n)
//       call dgesv( n, 1, w.A, n, w.ipiv, x, n, inform.external_return)
} //     end subroutine solve_general

//       ! calculate the leftmost eigenvalue of A
void min_eig_symm(const DoubleFortranMatrix& A, double& sigma,
  DoubleFortranVector& y) {
  auto M = A;
  DoubleFortranVector ew;
  DoubleFortranMatrix ev;
  M.eigenSystem(ew, ev);
  auto ind = ew.sortIndices();
  int imin = static_cast<int>(ind[0]);
  sigma = ew(imin);
  int n = static_cast<int>(A.size1());
  y.allocate(n);
  for(int i = 1; i <= n; ++i) {
    y(i) = ev(i, imin);
  }
} //     end subroutine min_eig_symm

///  Given an (m x n)  matrix J held by columns as a vector,
///  this routine returns the largest and smallest singular values
///  of J.
void get_svd_J(const DoubleFortranMatrix& J, double &s1, double &sn) {
//       integer, intent(in)  n,m 
//       real, intent(in)  J(:)
//       real, intent(out)  s1, sn
//       type( nlls_options )  options
//       integer, intent(out)  status
//       type( get_svd_J_work )  w

  auto n = J.len2();
  DoubleFortranMatrix U = J;
  DoubleFortranMatrix V(n, n);
  DoubleFortranVector S(n);
  DoubleFortranVector work(n);
  gsl_linalg_SV_decomp(U.gsl(), V.gsl(), S.gsl(), work.gsl());
  s1 = S(1);
  sn = S(n);
  std::cerr << "J=" << J << '\n';
} // subroutine get_svd_J

///! -----------------------------------------
///! more_sorensen
///! Solve the trust-region subproblem using 
///! the method of More and Sorensen
///!
///! Using the implementation as in Algorithm 7.3.6
///! of Trust Region Methods
///! 
///! main output  d, the soln to the TR subproblem
///! -----------------------------------------
void more_sorensen( const DoubleFortranMatrix& J, const DoubleFortranVector& f, const DoubleFortranMatrix& hf,
int n, int m, double Delta, DoubleFortranVector& d, double& nd, const nlls_options& options, nlls_inform& inform,
more_sorensen_work& w) {

  //     ! The code finds 
  //     !  d = arg min_p   v^T p + 0.5 * p^T A p
  //     !       s.t. ||p|| \leq Delta
  //     !
  //     ! set A and v for the model being considered here...

  //     ! Set A = J^T J
  matmult_inner(J,n,m,w.A);
  //     ! add any second order information...
  //     ! so A = J^T J + HF
  //     w.A = w.A + reshape(hf,(/n,n/))
  w.A += hf;
  //     ! now form v = J^T f 
  mult_Jt(J,f,w.v);

  //     ! if scaling needed, do it
  if ( options.scale != 0) {
    apply_scaling(J,n,m,w.A,w.v,w.apply_scaling_ws,options,inform);
  }

  auto local_ms_shift = options.more_sorensen_shift;

  //     ! d = -A\v
  DoubleFortranVector negv = w.v;
  negv *= -1.0;
  solve_spd(w.A,negv,w.LtL,d,n,inform);
  double sigma = 0.0;
  if (inform.status == NLLS_ERROR::OK) {
    //! A is symmetric positive definite....
    sigma = zero;
  }else{
    //! reset the error calls -- handled in the code....
    inform.status = NLLS_ERROR::OK;
    inform.external_return = 0;
    inform.external_name = "";
    min_eig_symm(w.A,sigma,w.y1);
    if (inform.status != NLLS_ERROR::OK) goto L1000;
    sigma = -(sigma - local_ms_shift);
    //! find a shift that makes (A + sigma I) positive definite
    get_pd_shift(n,sigma,d,options,inform,w);
    if (inform.status != NLLS_ERROR::OK) goto L4000;
  }

  nd = norm2(d);

  //     ! now, we're not in the trust region initally, so iterate....
  auto sigma_shift = zero;
  int no_restarts = 0;
  //     ! set 'small' in the context of the algorithm
  double epsilon = std::max( options.more_sorensen_tol * Delta, options.more_sorensen_tiny );
  for(int i = 1; i <= options.more_sorensen_maxits; ++i) {

    if (nd <= Delta + epsilon) {
      //! we're within the tr radius
      if ( abs(sigma) < options.more_sorensen_tiny ) {
        //! we're good....exit
        goto L1020;
      }else if ( abs( nd - Delta ) < epsilon ) {
        //! also good...exit
        goto L1020;
      }
      double alpha = 0.0;
      findbeta(d,w.y1,Delta,alpha,inform);
      if (inform.status != NLLS_ERROR::OK ) goto L1000;
      DoubleFortranVector tmp = w.y1;
      tmp *= alpha;
      d += tmp;
      //! also good....exit
      goto L1020;
    }

    //w.q = d; //! w.q = R'\d
    //DTRSM( "Left", "Lower", "No Transpose", "Non-unit", n, 1, one, w.LtL, n, w.q, n );
    w.LtL.solve(d, w.q);

    auto nq = norm2(w.q);

    sigma_shift = ( pow((nd/nq), 2) ) * ( (nd - Delta) / Delta );
    if (abs(sigma_shift) < options.more_sorensen_tiny * abs(sigma) ) {
      if (no_restarts < 1) { 
        //! find a shift that makes (A + sigma I) positive definite
        get_pd_shift(n,sigma,d,options,inform,w);
        if (inform.status != NLLS_ERROR::OK) goto L4000;
        no_restarts = no_restarts + 1;
      }else{
        //! we're not going to make progress...jump out 
        inform.status = NLLS_ERROR::MS_NO_PROGRESS;
        goto L4000;
      }
    }else{ 
      sigma = sigma + sigma_shift;
    }

    shift_matrix(w.A,sigma,w.AplusSigma,n);
    DoubleFortranVector negv = w.v;
    negv *= -1.0;
    solve_spd(w.AplusSigma,negv,w.LtL,d,n,inform);
    if (inform.status != NLLS_ERROR::OK) goto L1000;

    nd = norm2(d);
  }

  goto L1040;

  L1000:
  //     ! bad error return from external package
  goto L4000;
  //     
  L1020:
  //     ! inital point was successful
  goto L4000;

  L1040:
  //     ! maxits reached, not converged
  inform.status = NLLS_ERROR::MS_MAXITS;
  goto L4000;

  L3000:
  //     ! too many shifts
  inform.status = NLLS_ERROR::MS_TOO_MANY_SHIFTS;
  goto L4000;
  //     
  L4000:
  //     ! exit the routine
  if (options.scale != 0 ) {
    for(int i = 1; i <= n; ++i) {
      d(i) = d(i) / w.apply_scaling_ws.diag(i);
    }
  }
} //   end subroutine more_sorensen



//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//     !!                                                       !!
//     !! W O R K S P A C E   S E T U P   S U B R O U T I N E S !!
//     !!                                                       !!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

//get_svd_J_work::get_svd_J_work(int n, int m, bool calculate_svd_J)
//{
//  if (calculate_svd_J) {
//    Jcopy.allocate(n*m);
//    S.allocate(n);
//  }
//}

//void setup_workspace_more_sorensen(n,m,w,options,inform) {
//       integer, intent(in)  n,m
//       type( more_sorensen_work )  w
//       type( nlls_options ), intent(in)  options
//       type( nlls_inform ), intent(out)  inform
//
//       allocate(w.A(n,n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate(w.LtL(n,n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate(w.v(n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate(w.q(n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate(w.y1(n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate(w.AplusSigma(n,n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//
//       call setup_workspace_min_eig_symm(n,m,w.min_eig_symm_ws,options,inform)
//       if (inform.status > 0) goto 9010
//
//       if (options.scale > 0) then
//          call setup_workspace_apply_scaling(n,m,w.apply_scaling_ws,options,inform)
//          if (inform.status > 0) goto 9010
//       end if
//
//       return
//
//9000   continue ! allocation error here
//       inform.status = ERROR.ALLOCATION
//       inform.bad_alloc = "more_sorenesen"
//       return
//
//9010   continue ! error from called subroutine
//       return
//
//} // setup_workspace_more_sorensen

//calculate_step_work::calculate_step_work(int n, int m, int nlls_method) {
//  if (nlls_method == 3) { //! More-Sorensen 
//    setup_workspace_more_sorensen(n,m, more_sorensen_ws,options,inform);
//  } //else if (options.nlls_method == 4) { //! dtrs (Galahad)
//  //   setup_workspace_solve_dtrs(n,m, 
//  //        calculate_step_ws.solve_dtrs_ws, options, inform);
//  //} else {
//  //  throw std::logic_error("Initialization: method not implemented.");
//  //}
//}

NLLS_workspace::NLLS_workspace(int n, int m,const nlls_options& options, nlls_inform& inform) :
 first_call(0),
 iter(0),
 normF0(), normJF0(), normF(), normJF(),
 normJFold(), normJF_Newton(),
 Delta(),
 normd(),
 use_second_derivatives(false),
 hybrid_count(0),
 hybrid_tol(1.0),
 tr_nu(options.radius_increase),
 tr_p(7),
 /* DoubleFortranMatrix */ fNewton(), JNewton(), XNewton(),
 /* DoubleFortranVector */ J(m, n),
 /* DoubleFortranVector */ f(m), fnew(m),
 /* DoubleFortranMatrix */ hf(n,n), //hf_temp(),
 /* DoubleFortranVector */ d(n), g(n), Xnew(n),
 /* DoubleFortranVector */ y(n), y_sharp(n),// g_old(), g_mixed(),
 /* DoubleFortranVector */ //ysharpSks(), Sks(),
 /* DoubleFortranVector */ //resvec(), gradvec(),
 /* DoubleFortranVector */ largest_sv(), smallest_sv()
{
//
//       type( NLLS_workspace ), intent(out)  workspace
//       type( nlls_options ), intent(in)  options
//       integer, intent(in)  n,m
//       type( NLLS_inform ), intent(out)  inform

       y.zero();
       y_sharp.zero();

       if (! options.exact_second_derivatives) then
         g_old.allocate(n);
         g_mixed.allocate(n);
         Sks.allocate(n);
         ysharpSks.allocate(n);
       end_if

       if( options.output_progress_vectors ) then 
         resvec.allocate(options.maxit+1);
         gradvec.allocate(options.maxit+1);
       end_if

       if( options.calculate_svd_J ) then
         largest_sv.allocate(options.maxit + 1);
         smallest_sv.allocate(options.maxit + 1);
       end_if

       if( options.model == 3 ) then
          hf_temp.allocate(n, n);
       end_if


//
//       ! evaluate model in the main routine...       
//       call setup_workspace_evaluate_model(n,m,workspace.evaluate_model_ws,options,inform)
} // setup_workspaces

/// Calculate the 2-norm of a vector: sqrt(||V||^2)
//double norm2(const DoubleFortranVector& v) {
//  return v.norm();
//}

//
//     subroutine setup_workspace_solve_LLS(n,m,w,options,inform)
//       integer, intent(in)  n, m 
//       type( solve_LLS_work )  w 
//       type( nlls_options ), intent(in)  options
//       type( nlls_inform ), intent(out)  inform
//       integer  lwork
//
//       allocate( w.temp(max(m,n)), stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       lwork = max(1, min(m,n) + max(min(m,n), 1)*4) 
//       allocate( w.work(lwork), stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate( w.Jlls(n*m), stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//
//       return
//
//9000   continue  ! local allocation error
//       inform.status = ERROR.ALLOCATION
//       inform.bad_alloc = "solve_LLS"
//       return
//
//     end subroutine setup_workspace_solve_LLS
//
//
//     subroutine setup_workspace_evaluate_model(n,m,w,options,inform)
//       integer, intent(in)  n, m        
//       type( evaluate_model_work )  w
//       type( nlls_options ), intent(in)  options
//       type( nlls_inform ), intent(out)  inform
//
//       allocate( w.Jd(m), stat = inform.alloc_status )
//       if (inform.alloc_status > 0) goto 9000
//       allocate( w.Hd(n), stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//
//       return
//
//9000   continue
//       inform.status = ERROR.ALLOCATION
//       inform.bad_alloc = 'evaluate_model'
//       return
//     end subroutine setup_workspace_evaluate_model
//

//
//     subroutine setup_workspace_min_eig_symm(n,m,w,options,inform)
//       integer, intent(in)  n, m 
//       type( min_eig_symm_work)  w
//       type( nlls_options ), intent(in)  options
//       type( nlls_inform ), intent(out)  inform
//
//       real, allocatable  workquery(:)
//       integer  lwork, eigsout
//
//       allocate(w.A(n,n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//
//       allocate(workquery(1),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//
//       if (options.subproblem_eig_fact) then 
//          allocate(w.ew(n), stat = inform.alloc_status)
//          if (inform.alloc_status > 0) goto 9000
//
//          call dsyev('V', & ! both ew's and ev's 
//               'U', & ! upper triangle of A
//               n, w.A, n, & ! data about A
//               w.ew, workquery, -1, & 
//               inform.external_return)
//          if (inform.external_return .ne. 0) goto 9010
//       else
//          allocate( w.iwork(5*n), stat = inform.alloc_status )
//          if (inform.alloc_status > 0) goto 9000
//          allocate( w.ifail(n), stat = inform.alloc_status ) 
//          if (inform.alloc_status > 0) goto 9000
//
//          ! make a workspace query to dsyevx
//          call dsyevx( 'V',& ! get both ew's and ev's
//               'I',& ! just the numbered eigenvalues
//               'U',& ! upper triangle of A
//               n, w.A, n, & 
//               1.0, 1.0, & ! not used for RANGE = 'I'
//               1, 1, & ! only find the first eigenpair
//               0.5, & ! abstol for the eigensolver
//               eigsout, & ! total number of eigs found
//               1.0, 1.0, & ! the eigenvalue and eigenvector
//               n, & ! ldz (the eigenvector array)
//               workquery, -1, w.iwork, &  ! workspace
//               w.ifail, & ! array containing indicies of non-converging ews
//               inform.external_return)
//          if (inform.external_return .ne. 0) goto 9020
//       end if
//       lwork = int(workquery(1))
//       deallocate(workquery)
//       allocate( w.work(lwork), stat = inform.alloc_status )
//       if (inform.alloc_status > 0) goto 9000
//
//       return
//
//9000   continue      
//       inform.status = ERROR.ALLOCATION
//       inform.bad_alloc = "min_eig_symm"
//       return
//
//9010   continue
//       inform.status = ERROR.FROM_EXTERNAL
//       inform.external_name = "lapack_dsyev"
//
//9020   continue
//       inform.status = ERROR.FROM_EXTERNAL
//       inform.external_name = "lapack_dsyevx"
//       return
//
//     end subroutine setup_workspace_min_eig_symm
//
//     subroutine remove_workspace_min_eig_symm(w,options)
//       type( min_eig_symm_work)  w
//       type( nlls_options ), intent(in)  options
//
//       if(allocated( w.A )) deallocate(w.A)        
//
//       if (options.subproblem_eig_fact) then 
//          if(allocated( w.ew )) deallocate(w.ew)
//       else
//          if(allocated( w.iwork )) deallocate( w.iwork )
//          if(allocated( w.ifail )) deallocate( w.ifail ) 
//       end if
//       if(allocated( w.work )) deallocate( w.work ) 
//
//       return
//
//     end subroutine remove_workspace_min_eig_symm
//
//     subroutine setup_workspace_max_eig(n,m,w,options,inform)
//       integer, intent(in)  n, m 
//       type( max_eig_work)  w
//       type( nlls_options ), intent(in)  options
//       type( nlls_inform), intent(out)  inform
//       real, allocatable  workquery(:)
//       integer  lwork
//
//       allocate( w.alphaR(2*n), stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       w.alphaR = zero
//       allocate( w.alphaI(2*n), stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       w.alphaI = zero
//       allocate( w.beta(2*n),   stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       w.beta = zero
//       allocate( w.vr(2*n,2*n), stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate( w.ew_array(2*n), stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       w.ew_array = zero
//       allocate(workquery(1),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       ! make a workspace query to dggev
//       call dggev('N', & ! No left eigenvectors
//            'V', &! Yes right eigenvectors
//            2*n, 1.0, 2*n, 1.0, 2*n, &
//            1.0, 0.1, 0.1, & ! eigenvalue data
//            0.1, 2*n, & ! not referenced
//            0.1, 2*n, & ! right eigenvectors
//            workquery, -1, inform.external_return)
//       if (inform.external_return > 0) goto 9020
//       lwork = int(workquery(1))
//       deallocate(workquery)
//       allocate( w.work(lwork), stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate( w.nullindex(2*n), stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate( w.vecisreal(2*n), stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//
//       return
//
//9000   continue
//       inform.status = ERROR.ALLOCATION
//       inform.bad_alloc = "max_eig"
//       return
//
//9020   continue
//       inform.status = ERROR.FROM_EXTERNAL
//       inform.external_name = "lapack_dggev"
//       return
//
//     end subroutine setup_workspace_max_eig
//
//     subroutine remove_workspace_max_eig(w,options)
//       type( max_eig_work)  w
//       type( nlls_options ), intent(in)  options
//
//       if(allocated( w.alphaR )) deallocate( w.alphaR)
//       if(allocated( w.alphaI )) deallocate( w.alphaI )
//       if(allocated( w.beta )) deallocate( w.beta ) 
//       if(allocated( w.vr )) deallocate( w.vr ) 
//       if(allocated( w.ew_array )) deallocate( w.ew_array ) 
//       if(allocated( w.work )) deallocate( w.work ) 
//       if(allocated( w.nullindex )) deallocate( w.nullindex ) 
//       if(allocated( w.vecisreal )) deallocate( w.vecisreal )
//
//       return
//
//     end subroutine remove_workspace_max_eig
//
//     subroutine setup_workspace_solve_general(n, m, w, options, inform)
//       integer, intent(in)  n, m 
//       type( solve_general_work )  w
//       type( nlls_options ), intent(in)  options
//       type( nlls_inform), intent(out)  inform
//
//       allocate( w.A(n,n), stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate( w.ipiv(n), stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//
//       return
//
//9000   continue ! allocation error
//       inform.status = ERROR.ALLOCATION
//       inform.bad_alloc = "solve_general"
//       return
//
//     end subroutine setup_workspace_solve_general
//
//     subroutine remove_workspace_solve_general(w, options)
//       type( solve_general_work )  w
//       type( nlls_options ), intent(in)  options
//
//       if(allocated( w.A )) deallocate( w.A ) 
//       if(allocated( w.ipiv )) deallocate( w.ipiv ) 
//       return
//
//     end subroutine remove_workspace_solve_general
//
//     subroutine setup_workspace_solve_dtrs(n,m,w,options,inform)
//       integer, intent(in)  n,m
//       type( solve_dtrs_work )  w
//       type( nlls_options ), intent(in)  options
//       type( nlls_inform ), intent(out)  inform
//
//       allocate(w.A(n,n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate(w.ev(n,n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate(w.v(n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate(w.v_trans(n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate(w.ew(n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//       allocate(w.d_trans(n),stat = inform.alloc_status)
//       if (inform.alloc_status > 0) goto 9000
//
//       call setup_workspace_all_eig_symm(n,m,w.all_eig_symm_ws,options,inform)
//       if (inform.status > 0) goto 9010
//
//       if (options.scale > 0) then
//          call setup_workspace_apply_scaling(n,m,w.apply_scaling_ws,options,inform)
//          if (inform.status > 0) goto 9010
//       end if
//
//       return
//
//9000   continue ! allocation error here
//       inform.status = ERROR.ALLOCATION
//       inform.bad_alloc = "solve_dtrs"
//       return
//
//9010   continue  ! allocation error from called subroutine
//       return
//
//     end subroutine setup_workspace_solve_dtrs
//
//     subroutine remove_workspace_solve_dtrs(w,options)
//       type( solve_dtrs_work )  w
//       type( nlls_options ), intent(in)  options
//
//       if(allocated( w.A )) deallocate(w.A)
//       if(allocated( w.ev )) deallocate(w.ev)
//       if(allocated( w.v )) deallocate(w.v)
//       if(allocated( w.v_trans )) deallocate(w.v_trans)
//       if(allocated( w.ew )) deallocate(w.ew)
//       if(allocated( w.d_trans )) deallocate(w.d_trans)
//
//       call remove_workspace_all_eig_symm(w.all_eig_symm_ws,options)
//       if (options.scale > 0) then
//          call remove_workspace_apply_scaling(w.apply_scaling_ws,options)
//       end if
//
//       return
//
//     end subroutine remove_workspace_solve_dtrs
//
//     subroutine setup_workspace_all_eig_symm(n,m,w,options,inform)
//       integer, intent(in)  n,m
//       type( all_eig_symm_work )  w
//       type( nlls_options ), intent(in)  options
//       type( nlls_inform ), intent(out)  inform
//
//       real, allocatable  workquery(:)
//       real  A,ew
//       integer  lwork
//
//       A = 1.0_wp
//       ew = 1.0_wp
//
//       allocate(workquery(1), stat = inform.alloc_status)
//       if (inform.alloc_status .ne. 0 ) goto 8000
//       call dsyev('V', & ! both ew's and ev's 
//            'U', & ! upper triangle of A
//            n, A, n, & ! data about A
//            ew, workquery, -1, & 
//            inform.external_return)  
//       if (inform.external_return .ne. 0) goto 9000
//
//       lwork = int(workquery(1))
//       deallocate(workquery)
//       allocate( w.work(lwork), stat = inform.alloc_status )
//       if (inform.alloc_status > 0) goto 8000
//
//       return
//
//8000   continue  ! allocation error
//       inform.status = ERROR.ALLOCATION
//       inform.bad_alloc = "all_eig_sym"
//       return
//
//9000   continue ! error from lapack
//       inform.status = ERROR.FROM_EXTERNAL
//       inform.external_name = "lapack_dsyev"
//       return
//
//     end subroutine setup_workspace_all_eig_symm




//     subroutine setup_workspace_apply_scaling(n,m,w,options,inform)
//
//       integer, intent(in)  n,m
//       type( apply_scaling_work )  w
//       type( nlls_options ), intent(in)  options
//       type( nlls_inform ), intent(out)  inform
//
//       allocate(w.diag(n), stat = inform.alloc_status )
//       if (inform.alloc_status .ne. 0) goto 1000
//       w.diag = one
//       allocate(w.ev(n,n), stat = inform.alloc_status )
//       if (inform.alloc_status .ne. 0) goto 1000
//
//       if (options.scale == 4) then
//          allocate(w.tempvec(n))
//          call setup_workspace_all_eig_symm(n,m,w.all_eig_symm_ws,options,inform)
//          if (inform.status .ne. 0) goto 1010
//       end if
//
//       return
//
//1000   continue ! allocation error here
//       inform.status = ERROR.ALLOCATION
//       inform.bad_alloc = "apply_scaling"
//       return
//
//1010   continue ! error from lower down subroutine
//       return
//
//     end subroutine setup_workspace_apply_scaling
//


} // namespace RalNlls
}
}
