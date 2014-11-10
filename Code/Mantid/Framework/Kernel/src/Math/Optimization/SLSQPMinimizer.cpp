#include "MantidKernel/Math/Optimization/SLSQPMinimizer.h"
#include "MantidKernel/Exception.h"

#include <algorithm>
#include <cmath>
#include <cassert>
#include <sstream>

namespace Mantid { namespace Kernel {

  namespace Math
  {
    namespace
    {
      ///@cond
      // Forward-declaration of minimizer
      int slsqp_(int *m, int *meq, int *la, int *n,
          double *x, double *xl, double *xu, double *f,
          double *c__, double *g, double *a, double *acc,
          int *iter, int *mode, double *w, int *l_w__, int *
          jw, int *l_jw__);
      ///@endcond
    }

    /**
     * Perform the minimization using the SLSQP routine
     * @param x0 The starting parameter values
     * @return
     */
    std::vector<double> SLSQPMinimizer::minimize(const std::vector<double> & x0) const
    {
      assert( numParameters() == x0.size());

      // slsqp parameters
      int m = static_cast<int>(numEqualityConstraints() + numInequalityConstraints());
      int meq = static_cast<int>(numEqualityConstraints());
      int la = std::max(1, m);
      int n = static_cast<int>(numParameters());

      std::vector<double> x = x0;
      std::vector<double> xl(n,-1e12), xu(n, 1e12); //bounds
      // Function value holder
      double fx(0.0);
      // Constraint value holder
      std::vector<double> constraintValues(m, 0.0);

      //slsqp requires gradient array to be one longer than number of parameters
      std::vector<double> gradient(m_nparams + 1, 0.0);

      double acc = 1e-06; // acceptance
      int majiter = 100;
      int mode = 0; // Starts in mode 0

      // workspace spaces for slsqp
      int n1 = n+1;
      int mineq = m - meq + n1 + n1;
      int len_w = (3*n1+m)*(n1+1)+(n1-meq+1)*(mineq+2) + \
        2*mineq+(n1+mineq)*(n1-meq) + \
        2*meq + n1 +(n+1)*n/2 + 2*m + 3*n + 3*n1 + 1;
      int len_jw = mineq;
      std::vector<double> vec_w(len_w, 0.0);
      double * w = vec_w.data();
      std::vector<int> vec_jw(len_jw,0);
      int * jw = vec_jw.data();

      while(true)
      {
        if(mode == 0 || mode == 1)// objective and constraint evaluation required
        {
          // Compute objective function
          fx = fvalue(x);
          evaluateConstraints(constraintValues,x);
        }
        if(mode == 0 || mode == -1) // gradient evaluation required
        {
          // Compute the derivatives of the objective function
          fprime(gradient,x);
        }
        // Call SLSQP
        slsqp_(&m, &meq, &la, &n, x.data(), xl.data(), xu.data(), &fx, constraintValues.data(),
               gradient.data(), m_constraintNorms.data(), &acc,
               &majiter, &mode, w, &len_w, jw, &len_jw);

        // If exit mode is not -1 or 1, slsqp has completed
        if(mode != 1 && mode != -1) break;
      }
      return x;
    }

  /**
   * Computes the gradient w.r.t to each paramter
   * @param grad Output array to store gradient
   * @param x Current x pt
   */
  void SLSQPMinimizer::fprime(std::vector<double> & grad, const std::vector<double> & x) const
  {
    assert(grad.size() > numParameters() ); // > for slsqp

    const double epsilon(1e-08);

    double f0 = fvalue(x);
    std::vector<double> tmp = x;
    for(size_t i = 0; i < numParameters(); ++i)
    {
      double & xi = tmp[i];
      const double curx = xi; //copy
      xi += epsilon;
      grad[i] = (fvalue(tmp) - f0)/epsilon;
      xi = curx;
    }
  }

  /**
   * Compute values of constraints at current x point
   * @param constrValues Output array to store values
   * @param x Values of the parameters
   */
  void SLSQPMinimizer::evaluateConstraints(std::vector<double> & constrValues, const std::vector<double> & x) const
  {
    assert(numEqualityConstraints() + numInequalityConstraints() == constrValues.size());

    // Need to compute position in flat array
    const size_t numConstrs(constrValues.size());
    for(size_t i = 0; i < numConstrs; ++i)
    {
      double value(0.0);
      for(size_t j = 0; j < numParameters(); ++j)
      {
        value += m_constraintNorms[j*numConstrs + i]*x[j];
      }
      constrValues[i] = value;
    }
  }


  /**
   * Create constraint array
   * @param equality A matrix of equality constraints \f$A_{eq}\f$(the number of columns must match number of parameters)
   *                 where \f$A_{eq} x = 0\f$
   * @param inequality A matrix of inequality constraints (the number of columns must match number of parameters
   *                 where \f$A_{eq} x \geq 0\f$
   */
  void SLSQPMinimizer::initializeConstraints(const DblMatrix & equality, const DblMatrix & inequality)
  {
    const size_t totalNumConstr = numEqualityConstraints() + numInequalityConstraints();
    if(totalNumConstr == 0) return;

    // Sanity checks on matrix sizes
    for(size_t i = 0; i < 2; ++i)
    {
      size_t ncols(0);
      std::string matrix("");
      if(i == 0)
      {
        ncols = equality.numCols();
        matrix = "equality";
      }
      else
      {
        ncols = inequality.numCols();
        matrix = "inequality";
      }

      if(ncols > 0 && ncols != numParameters())
      {
        std::ostringstream os;
        os << "SLSQPMinimizer::initializeConstraints - Invalid " << matrix << " constraint matrix. Number of columns must match number of parameters. ncols="
            << ncols << ", nparams=" << numParameters();
        throw std::invalid_argument(os.str());
      }
    }


    // --- Constraint Vector Layout As Required By SLSQP -----
    // The equality constraints are specified first, followed by the inequality constraints. The constraints
    // should be written column-wise such that to get move between successive values of the same constraint you
    // jump (numEqualityConstraints()+numInequalityConstraints()) in the array. The array is then padded
    // by (numEqualityConstraints()+numInEqualityConstraints()) zeroes
    const size_t constrVecSize = totalNumConstr*numParameters() + totalNumConstr;
    m_constraintNorms.resize(constrVecSize, 0.0);

    size_t constrCounter(0);
    while(constrCounter < totalNumConstr)
    {
      const DblMatrix *constrMatrix(NULL);
      if(constrCounter < numEqualityConstraints()) constrMatrix = &equality;
      else constrMatrix = &inequality;

      for(size_t i = 0; i < constrMatrix->numRows(); ++i, ++constrCounter)
      {
        const double * matrixRow = (*constrMatrix)[i];
        for(size_t j = 0; j < constrMatrix->numCols(); ++j)
        {
          m_constraintNorms[j*totalNumConstr + constrCounter] = matrixRow[j];
        }
      }
    }
    assert(totalNumConstr == constrCounter);
  }


namespace
{
  ///@cond

  //---------------------------------------------------------------------------------------------------//
  //
  // The following code was translated from fortran using f2c and then hand-cleaned so that it will
  // compile without the f2c headers.
  //
  // The main entry point is the slsqp_ function. All others are helpers.
  //
  // By hand changes:
  //     - doublereal --> double
  //     - integer --> int
  //     - real --> float
  //
  // Line 2639: Commented out next_fmt declaration and corresponding fmt_ statements that cause compiler
  //            warnings and are unused
  //---------------------------------------------------------------------------------------------------//

  /* Table of constant values */
  static int c__0 = 0;
  static int c__1 = 1;
  static int c__2 = 2;

  double d_sign(double *a, double *b)
  {
    double x;
    x = (*a >= 0 ? *a : - *a);
    return( *b >= 0 ? x : -x);
  }

  // --------------------- Forward declarations of helpers ---------------------
  int slsqpb_(int *, int *, int *,
          int *, double *, double *, double *, double *,
           double *, double *, double *, double *, int *
          , int *, double *, double *, double *, double
          *, double *, double *, double *, double *,
          int *);
  int dcopy___(int *n, double *dx, int *incx,
      double *dy, int *incy);
  int daxpy_sl__(int *n, double *da, double *dx,
                 int *incx, double *dy, int *incy);
  int lsq_(int *m, int *meq, int *n, int *nl,
           int *la, double *l, double *g, double *a, double *
           b, double *xl, double *xu, double *x, double *y,
           double *w, int *jw, int *mode);
  double ddot_sl__(int *n, double *dx, int *incx, double * dy,
                   int *incy);
  int dscal_sl__(int *n, double *da, double *dx,
                 int *incx);
  double linmin_(int *mode, double *ax, double *bx, double *
                 f, double *tol);
  double dnrm2___(int *n, double *dx, int *incx);
  int ldl_(int *n, double *a, double *z__,
           double *sigma, double *w);
  int lsei_(double *c__, double *d__, double *e,
            double *f, double *g, double *h__, int *lc, int *
            mc, int *le, int *me, int *lg, int *mg, int *n,
            double *x, double *xnrm, double *w, int *jw, int * mode);
  int h12_(int *mode, int *lpivot, int *l1,
           int *m, double *u, int *iue, double *up, double *c__,
           int *ice, int *icv, int *ncv);
  int hfti_(double *a, int *mda, int *m, int *n,
            double *b, int *mdb, int *nb, double *tau, int
            *krank, double *rnorm, double *h__, double *g, int *ip);
  int lsi_(double *e, double *f, double *g,
           double *h__, int *le, int *me, int *lg, int *mg,
           int *n, double *x, double *xnorm, double *w, int *jw,
           int *mode);
  int ldp_(double *g, int *mg, int *m, int *n,
           double *h__, double *x, double *xnorm, double *w,
           int *index, int *mode);
  int nnls_(double *a, int *mda, int *m, int *n,
            double *b, double *x, double *rnorm, double *w,
            double *z__, int *index, int *mode);
  int dsrotg_(double *da, double *db, double *c__,
              double *s);
  int dsrot_(int *n, double *dx, int *incx,
             double *dy, int *incy, double *c__, double *s);
  //----------------------------------------------------------------------------

  /*      ALGORITHM 733, COLLECTED ALGORITHMS FROM ACM. */
  /*      TRANSACTIONS ON MATHEMATICAL SOFTWARE, */
  /*      VOL. 20, NO. 3, SEPTEMBER, 1994, PP. 262-281. */
  /*      http://doi.acm.org/10.1145/192115.192124 */


  /*      http://permalink.gmane.org/gmane.comp.python.scientific.devel/6725 */
  /*      ------ */
  /*      From: Deborah Cotton <cotton@hq.acm.org> */
  /*      Date: Fri, 14 Sep 2007 12:35:55 -0500 */
  /*      Subject: RE: Algorithm License requested */
  /*      To: Alan Isaac */

  /*      Prof. Issac, */

  /*      In that case, then because the author consents to [the ACM] releasing */
  /*      the code currently archived at http://www.netlib.org/toms/733 under the */
  /*      BSD license, the ACM hereby releases this code under the BSD license. */

  /*      Regards, */

  /*      Deborah Cotton, Copyright & Permissions */
  /*      ACM Publications */
  /*      2 Penn Plaza, Suite 701** */
  /*      New York, NY 10121-0701 */
  /*      permissions@acm.org */
  /*      212.869.7440 ext. 652 */
  /*      Fax. 212.869.0481 */
  /*      ------ */

  /* *********************************************************************** */
  /*                              optimizer                               * */
  /* *********************************************************************** */
  int slsqp_(int *m, int *meq, int *la, int *n,
     double *x, double *xl, double *xu, double *f,
    double *c__, double *g, double *a, double *acc,
    int *iter, int *mode, double *w, int *l_w__, int *
    jw, int *l_jw__)
  {
      /* System generated locals */
      int a_dim1, a_offset, i__1, i__2;

      /* Local variables */
      static int n1, il, im, ir, is, iu, iv, iw, ix, mineq;

  /*   SLSQP       S EQUENTIAL  L EAST  SQ UARES  P ROGRAMMING */
  /*            TO SOLVE GENERAL NONLINEAR OPTIMIZATION PROBLEMS */
  /* *********************************************************************** */
  /* *                                                                     * */
  /* *                                                                     * */
  /* *            A NONLINEAR PROGRAMMING METHOD WITH                      * */
  /* *            QUADRATIC  PROGRAMMING  SUBPROBLEMS                      * */
  /* *                                                                     * */
  /* *                                                                     * */
  /* *  THIS SUBROUTINE SOLVES THE GENERAL NONLINEAR PROGRAMMING PROBLEM   * */
  /* *                                                                     * */
  /* *            MINIMIZE    F(X)                                         * */
  /* *                                                                     * */
  /* *            SUBJECT TO  C (X) .EQ. 0  ,  J = 1,...,MEQ               * */
  /* *                         J                                           * */
  /* *                                                                     * */
  /* *                        C (X) .GE. 0  ,  J = MEQ+1,...,M             * */
  /* *                         J                                           * */
  /* *                                                                     * */
  /* *                        XL .LE. X .LE. XU , I = 1,...,N.             * */
  /* *                          I      I       I                           * */
  /* *                                                                     * */
  /* *  THE ALGORITHM IMPLEMENTS THE METHOD OF HAN AND POWELL              * */
  /* *  WITH BFGS-UPDATE OF THE B-MATRIX AND L1-TEST FUNCTION              * */
  /* *  WITHIN THE STEPLENGTH ALGORITHM.                                   * */
  /* *                                                                     * */
  /* *    PARAMETER DESCRIPTION:                                           * */
  /* *    ( * MEANS THIS PARAMETER WILL BE CHANGED DURING CALCULATION )    * */
  /* *                                                                     * */
  /* *    M              IS THE TOTAL NUMBER OF CONSTRAINTS, M .GE. 0      * */
  /* *    MEQ            IS THE NUMBER OF EQUALITY CONSTRAINTS, MEQ .GE. 0 * */
  /* *    LA             SEE A, LA .GE. MAX(M,1)                           * */
  /* *    N              IS THE NUMBER OF VARIBLES, N .GE. 1               * */
  /* *  * X()            X() STORES THE CURRENT ITERATE OF THE N VECTOR X  * */
  /* *                   ON ENTRY X() MUST BE INITIALIZED. ON EXIT X()     * */
  /* *                   STORES THE SOLUTION VECTOR X IF MODE = 0.         * */
  /* *    XL()           XL() STORES AN N VECTOR OF LOWER BOUNDS XL TO X.  * */
  /* *    XU()           XU() STORES AN N VECTOR OF UPPER BOUNDS XU TO X.  * */
  /* *    F              IS THE VALUE OF THE OBJECTIVE FUNCTION.           * */
  /* *    C()            C() STORES THE M VECTOR C OF CONSTRAINTS,         * */
  /* *                   EQUALITY CONSTRAINTS (IF ANY) FIRST.              * */
  /* *                   DIMENSION OF C MUST BE GREATER OR EQUAL LA,       * */
  /* *                   which must be GREATER OR EQUAL MAX(1,M).          * */
  /* *    G()            G() STORES THE N VECTOR G OF PARTIALS OF THE      * */
  /* *                   OBJECTIVE FUNCTION; DIMENSION OF G MUST BE        * */
  /* *                   GREATER OR EQUAL N+1.                             * */
  /* *    A(),LA,M,N     THE LA BY N + 1 ARRAY A() STORES                  * */
  /* *                   THE M BY N MATRIX A OF CONSTRAINT NORMALS.        * */
  /* *                   A() HAS FIRST DIMENSIONING PARAMETER LA,          * */
  /* *                   WHICH MUST BE GREATER OR EQUAL MAX(1,M).          * */
  /* *    F,C,G,A        MUST ALL BE SET BY THE USER BEFORE EACH CALL.     * */
  /* *  * ACC            ABS(ACC) CONTROLS THE FINAL ACCURACY.             * */
  /* *                   IF ACC .LT. ZERO AN EXACT LINESEARCH IS PERFORMED,* */
  /* *                   OTHERWISE AN ARMIJO-TYPE LINESEARCH IS USED.      * */
  /* *  * ITER           PRESCRIBES THE MAXIMUM NUMBER OF ITERATIONS.      * */
  /* *                   ON EXIT ITER INDICATES THE NUMBER OF ITERATIONS.  * */
  /* *  * MODE           MODE CONTROLS CALCULATION:                        * */
  /* *                   REVERSE COMMUNICATION IS USED IN THE SENSE THAT   * */
  /* *                   THE PROGRAM IS INITIALIZED BY MODE = 0; THEN IT IS* */
  /* *                   TO BE CALLED REPEATEDLY BY THE USER UNTIL A RETURN* */
  /* *                   WITH MODE .NE. IABS(1) TAKES PLACE.               * */
  /* *                   IF MODE = -1 GRADIENTS HAVE TO BE CALCULATED,     * */
  /* *                   WHILE WITH MODE = 1 FUNCTIONS HAVE TO BE CALCULATED */
  /* *                   MODE MUST NOT BE CHANGED BETWEEN SUBSEQUENT CALLS * */
  /* *                   OF SQP.                                           * */
  /* *                   EVALUATION MODES:                                 * */
  /* *        MODE = -1: GRADIENT EVALUATION, (G&A)                        * */
  /* *                0: ON ENTRY: INITIALIZATION, (F,G,C&A)               * */
  /* *                   ON EXIT : REQUIRED ACCURACY FOR SOLUTION OBTAINED * */
  /* *                1: FUNCTION EVALUATION, (F&C)                        * */
  /* *                                                                     * */
  /* *                   FAILURE MODES:                                    * */
  /* *                2: NUMBER OF EQUALITY CONTRAINTS LARGER THAN N       * */
  /* *                3: MORE THAN 3*N ITERATIONS IN LSQ SUBPROBLEM        * */
  /* *                4: INEQUALITY CONSTRAINTS INCOMPATIBLE               * */
  /* *                5: SINGULAR MATRIX E IN LSQ SUBPROBLEM               * */
  /* *                6: SINGULAR MATRIX C IN LSQ SUBPROBLEM               * */
  /* *                7: RANK-DEFICIENT EQUALITY CONSTRAINT SUBPROBLEM HFTI* */
  /* *                8: POSITIVE DIRECTIONAL DERIVATIVE FOR LINESEARCH    * */
  /* *                9: MORE THAN ITER ITERATIONS IN SQP                  * */
  /* *             >=10: WORKING SPACE W OR JW TOO SMALL,                  * */
  /* *                   W SHOULD BE ENLARGED TO L_W=MODE/1000             * */
  /* *                   JW SHOULD BE ENLARGED TO L_JW=MODE-1000*L_W       * */
  /* *  * W(), L_W       W() IS A ONE DIMENSIONAL WORKING SPACE,           * */
  /* *                   THE LENGTH L_W OF WHICH SHOULD BE AT LEAST        * */
  /* *                   (3*N1+M)*(N1+1)                        for LSQ    * */
  /* *                  +(N1-MEQ+1)*(MINEQ+2) + 2*MINEQ         for LSI    * */
  /* *                  +(N1+MINEQ)*(N1-MEQ) + 2*MEQ + N1       for LSEI   * */
  /* *                  + N1*N/2 + 2*M + 3*N + 3*N1 + 1         for SLSQPB * */
  /* *                   with MINEQ = M - MEQ + 2*N1  &  N1 = N+1          * */
  /* *        NOTICE:    FOR PROPER DIMENSIONING OF W IT IS RECOMMENDED TO * */
  /* *                   COPY THE FOLLOWING STATEMENTS INTO THE HEAD OF    * */
  /* *                   THE CALLING PROGRAM (AND REMOVE THE COMMENT C)    * */
  /* ####################################################################### */
  /*     INT LEN_W, LEN_JW, M, N, N1, MEQ, MINEQ */
  /*     PARAMETER (M=... , MEQ=... , N=...  ) */
  /*     PARAMETER (N1= N+1, MINEQ= M-MEQ+N1+N1) */
  /*     PARAMETER (LEN_W= */
  /*    $           (3*N1+M)*(N1+1) */
  /*    $          +(N1-MEQ+1)*(MINEQ+2) + 2*MINEQ */
  /*    $          +(N1+MINEQ)*(N1-MEQ) + 2*MEQ + N1 */
  /*    $          +(N+1)*N/2 + 2*M + 3*N + 3*N1 + 1, */
  /*    $           LEN_JW=MINEQ) */
  /*     DOUBLE PRECISION W(LEN_W) */
  /*     INT          JW(LEN_JW) */
  /* ####################################################################### */
  /* *                   THE FIRST M+N+N*N1/2 ELEMENTS OF W MUST NOT BE    * */
  /* *                   CHANGED BETWEEN SUBSEQUENT CALLS OF SLSQP.        * */
  /* *                   ON RETURN W(1) ... W(M) CONTAIN THE MULTIPLIERS   * */
  /* *                   ASSOCIATED WITH THE GENERAL CONSTRAINTS, WHILE    * */
  /* *                   W(M+1) ... W(M+N(N+1)/2) STORE THE CHOLESKY FACTOR* */
  /* *                   L*D*L(T) OF THE APPROXIMATE HESSIAN OF THE        * */
  /* *                   LAGRANGIAN COLUMNWISE DENSE AS LOWER TRIANGULAR   * */
  /* *                   UNIT MATRIX L WITH D IN ITS 'DIAGONAL' and        * */
  /* *                   W(M+N(N+1)/2+N+2 ... W(M+N(N+1)/2+N+2+M+2N)       * */
  /* *                   CONTAIN THE MULTIPLIERS ASSOCIATED WITH ALL       * */
  /* *                   ALL CONSTRAINTS OF THE QUADRATIC PROGRAM FINDING  * */
  /* *                   THE SEARCH DIRECTION TO THE SOLUTION X*           * */
  /* *  * JW(), L_JW     JW() IS A ONE DIMENSIONAL INT WORKING SPACE   * */
  /* *                   THE LENGTH L_JW OF WHICH SHOULD BE AT LEAST       * */
  /* *                   MINEQ                                             * */
  /* *                   with MINEQ = M - MEQ + 2*N1  &  N1 = N+1          * */
  /* *                                                                     * */
  /* *  THE USER HAS TO PROVIDE THE FOLLOWING SUBROUTINES:                 * */
  /* *     LDL(N,A,Z,SIG,W) :   UPDATE OF THE LDL'-FACTORIZATION.          * */
  /* *     LINMIN(A,B,F,TOL) :  LINESEARCH ALGORITHM IF EXACT = 1          * */
  /* *     LSQ(M,MEQ,LA,N,NC,C,D,A,B,XL,XU,X,LAMBDA,W,....) :              * */
  /* *                                                                     * */
  /* *        SOLUTION OF THE QUADRATIC PROGRAM                            * */
  /* *                QPSOL IS RECOMMENDED:                                * */
  /* *     PE GILL, W MURRAY, MA SAUNDERS, MH WRIGHT:                      * */
  /* *     USER'S GUIDE FOR SOL/QPSOL:                                     * */
  /* *     A FORTRAN PACKAGE FOR QUADRATIC PROGRAMMING,                    * */
  /* *     TECHNICAL REPORT SOL 83-7, JULY 1983                            * */
  /* *     DEPARTMENT OF OPERATIONS RESEARCH, STANFORD UNIVERSITY          * */
  /* *     STANFORD, CA 94305                                              * */
  /* *     QPSOL IS THE MOST ROBUST AND EFFICIENT QP-SOLVER                * */
  /* *     AS IT ALLOWS WARM STARTS WITH PROPER WORKING SETS               * */
  /* *                                                                     * */
  /* *     IF IT IS NOT AVAILABLE USE LSEI, A CONSTRAINT LINEAR LEAST      * */
  /* *     SQUARES SOLVER IMPLEMENTED USING THE SOFTWARE HFTI, LDP, NNLS   * */
  /* *     FROM C.L. LAWSON, R.J.HANSON: SOLVING LEAST SQUARES PROBLEMS,   * */
  /* *     PRENTICE HALL, ENGLEWOOD CLIFFS, 1974.                          * */
  /* *     LSEI COMES WITH THIS PACKAGE, together with all necessary SR's. * */
  /* *                                                                     * */
  /* *     TOGETHER WITH A COUPLE OF SUBROUTINES FROM BLAS LEVEL 1         * */
  /* *                                                                     * */
  /* *     SQP IS HEAD SUBROUTINE FOR BODY SUBROUTINE SQPBDY               * */
  /* *     IN WHICH THE ALGORITHM HAS BEEN IMPLEMENTED.                    * */
  /* *                                                                     * */
  /* *  IMPLEMENTED BY: DIETER KRAFT, DFVLR OBERPFAFFENHOFEN               * */
  /* *  as described in Dieter Kraft: A Software Package for               * */
  /* *                                Sequential Quadratic Programming     * */
  /* *                                DFVLR-FB 88-28, 1988                 * */
  /* *  which should be referenced if the user publishes results of SLSQP  * */
  /* *                                                                     * */
  /* *  DATE:           APRIL - OCTOBER, 1981.                             * */
  /* *  STATUS:         DECEMBER, 31-ST, 1984.                             * */
  /* *  STATUS:         MARCH   , 21-ST, 1987, REVISED TO FORTAN 77        * */
  /* *  STATUS:         MARCH   , 20-th, 1989, REVISED TO MS-FORTRAN       * */
  /* *  STATUS:         APRIL   , 14-th, 1989, HESSE   in-line coded       * */
  /* *  STATUS:         FEBRUARY, 28-th, 1991, FORTRAN/2 Version 1.04      * */
  /* *                                         accepts Statement Functions * */
  /* *  STATUS:         MARCH   ,  1-st, 1991, tested with SALFORD         * */
  /* *                                         FTN77/386 COMPILER VERS 2.40* */
  /* *                                         in protected mode           * */
  /* *                                                                     * */
  /* *********************************************************************** */
  /* *                                                                     * */
  /* *  Copyright 1991: Dieter Kraft, FHM                                  * */
  /* *                                                                     * */
  /* *********************************************************************** */
  /*     dim(W) =         N1*(N1+1) + MEQ*(N1+1) + MINEQ*(N1+1)  for LSQ */
  /*                    +(N1-MEQ+1)*(MINEQ+2) + 2*MINEQ          for LSI */
  /*                    +(N1+MINEQ)*(N1-MEQ) + 2*MEQ + N1        for LSEI */
  /*                    + N1*N/2 + 2*M + 3*N +3*N1 + 1           for SLSQPB */
  /*                      with MINEQ = M - MEQ + 2*N1  &  N1 = N+1 */
  /*   CHECK LENGTH OF WORKING ARRAYS */
      /* Parameter adjustments */
      --c__;
      a_dim1 = *la;
      a_offset = 1 + a_dim1;
      a -= a_offset;
      --g;
      --xu;
      --xl;
      --x;
      --w;
      --jw;

      /* Function Body */
      n1 = *n + 1;
      mineq = *m - *meq + n1 + n1;
      il = (n1 * 3 + *m) * (n1 + 1) + (n1 - *meq + 1) * (mineq + 2) + (mineq <<
        1) + (n1 + mineq) * (n1 - *meq) + (*meq << 1) + n1 * *n / 2 + (*m
        << 1) + *n * 3 + (n1 << 2) + 1;
  /* Computing MAX */
      i__1 = mineq, i__2 = n1 - *meq;
      im = std::max(i__1,i__2);
      if (*l_w__ < il || *l_jw__ < im) {
          *mode = std::max(10,il) * 1000;
    *mode += std::max(10,im);
    return 0;
      }
  /*   PREPARE DATA FOR CALLING SQPBDY  -  INITIAL ADDRESSES IN W */
      im = 1;
      il = im + std::max(1,*m);
      il = im + *la;
      ix = il + n1 * *n / 2 + 1;
      ir = ix + *n;
      is = ir + *n + *n + std::max(1,*m);
      is = ir + *n + *n + *la;
      iu = is + n1;
      iv = iu + n1;
      iw = iv + n1;

      slsqpb_(m, meq, la, n, &x[1], &xl[1], &xu[1], f, &c__[1], &g[1], &a[
        a_offset], acc, iter, mode, &w[ir], &w[il], &w[ix], &w[im], &w[is]
        , &w[iu], &w[iv], &w[iw], &jw[1]);
      return 0;
  } /* slsqp_ */

  int slsqpb_(int *m, int *meq, int *la, int *
    n, double *x, double *xl, double *xu, double *f,
    double *c__, double *g, double *a, double *acc,
    int *iter, int *mode, double *r__, double *l,
    double *x0, double *mu, double *s, double *u,
    double *v, double *w, int *iw)
  {
      /* Initialized data */

      static double zero = 0.;
      static double one = 1.;
      static double alfmin = .1;
      static double hun = 100.;
      static double ten = 10.;
      static double two = 2.;

      /* System generated locals */
      int a_dim1, a_offset, i__1, i__2;
      double d__1, d__2;

      /* Local variables */
      static int i__, j, k;
      static double t, f0, h1, h2, h3, h4;
      static int n1, n2, n3;
      static double t0, gs;
      static double tol;
      static int line;
      static double alpha;
      static int iexact;
      static int incons, ireset, itermx;

  /*   NONLINEAR PROGRAMMING BY SOLVING SEQUENTIALLY QUADRATIC PROGRAMS */
  /*        -  L1 - LINE SEARCH,  POSITIVE DEFINITE  BFGS UPDATE  - */
  /*                      BODY SUBROUTINE FOR SLSQP */
  /*     dim(W) =         N1*(N1+1) + MEQ*(N1+1) + MINEQ*(N1+1)  for LSQ */
  /*                     +(N1-MEQ+1)*(MINEQ+2) + 2*MINEQ */
  /*                     +(N1+MINEQ)*(N1-MEQ) + 2*MEQ + N1       for LSEI */
  /*                      with MINEQ = M - MEQ + 2*N1  &  N1 = N+1 */
      /* Parameter adjustments */
      --mu;
      --c__;
      --v;
      --u;
      --s;
      --x0;
      --l;
      --r__;
      a_dim1 = *la;
      a_offset = 1 + a_dim1;
      a -= a_offset;
      --g;
      --xu;
      --xl;
      --x;
      --w;
      --iw;

      /* Function Body */
      if (*mode < 0) {
    goto L260;
      } else if (*mode == 0) {
    goto L100;
      } else {
    goto L220;
      }
  L100:
      itermx = *iter;
      if (*acc >= zero) {
    iexact = 0;
      } else {
    iexact = 1;
      }
      *acc = std::abs(*acc);
      tol = ten * *acc;
      *iter = 0;
      ireset = 0;
      n1 = *n + 1;
      n2 = n1 * *n / 2;
      n3 = n2 + 1;
      s[1] = zero;
      mu[1] = zero;
      dcopy___(n, &s[1], &c__0, &s[1], &c__1);
      dcopy___(m, &mu[1], &c__0, &mu[1], &c__1);
  /*   RESET BFGS MATRIX */
  L110:
      ++ireset;
      if (ireset > 5) {
    goto L255;
      }
      l[1] = zero;
      dcopy___(&n2, &l[1], &c__0, &l[1], &c__1);
      j = 1;
      i__1 = *n;
      for (i__ = 1; i__ <= i__1; ++i__) {
    l[j] = one;
    j = j + n1 - i__;
  /* L120: */
      }
  /*   MAIN ITERATION : SEARCH DIRECTION, STEPLENGTH, LDL'-UPDATE */
  L130:
      ++(*iter);
      *mode = 9;
      if (*iter > itermx) {
    goto L330;
      }
  /*   SEARCH DIRECTION AS SOLUTION OF QP - SUBPROBLEM */
      dcopy___(n, &xl[1], &c__1, &u[1], &c__1);
      dcopy___(n, &xu[1], &c__1, &v[1], &c__1);
      d__1 = -one;
      daxpy_sl__(n, &d__1, &x[1], &c__1, &u[1], &c__1);
      d__1 = -one;
      daxpy_sl__(n, &d__1, &x[1], &c__1, &v[1], &c__1);
      h4 = one;
      lsq_(m, meq, n, &n3, la, &l[1], &g[1], &a[a_offset], &c__[1], &u[1], &v[1]
        , &s[1], &r__[1], &w[1], &iw[1], mode);
  /*   AUGMENTED PROBLEM FOR INCONSISTENT LINEARIZATION */
      if (*mode == 6) {
    if (*n == *meq) {
        *mode = 4;
    }
      }
      if (*mode == 4) {
    i__1 = *m;
    for (j = 1; j <= i__1; ++j) {
        if (j <= *meq) {
      a[j + n1 * a_dim1] = -c__[j];
        } else {
  /* Computing MAX */
      d__1 = -c__[j];
      a[j + n1 * a_dim1] = std::max(d__1,zero);
        }
  /* L140: */
    }
    s[1] = zero;
    dcopy___(n, &s[1], &c__0, &s[1], &c__1);
    h3 = zero;
    g[n1] = zero;
    l[n3] = hun;
    s[n1] = one;
    u[n1] = zero;
    v[n1] = one;
    incons = 0;
  L150:
    lsq_(m, meq, &n1, &n3, la, &l[1], &g[1], &a[a_offset], &c__[1], &u[1],
       &v[1], &s[1], &r__[1], &w[1], &iw[1], mode);
    h4 = one - s[n1];
    if (*mode == 4) {
        l[n3] = ten * l[n3];
        ++incons;
        if (incons > 5) {
      goto L330;
        }
        goto L150;
    } else if (*mode != 1) {
        goto L330;
    }
      } else if (*mode != 1) {
    goto L330;
      }
  /*   UPDATE MULTIPLIERS FOR L1-TEST */
      i__1 = *n;
      for (i__ = 1; i__ <= i__1; ++i__) {
    v[i__] = g[i__] - ddot_sl__(m, &a[i__ * a_dim1 + 1], &c__1, &r__[1], &
      c__1);
  /* L160: */
      }
      f0 = *f;
      dcopy___(n, &x[1], &c__1, &x0[1], &c__1);
      gs = ddot_sl__(n, &g[1], &c__1, &s[1], &c__1);
      h1 = std::abs(gs);
      h2 = zero;
      i__1 = *m;
      for (j = 1; j <= i__1; ++j) {
    if (j <= *meq) {
        h3 = c__[j];
    } else {
        h3 = zero;
    }
  /* Computing MAX */
    d__1 = -c__[j];
    h2 += std::max(d__1,h3);
    h3 = (d__1 = r__[j], std::abs(d__1));
  /* Computing MAX */
    d__1 = h3, d__2 = (mu[j] + h3) / two;
    mu[j] = std::max(d__1,d__2);
    h1 += h3 * (d__1 = c__[j], std::abs(d__1));
  /* L170: */
      }
  /*   CHECK CONVERGENCE */
      *mode = 0;
      if (h1 < *acc && h2 < *acc) {
    goto L330;
      }
      h1 = zero;
      i__1 = *m;
      for (j = 1; j <= i__1; ++j) {
    if (j <= *meq) {
        h3 = c__[j];
    } else {
        h3 = zero;
    }
  /* Computing MAX */
    d__1 = -c__[j];
    h1 += mu[j] * std::max(d__1,h3);
  /* L180: */
      }
      t0 = *f + h1;
      h3 = gs - h1 * h4;
      *mode = 8;
      if (h3 >= zero) {
    goto L110;
      }
  /*   LINE SEARCH WITH AN L1-TESTFUNCTION */
      line = 0;
      alpha = one;
      if (iexact == 1) {
    goto L210;
      }
  /*   INEXACT LINESEARCH */
  L190:
      ++line;
      h3 = alpha * h3;
      dscal_sl__(n, &alpha, &s[1], &c__1);
      dcopy___(n, &x0[1], &c__1, &x[1], &c__1);
      daxpy_sl__(n, &one, &s[1], &c__1, &x[1], &c__1);
      *mode = 1;
      goto L330;
  L200:
      if (h1 <= h3 / ten || line > 10) {
    goto L240;
      }
  /* Computing MAX */
      d__1 = h3 / (two * (h3 - h1));
      alpha = std::max(d__1,alfmin);
      goto L190;
  /*   EXACT LINESEARCH */
  L210:
      if (line != 3) {
    alpha = linmin_(&line, &alfmin, &one, &t, &tol);
    dcopy___(n, &x0[1], &c__1, &x[1], &c__1);
    daxpy_sl__(n, &alpha, &s[1], &c__1, &x[1], &c__1);
    *mode = 1;
    goto L330;
      }
      dscal_sl__(n, &alpha, &s[1], &c__1);
      goto L240;
  /*   CALL FUNCTIONS AT CURRENT X */
  L220:
      t = *f;
      i__1 = *m;
      for (j = 1; j <= i__1; ++j) {
    if (j <= *meq) {
        h1 = c__[j];
    } else {
        h1 = zero;
    }
  /* Computing MAX */
    d__1 = -c__[j];
    t += mu[j] * std::max(d__1,h1);
  /* L230: */
      }
      h1 = t - t0;
      switch (iexact + 1) {
    case 1:  goto L200;
    case 2:  goto L210;
      }
  /*   CHECK CONVERGENCE */
  L240:
      h3 = zero;
      i__1 = *m;
      for (j = 1; j <= i__1; ++j) {
    if (j <= *meq) {
        h1 = c__[j];
    } else {
        h1 = zero;
    }
  /* Computing MAX */
    d__1 = -c__[j];
    h3 += std::max(d__1,h1);
  /* L250: */
      }
      if (((d__1 = *f - f0, std::abs(d__1)) < *acc || dnrm2___(n, &s[1], &c__1) < *
        acc) && h3 < *acc) {
    *mode = 0;
      } else {
    *mode = -1;
      }
      goto L330;
  /*   CHECK relaxed CONVERGENCE in case of positive directional derivative */
  L255:
      if (((d__1 = *f - f0, std::abs(d__1)) < tol || dnrm2___(n, &s[1], &c__1) < tol)
         && h3 < tol) {
    *mode = 0;
      } else {
    *mode = 8;
      }
      goto L330;
  /*   CALL JACOBIAN AT CURRENT X */
  /*   UPDATE CHOLESKY-FACTORS OF HESSIAN MATRIX BY MODIFIED BFGS FORMULA */
  L260:
      i__1 = *n;
      for (i__ = 1; i__ <= i__1; ++i__) {
    u[i__] = g[i__] - ddot_sl__(m, &a[i__ * a_dim1 + 1], &c__1, &r__[1], &
      c__1) - v[i__];
  /* L270: */
      }
  /*   L'*S */
      k = 0;
      i__1 = *n;
      for (i__ = 1; i__ <= i__1; ++i__) {
    h1 = zero;
    ++k;
    i__2 = *n;
    for (j = i__ + 1; j <= i__2; ++j) {
        ++k;
        h1 += l[k] * s[j];
  /* L280: */
    }
    v[i__] = s[i__] + h1;
  /* L290: */
      }
  /*   D*L'*S */
      k = 1;
      i__1 = *n;
      for (i__ = 1; i__ <= i__1; ++i__) {
    v[i__] = l[k] * v[i__];
    k = k + n1 - i__;
  /* L300: */
      }
  /*   L*D*L'*S */
      for (i__ = *n; i__ >= 1; --i__) {
    h1 = zero;
    k = i__;
    i__1 = i__ - 1;
    for (j = 1; j <= i__1; ++j) {
        h1 += l[k] * v[j];
        k = k + *n - j;
  /* L310: */
    }
    v[i__] += h1;
  /* L320: */
      }
      h1 = ddot_sl__(n, &s[1], &c__1, &u[1], &c__1);
      h2 = ddot_sl__(n, &s[1], &c__1, &v[1], &c__1);
      h3 = h2 * .2;
      if (h1 < h3) {
    h4 = (h2 - h3) / (h2 - h1);
    h1 = h3;
    dscal_sl__(n, &h4, &u[1], &c__1);
    d__1 = one - h4;
    daxpy_sl__(n, &d__1, &v[1], &c__1, &u[1], &c__1);
      }
      d__1 = one / h1;
      ldl_(n, &l[1], &u[1], &d__1, &v[1]);
      d__1 = -one / h2;
      ldl_(n, &l[1], &v[1], &d__1, &u[1]);
  /*   END OF MAIN ITERATION */
      goto L130;
  /*   END OF SLSQPB */
  L330:
      return 0;
  } /* slsqpb_ */

  int lsq_(int *m, int *meq, int *n, int *nl,
    int *la, double *l, double *g, double *a, double *
    b, double *xl, double *xu, double *x, double *y,
    double *w, int *jw, int *mode)
  {
      /* Initialized data */

      static double zero = 0.;
      static double one = 1.;

      /* System generated locals */
      int a_dim1, a_offset, i__1, i__2;
      double d__1;

      /* Local variables */
      static int i__, i1, i2, i3, i4, m1, n1, n2, n3, ic, id, ie, if__, ig,
        ih, il, im, ip, iu, iw;
      static double diag;
      static int mineq;
      static double xnorm;

  /*   MINIMIZE with respect to X */
  /*             ||E*X - F|| */
  /*                                      1/2  T */
  /*   WITH UPPER TRIANGULAR MATRIX E = +D   *L , */
  /*                                      -1/2  -1 */
  /*                     AND VECTOR F = -D    *L  *G, */
  /*  WHERE THE UNIT LOWER TRIDIANGULAR MATRIX L IS STORED COLUMNWISE */
  /*  DENSE IN THE N*(N+1)/2 ARRAY L WITH VECTOR D STORED IN ITS */
  /* 'DIAGONAL' THUS SUBSTITUTING THE ONE-ELEMENTS OF L */
  /*   SUBJECT TO */
  /*             A(J)*X - B(J) = 0 ,         J=1,...,MEQ, */
  /*             A(J)*X - B(J) >=0,          J=MEQ+1,...,M, */
  /*             XL(I) <= X(I) <= XU(I),     I=1,...,N, */
  /*     ON ENTRY, THE USER HAS TO PROVIDE THE ARRAYS L, G, A, B, XL, XU. */
  /*     WITH DIMENSIONS: L(N*(N+1)/2), G(N), A(LA,N), B(M), XL(N), XU(N) */
  /*     THE WORKING ARRAY W MUST HAVE AT LEAST THE FOLLOWING DIMENSION: */
  /*     DIM(W) =        (3*N+M)*(N+1)                        for LSQ */
  /*                    +(N-MEQ+1)*(MINEQ+2) + 2*MINEQ        for LSI */
  /*                    +(N+MINEQ)*(N-MEQ) + 2*MEQ + N        for LSEI */
  /*                      with MINEQ = M - MEQ + 2*N */
  /*     ON RETURN, NO ARRAY WILL BE CHANGED BY THE SUBROUTINE. */
  /*     X     STORES THE N-DIMENSIONAL SOLUTION VECTOR */
  /*     Y     STORES THE VECTOR OF LAGRANGE MULTIPLIERS OF DIMENSION */
  /*           M+N+N (CONSTRAINTS+LOWER+UPPER BOUNDS) */
  /*     MODE  IS A SUCCESS-FAILURE FLAG WITH THE FOLLOWING MEANINGS: */
  /*          MODE=1: SUCCESSFUL COMPUTATION */
  /*               2: ERROR RETURN BECAUSE OF WRONG DIMENSIONS (N<1) */
  /*               3: ITERATION COUNT EXCEEDED BY NNLS */
  /*               4: INEQUALITY CONSTRAINTS INCOMPATIBLE */
  /*               5: MATRIX E IS NOT OF FULL RANK */
  /*               6: MATRIX C IS NOT OF FULL RANK */
  /*               7: RANK DEFECT IN HFTI */
  /*     coded            Dieter Kraft, april 1987 */
  /*     revised                        march 1989 */
      /* Parameter adjustments */
      --y;
      --x;
      --xu;
      --xl;
      --g;
      --l;
      --b;
      a_dim1 = *la;
      a_offset = 1 + a_dim1;
      a -= a_offset;
      --w;
      --jw;

      /* Function Body */
      n1 = *n + 1;
      mineq = *m - *meq;
      m1 = mineq + *n + *n;
  /*  determine whether to solve problem */
  /*  with inconsistent linerarization (n2=1) */
  /*  or not (n2=0) */
      n2 = n1 * *n / 2 + 1;
      if (n2 == *nl) {
    n2 = 0;
      } else {
    n2 = 1;
      }
      n3 = *n - n2;
  /*  RECOVER MATRIX E AND VECTOR F FROM L AND G */
      i2 = 1;
      i3 = 1;
      i4 = 1;
      ie = 1;
      if__ = *n * *n + 1;
      i__1 = n3;
      for (i__ = 1; i__ <= i__1; ++i__) {
    i1 = n1 - i__;
    diag = sqrt(l[i2]);
    w[i3] = zero;
    dcopy___(&i1, &w[i3], &c__0, &w[i3], &c__1);
    i__2 = i1 - n2;
    dcopy___(&i__2, &l[i2], &c__1, &w[i3], n);
    i__2 = i1 - n2;
    dscal_sl__(&i__2, &diag, &w[i3], n);
    w[i3] = diag;
    i__2 = i__ - 1;
    w[if__ - 1 + i__] = (g[i__] - ddot_sl__(&i__2, &w[i4], &c__1, &w[if__]
      , &c__1)) / diag;
    i2 = i2 + i1 - n2;
    i3 += n1;
    i4 += *n;
  /* L10: */
      }
      if (n2 == 1) {
    w[i3] = l[*nl];
    w[i4] = zero;
    dcopy___(&n3, &w[i4], &c__0, &w[i4], &c__1);
    w[if__ - 1 + *n] = zero;
      }
      d__1 = -one;
      dscal_sl__(n, &d__1, &w[if__], &c__1);
      ic = if__ + *n;
      id = ic + *meq * *n;
      if (*meq > 0) {
  /*  RECOVER MATRIX C FROM UPPER PART OF A */
    i__1 = *meq;
    for (i__ = 1; i__ <= i__1; ++i__) {
        dcopy___(n, &a[i__ + a_dim1], la, &w[ic - 1 + i__], meq);
  /* L20: */
    }
  /*  RECOVER VECTOR D FROM UPPER PART OF B */
    dcopy___(meq, &b[1], &c__1, &w[id], &c__1);
    d__1 = -one;
    dscal_sl__(meq, &d__1, &w[id], &c__1);
      }
      ig = id + *meq;
      if (mineq > 0) {
  /*  RECOVER MATRIX G FROM LOWER PART OF A */
    i__1 = mineq;
    for (i__ = 1; i__ <= i__1; ++i__) {
        dcopy___(n, &a[*meq + i__ + a_dim1], la, &w[ig - 1 + i__], &m1);
  /* L30: */
    }
      }
  /*  AUGMENT MATRIX G BY +I AND -I */
      ip = ig + mineq;
      i__1 = *n;
      for (i__ = 1; i__ <= i__1; ++i__) {
    w[ip - 1 + i__] = zero;
    dcopy___(n, &w[ip - 1 + i__], &c__0, &w[ip - 1 + i__], &m1);
  /* L40: */
      }
      w[ip] = one;
      i__1 = m1 + 1;
      dcopy___(n, &w[ip], &c__0, &w[ip], &i__1);
      im = ip + *n;
      i__1 = *n;
      for (i__ = 1; i__ <= i__1; ++i__) {
    w[im - 1 + i__] = zero;
    dcopy___(n, &w[im - 1 + i__], &c__0, &w[im - 1 + i__], &m1);
  /* L50: */
      }
      w[im] = -one;
      i__1 = m1 + 1;
      dcopy___(n, &w[im], &c__0, &w[im], &i__1);
      ih = ig + m1 * *n;
      if (mineq > 0) {
  /*  RECOVER H FROM LOWER PART OF B */
    dcopy___(&mineq, &b[*meq + 1], &c__1, &w[ih], &c__1);
    d__1 = -one;
    dscal_sl__(&mineq, &d__1, &w[ih], &c__1);
      }
  /*  AUGMENT VECTOR H BY XL AND XU */
      il = ih + mineq;
      dcopy___(n, &xl[1], &c__1, &w[il], &c__1);
      iu = il + *n;
      dcopy___(n, &xu[1], &c__1, &w[iu], &c__1);
      d__1 = -one;
      dscal_sl__(n, &d__1, &w[iu], &c__1);
      iw = iu + *n;
      i__1 = std::max(1,*meq);
      lsei_(&w[ic], &w[id], &w[ie], &w[if__], &w[ig], &w[ih], &i__1, meq, n, n,
        &m1, &m1, n, &x[1], &xnorm, &w[iw], &jw[1], mode);
      if (*mode == 1) {
  /*   restore Lagrange multipliers */
    dcopy___(m, &w[iw], &c__1, &y[1], &c__1);
    dcopy___(&n3, &w[iw + *m], &c__1, &y[*m + 1], &c__1);
    dcopy___(&n3, &w[iw + *m + *n], &c__1, &y[*m + n3 + 1], &c__1);
      }
  /*   END OF SUBROUTINE LSQ */
      return 0;
  } /* lsq_ */

  int lsei_(double *c__, double *d__, double *e,
    double *f, double *g, double *h__, int *lc, int *
    mc, int *le, int *me, int *lg, int *mg, int *n,
    double *x, double *xnrm, double *w, int *jw, int *
    mode)
  {
      /* Initialized data */

      static double epmach = 2.22e-16;
      static double zero = 0.;

      /* System generated locals */
      int c_dim1, c_offset, e_dim1, e_offset, g_dim1, g_offset, i__1, i__2,
        i__3;
      double d__1;

      /* Local variables */
      static int i__, j, k, l;
      static double t;
      static int ie, if__, ig, iw, mc1;
      static int krank;

  /*     FOR MODE=1, THE SUBROUTINE RETURNS THE SOLUTION X OF */
  /*     EQUALITY & INEQUALITY CONSTRAINED LEAST SQUARES PROBLEM LSEI : */
  /*                MIN ||E*X - F|| */
  /*                 X */
  /*                S.T.  C*X  = D, */
  /*                      G*X >= H. */
  /*     USING QR DECOMPOSITION & ORTHOGONAL BASIS OF NULLSPACE OF C */
  /*     CHAPTER 23.6 OF LAWSON & HANSON: SOLVING LEAST SQUARES PROBLEMS. */
  /*     THE FOLLOWING DIMENSIONS OF THE ARRAYS DEFINING THE PROBLEM */
  /*     ARE NECESSARY */
  /*     DIM(E) :   FORMAL (LE,N),    ACTUAL (ME,N) */
  /*     DIM(F) :   FORMAL (LE  ),    ACTUAL (ME  ) */
  /*     DIM(C) :   FORMAL (LC,N),    ACTUAL (MC,N) */
  /*     DIM(D) :   FORMAL (LC  ),    ACTUAL (MC  ) */
  /*     DIM(G) :   FORMAL (LG,N),    ACTUAL (MG,N) */
  /*     DIM(H) :   FORMAL (LG  ),    ACTUAL (MG  ) */
  /*     DIM(X) :   FORMAL (N   ),    ACTUAL (N   ) */
  /*     DIM(W) :   2*MC+ME+(ME+MG)*(N-MC)  for LSEI */
  /*              +(N-MC+1)*(MG+2)+2*MG     for LSI */
  /*     DIM(JW):   MAX(MG,L) */
  /*     ON ENTRY, THE USER HAS TO PROVIDE THE ARRAYS C, D, E, F, G, AND H. */
  /*     ON RETURN, ALL ARRAYS WILL BE CHANGED BY THE SUBROUTINE. */
  /*     X     STORES THE SOLUTION VECTOR */
  /*     XNORM STORES THE RESIDUUM OF THE SOLUTION IN EUCLIDIAN NORM */
  /*     W     STORES THE VECTOR OF LAGRANGE MULTIPLIERS IN ITS FIRST */
  /*           MC+MG ELEMENTS */
  /*     MODE  IS A SUCCESS-FAILURE FLAG WITH THE FOLLOWING MEANINGS: */
  /*          MODE=1: SUCCESSFUL COMPUTATION */
  /*               2: ERROR RETURN BECAUSE OF WRONG DIMENSIONS (N<1) */
  /*               3: ITERATION COUNT EXCEEDED BY NNLS */
  /*               4: INEQUALITY CONSTRAINTS INCOMPATIBLE */
  /*               5: MATRIX E IS NOT OF FULL RANK */
  /*               6: MATRIX C IS NOT OF FULL RANK */
  /*               7: RANK DEFECT IN HFTI */
  /*     18.5.1981, DIETER KRAFT, DFVLR OBERPFAFFENHOFEN */
  /*     20.3.1987, DIETER KRAFT, DFVLR OBERPFAFFENHOFEN */
      /* Parameter adjustments */
      --d__;
      --f;
      --h__;
      --x;
      g_dim1 = *lg;
      g_offset = 1 + g_dim1;
      g -= g_offset;
      e_dim1 = *le;
      e_offset = 1 + e_dim1;
      e -= e_offset;
      c_dim1 = *lc;
      c_offset = 1 + c_dim1;
      c__ -= c_offset;
      --w;
      --jw;

      /* Function Body */
      *mode = 2;
      if (*mc > *n) {
    goto L75;
      }
      l = *n - *mc;
      mc1 = *mc + 1;
      iw = (l + 1) * (*mg + 2) + (*mg << 1) + *mc;
      ie = iw + *mc + 1;
      if__ = ie + *me * l;
      ig = if__ + *me;
  /*  TRIANGULARIZE C AND APPLY FACTORS TO E AND G */
      i__1 = *mc;
      for (i__ = 1; i__ <= i__1; ++i__) {
  /* Computing MIN */
    i__2 = i__ + 1;
    j = std::min(i__2,*lc);
    i__2 = i__ + 1;
    i__3 = *mc - i__;
    h12_(&c__1, &i__, &i__2, n, &c__[i__ + c_dim1], lc, &w[iw + i__], &
      c__[j + c_dim1], lc, &c__1, &i__3);
    i__2 = i__ + 1;
    h12_(&c__2, &i__, &i__2, n, &c__[i__ + c_dim1], lc, &w[iw + i__], &e[
      e_offset], le, &c__1, me);
  /* L10: */
    i__2 = i__ + 1;
    h12_(&c__2, &i__, &i__2, n, &c__[i__ + c_dim1], lc, &w[iw + i__], &g[
      g_offset], lg, &c__1, mg);
      }
  /*  SOLVE C*X=D AND MODIFY F */
      *mode = 6;
      i__2 = *mc;
      for (i__ = 1; i__ <= i__2; ++i__) {
    if ((d__1 = c__[i__ + i__ * c_dim1], std::abs(d__1)) < epmach) {
        goto L75;
    }
    i__1 = i__ - 1;
    x[i__] = (d__[i__] - ddot_sl__(&i__1, &c__[i__ + c_dim1], lc, &x[1], &
      c__1)) / c__[i__ + i__ * c_dim1];
  /* L15: */
      }
      *mode = 1;
      w[mc1] = zero;
      i__2 = *mg - *mc;
      dcopy___(&i__2, &w[mc1], &c__0, &w[mc1], &c__1);
      if (*mc == *n) {
    goto L50;
      }
      i__2 = *me;
      for (i__ = 1; i__ <= i__2; ++i__) {
  /* L20: */
    w[if__ - 1 + i__] = f[i__] - ddot_sl__(mc, &e[i__ + e_dim1], le, &x[1]
      , &c__1);
      }
  /*  STORE TRANSFORMED E & G */
      i__2 = *me;
      for (i__ = 1; i__ <= i__2; ++i__) {
  /* L25: */
    dcopy___(&l, &e[i__ + mc1 * e_dim1], le, &w[ie - 1 + i__], me);
      }
      i__2 = *mg;
      for (i__ = 1; i__ <= i__2; ++i__) {
  /* L30: */
    dcopy___(&l, &g[i__ + mc1 * g_dim1], lg, &w[ig - 1 + i__], mg);
      }
      if (*mg > 0) {
    goto L40;
      }
  /*  SOLVE LS WITHOUT INEQUALITY CONSTRAINTS */
      *mode = 7;
      k = std::max(*le,*n);
      t = sqrt(epmach);
      hfti_(&w[ie], me, me, &l, &w[if__], &k, &c__1, &t, &krank, xnrm, &w[1], &
        w[l + 1], &jw[1]);
      dcopy___(&l, &w[if__], &c__1, &x[mc1], &c__1);
      if (krank != l) {
    goto L75;
      }
      *mode = 1;
      goto L50;
  /*  MODIFY H AND SOLVE INEQUALITY CONSTRAINED LS PROBLEM */
  L40:
      i__2 = *mg;
      for (i__ = 1; i__ <= i__2; ++i__) {
  /* L45: */
    h__[i__] -= ddot_sl__(mc, &g[i__ + g_dim1], lg, &x[1], &c__1);
      }
      lsi_(&w[ie], &w[if__], &w[ig], &h__[1], me, me, mg, mg, &l, &x[mc1], xnrm,
         &w[mc1], &jw[1], mode);
      if (*mc == 0) {
    goto L75;
      }
      t = dnrm2___(mc, &x[1], &c__1);
      *xnrm = sqrt(*xnrm * *xnrm + t * t);
      if (*mode != 1) {
    goto L75;
      }
  /*  SOLUTION OF ORIGINAL PROBLEM AND LAGRANGE MULTIPLIERS */
  L50:
      i__2 = *me;
      for (i__ = 1; i__ <= i__2; ++i__) {
  /* L55: */
    f[i__] = ddot_sl__(n, &e[i__ + e_dim1], le, &x[1], &c__1) - f[i__];
      }
      i__2 = *mc;
      for (i__ = 1; i__ <= i__2; ++i__) {
  /* L60: */
    d__[i__] = ddot_sl__(me, &e[i__ * e_dim1 + 1], &c__1, &f[1], &c__1) -
      ddot_sl__(mg, &g[i__ * g_dim1 + 1], &c__1, &w[mc1], &c__1);
      }
      for (i__ = *mc; i__ >= 1; --i__) {
  /* L65: */
    i__2 = i__ + 1;
    h12_(&c__2, &i__, &i__2, n, &c__[i__ + c_dim1], lc, &w[iw + i__], &x[
      1], &c__1, &c__1, &c__1);
      }
      for (i__ = *mc; i__ >= 1; --i__) {
  /* Computing MIN */
    i__2 = i__ + 1;
    j = std::min(i__2,*lc);
    i__2 = *mc - i__;
    w[i__] = (d__[i__] - ddot_sl__(&i__2, &c__[j + i__ * c_dim1], &c__1, &
      w[j], &c__1)) / c__[i__ + i__ * c_dim1];
  /* L70: */
      }
  /*  END OF SUBROUTINE LSEI */
  L75:
      return 0;
  } /* lsei_ */

  int lsi_(double *e, double *f, double *g,
           double *h__, int *le, int *me, int *lg, int *mg,
           int *n, double *x, double *xnorm, double *w, int *
           jw, int *mode)
  {
      /* Initialized data */

      static double epmach = 2.22e-16;
      static double one = 1.;

      /* System generated locals */
      int e_dim1, e_offset, g_dim1, g_offset, i__1, i__2, i__3;
      double d__1;

      /* Local variables */
      static int i__, j;
      static double t;

  /*     FOR MODE=1, THE SUBROUTINE RETURNS THE SOLUTION X OF */
  /*     INEQUALITY CONSTRAINED LINEAR LEAST SQUARES PROBLEM: */
  /*                    MIN ||E*X-F|| */
  /*                     X */
  /*                    S.T.  G*X >= H */
  /*     THE ALGORITHM IS BASED ON QR DECOMPOSITION AS DESCRIBED IN */
  /*     CHAPTER 23.5 OF LAWSON & HANSON: SOLVING LEAST SQUARES PROBLEMS */
  /*     THE FOLLOWING DIMENSIONS OF THE ARRAYS DEFINING THE PROBLEM */
  /*     ARE NECESSARY */
  /*     DIM(E) :   FORMAL (LE,N),    ACTUAL (ME,N) */
  /*     DIM(F) :   FORMAL (LE  ),    ACTUAL (ME  ) */
  /*     DIM(G) :   FORMAL (LG,N),    ACTUAL (MG,N) */
  /*     DIM(H) :   FORMAL (LG  ),    ACTUAL (MG  ) */
  /*     DIM(X) :   N */
  /*     DIM(W) :   (N+1)*(MG+2) + 2*MG */
  /*     DIM(JW):   LG */
  /*     ON ENTRY, THE USER HAS TO PROVIDE THE ARRAYS E, F, G, AND H. */
  /*     ON RETURN, ALL ARRAYS WILL BE CHANGED BY THE SUBROUTINE. */
  /*     X     STORES THE SOLUTION VECTOR */
  /*     XNORM STORES THE RESIDUUM OF THE SOLUTION IN EUCLIDIAN NORM */
  /*     W     STORES THE VECTOR OF LAGRANGE MULTIPLIERS IN ITS FIRST */
  /*           MG ELEMENTS */
  /*     MODE  IS A SUCCESS-FAILURE FLAG WITH THE FOLLOWING MEANINGS: */
  /*          MODE=1: SUCCESSFUL COMPUTATION */
  /*               2: ERROR RETURN BECAUSE OF WRONG DIMENSIONS (N<1) */
  /*               3: ITERATION COUNT EXCEEDED BY NNLS */
  /*               4: INEQUALITY CONSTRAINTS INCOMPATIBLE */
  /*               5: MATRIX E IS NOT OF FULL RANK */
  /*     03.01.1980, DIETER KRAFT: CODED */
  /*     20.03.1987, DIETER KRAFT: REVISED TO FORTRAN 77 */
      /* Parameter adjustments */
      --f;
      --jw;
      --h__;
      --x;
      g_dim1 = *lg;
      g_offset = 1 + g_dim1;
      g -= g_offset;
      e_dim1 = *le;
      e_offset = 1 + e_dim1;
      e -= e_offset;
      --w;

      /* Function Body */
  /*  QR-FACTORS OF E AND APPLICATION TO F */
      i__1 = *n;
      for (i__ = 1; i__ <= i__1; ++i__) {
  /* Computing MIN */
    i__2 = i__ + 1;
    j = std::min(i__2,*n);
    i__2 = i__ + 1;
    i__3 = *n - i__;
    h12_(&c__1, &i__, &i__2, me, &e[i__ * e_dim1 + 1], &c__1, &t, &e[j *
      e_dim1 + 1], &c__1, le, &i__3);
  /* L10: */
    i__2 = i__ + 1;
    h12_(&c__2, &i__, &i__2, me, &e[i__ * e_dim1 + 1], &c__1, &t, &f[1], &
      c__1, &c__1, &c__1);
      }
  /*  TRANSFORM G AND H TO GET LEAST DISTANCE PROBLEM */
      *mode = 5;
      i__2 = *mg;
      for (i__ = 1; i__ <= i__2; ++i__) {
    i__1 = *n;
    for (j = 1; j <= i__1; ++j) {
        if ((d__1 = e[j + j * e_dim1], std::abs(d__1)) < epmach) {
      goto L50;
        }
  /* L20: */
        i__3 = j - 1;
        g[i__ + j * g_dim1] = (g[i__ + j * g_dim1] - ddot_sl__(&i__3, &g[
          i__ + g_dim1], lg, &e[j * e_dim1 + 1], &c__1)) / e[j + j *
           e_dim1];
    }
  /* L30: */
    h__[i__] -= ddot_sl__(n, &g[i__ + g_dim1], lg, &f[1], &c__1);
      }
  /*  SOLVE LEAST DISTANCE PROBLEM */
      ldp_(&g[g_offset], lg, mg, n, &h__[1], &x[1], xnorm, &w[1], &jw[1], mode);
      if (*mode != 1) {
    goto L50;
      }
  /*  SOLUTION OF ORIGINAL PROBLEM */
      daxpy_sl__(n, &one, &f[1], &c__1, &x[1], &c__1);
      for (i__ = *n; i__ >= 1; --i__) {
  /* Computing MIN */
    i__2 = i__ + 1;
    j = std::min(i__2,*n);
  /* L40: */
    i__2 = *n - i__;
    x[i__] = (x[i__] - ddot_sl__(&i__2, &e[i__ + j * e_dim1], le, &x[j], &
      c__1)) / e[i__ + i__ * e_dim1];
      }
  /* Computing MIN */
      i__2 = *n + 1;
      j = std::min(i__2,*me);
      i__2 = *me - *n;
      t = dnrm2___(&i__2, &f[j], &c__1);
      *xnorm = sqrt(*xnorm * *xnorm + t * t);
  /*  END OF SUBROUTINE LSI */
  L50:
      return 0;
  } /* lsi_ */

  int ldp_(double *g, int *mg, int *m, int *n,
    double *h__, double *x, double *xnorm, double *w,
    int *index, int *mode)
  {
      /* Initialized data */

      static double zero = 0.;
      static double one = 1.;

      /* System generated locals */
      int g_dim1, g_offset, i__1, i__2;
      double d__1;

      /* Local variables */
      static int i__, j, n1, if__, iw, iy, iz;
      static double fac;
      static double rnorm;
      static int iwdual;

  /*                     T */
  /*     MINIMIZE   1/2 X X    SUBJECT TO   G * X >= H. */
  /*       C.L. LAWSON, R.J. HANSON: 'SOLVING LEAST SQUARES PROBLEMS' */
  /*       PRENTICE HALL, ENGLEWOOD CLIFFS, NEW JERSEY, 1974. */
  /*     PARAMETER DESCRIPTION: */
  /*     G(),MG,M,N   ON ENTRY G() STORES THE M BY N MATRIX OF */
  /*                  LINEAR INEQUALITY CONSTRAINTS. G() HAS FIRST */
  /*                  DIMENSIONING PARAMETER MG */
  /*     H()          ON ENTRY H() STORES THE M VECTOR H REPRESENTING */
  /*                  THE RIGHT SIDE OF THE INEQUALITY SYSTEM */
  /*     REMARK: G(),H() WILL NOT BE CHANGED DURING CALCULATIONS BY LDP */
  /*     X()          ON ENTRY X() NEED NOT BE INITIALIZED. */
  /*                  ON EXIT X() STORES THE SOLUTION VECTOR X IF MODE=1. */
  /*     XNORM        ON EXIT XNORM STORES THE EUCLIDIAN NORM OF THE */
  /*                  SOLUTION VECTOR IF COMPUTATION IS SUCCESSFUL */
  /*     W()          W IS A ONE DIMENSIONAL WORKING SPACE, THE LENGTH */
  /*                  OF WHICH SHOULD BE AT LEAST (M+2)*(N+1) + 2*M */
  /*                  ON EXIT W() STORES THE LAGRANGE MULTIPLIERS */
  /*                  ASSOCIATED WITH THE CONSTRAINTS */
  /*                  AT THE SOLUTION OF PROBLEM LDP */
  /*     INDEX()      INDEX() IS A ONE DIMENSIONAL INT WORKING SPACE */
  /*                  OF LENGTH AT LEAST M */
  /*     MODE         MODE IS A SUCCESS-FAILURE FLAG WITH THE FOLLOWING */
  /*                  MEANINGS: */
  /*          MODE=1: SUCCESSFUL COMPUTATION */
  /*               2: ERROR RETURN BECAUSE OF WRONG DIMENSIONS (N.LE.0) */
  /*               3: ITERATION COUNT EXCEEDED BY NNLS */
  /*               4: INEQUALITY CONSTRAINTS INCOMPATIBLE */
      /* Parameter adjustments */
      --index;
      --h__;
      --x;
      g_dim1 = *mg;
      g_offset = 1 + g_dim1;
      g -= g_offset;
      --w;

      /* Function Body */
      *mode = 2;
      if (*n <= 0) {
    goto L50;
      }
  /*  STATE DUAL PROBLEM */
      *mode = 1;
      x[1] = zero;
      dcopy___(n, &x[1], &c__0, &x[1], &c__1);
      *xnorm = zero;
      if (*m == 0) {
    goto L50;
      }
      iw = 0;
      i__1 = *m;
      for (j = 1; j <= i__1; ++j) {
    i__2 = *n;
    for (i__ = 1; i__ <= i__2; ++i__) {
        ++iw;
  /* L10: */
        w[iw] = g[j + i__ * g_dim1];
    }
    ++iw;
  /* L20: */
    w[iw] = h__[j];
      }
      if__ = iw + 1;
      i__1 = *n;
      for (i__ = 1; i__ <= i__1; ++i__) {
    ++iw;
  /* L30: */
    w[iw] = zero;
      }
      w[iw + 1] = one;
      n1 = *n + 1;
      iz = iw + 2;
      iy = iz + n1;
      iwdual = iy + *m;
  /*  SOLVE DUAL PROBLEM */
      nnls_(&w[1], &n1, &n1, m, &w[if__], &w[iy], &rnorm, &w[iwdual], &w[iz], &
        index[1], mode);
      if (*mode != 1) {
    goto L50;
      }
      *mode = 4;
      if (rnorm <= zero) {
    goto L50;
      }
  /*  COMPUTE SOLUTION OF PRIMAL PROBLEM */
      fac = one - ddot_sl__(m, &h__[1], &c__1, &w[iy], &c__1);
      d__1 = one + fac;
      if (d__1 - one <= zero) {
    goto L50;
      }
      *mode = 1;
      fac = one / fac;
      i__1 = *n;
      for (j = 1; j <= i__1; ++j) {
  /* L40: */
    x[j] = fac * ddot_sl__(m, &g[j * g_dim1 + 1], &c__1, &w[iy], &c__1);
      }
      *xnorm = dnrm2___(n, &x[1], &c__1);
  /*  COMPUTE LAGRANGE MULTIPLIERS FOR PRIMAL PROBLEM */
      w[1] = zero;
      dcopy___(m, &w[1], &c__0, &w[1], &c__1);
      daxpy_sl__(m, &fac, &w[iy], &c__1, &w[1], &c__1);
  /*  END OF SUBROUTINE LDP */
  L50:
      return 0;
  } /* ldp_ */

  int nnls_(double *a, int *mda, int *m, int *
            n, double *b, double *x, double *rnorm, double *w,
            double *z__, int *index, int *mode)
  {
      /* Initialized data */

      static double zero = 0.;
      static double one = 1.;
      static double factor = .01;

      /* System generated locals */
      int a_dim1, a_offset, i__1, i__2;
      double d__1;

      /* Local variables */
      static double c__;
      static int i__, j, k, l;
      static double s, t;
      static int ii, jj, ip, iz, jz;
      static double up;
      static int iz1, iz2, npp1, iter;
      static double wmax, alpha, asave;
      static int itmax, izmax, nsetp;
      static double unorm;

  /*     C.L.LAWSON AND R.J.HANSON, JET PROPULSION LABORATORY: */
  /*     'SOLVING LEAST SQUARES PROBLEMS'. PRENTICE-HALL.1974 */
  /*      **********   NONNEGATIVE LEAST SQUARES   ********** */
  /*     GIVEN AN M BY N MATRIX, A, AND AN M-VECTOR, B, COMPUTE AN */
  /*     N-VECTOR, X, WHICH SOLVES THE LEAST SQUARES PROBLEM */
  /*                  A*X = B  SUBJECT TO  X >= 0 */
  /*     A(),MDA,M,N */
  /*            MDA IS THE FIRST DIMENSIONING PARAMETER FOR THE ARRAY,A(). */
  /*            ON ENTRY A()  CONTAINS THE M BY N MATRIX,A. */
  /*            ON EXIT A() CONTAINS THE PRODUCT Q*A, */
  /*            WHERE Q IS AN M BY M ORTHOGONAL MATRIX GENERATED */
  /*            IMPLICITLY BY THIS SUBROUTINE. */
  /*            EITHER M>=N OR M<N IS PERMISSIBLE. */
  /*            THERE IS NO RESTRICTION ON THE RANK OF A. */
  /*     B()    ON ENTRY B() CONTAINS THE M-VECTOR, B. */
  /*            ON EXIT B() CONTAINS Q*B. */
  /*     X()    ON ENTRY X() NEED NOT BE INITIALIZED. */
  /*            ON EXIT X() WILL CONTAIN THE SOLUTION VECTOR. */
  /*     RNORM  ON EXIT RNORM CONTAINS THE EUCLIDEAN NORM OF THE */
  /*            RESIDUAL VECTOR. */
  /*     W()    AN N-ARRAY OF WORKING SPACE. */
  /*            ON EXIT W() WILL CONTAIN THE DUAL SOLUTION VECTOR. */
  /*            W WILL SATISFY W(I)=0 FOR ALL I IN SET P */
  /*            AND W(I)<=0 FOR ALL I IN SET Z */
  /*     Z()    AN M-ARRAY OF WORKING SPACE. */
  /*     INDEX()AN INT WORKING ARRAY OF LENGTH AT LEAST N. */
  /*            ON EXIT THE CONTENTS OF THIS ARRAY DEFINE THE SETS */
  /*            P AND Z AS FOLLOWS: */
  /*            INDEX(1)    THRU INDEX(NSETP) = SET P. */
  /*            INDEX(IZ1)  THRU INDEX (IZ2)  = SET Z. */
  /*            IZ1=NSETP + 1 = NPP1, IZ2=N. */
  /*     MODE   THIS IS A SUCCESS-FAILURE FLAG WITH THE FOLLOWING MEANING: */
  /*            1    THE SOLUTION HAS BEEN COMPUTED SUCCESSFULLY. */
  /*            2    THE DIMENSIONS OF THE PROBLEM ARE WRONG, */
  /*                 EITHER M <= 0 OR N <= 0. */
  /*            3    ITERATION COUNT EXCEEDED, MORE THAN 3*N ITERATIONS. */
      /* Parameter adjustments */
      --z__;
      --b;
      --index;
      --w;
      --x;
      a_dim1 = *mda;
      a_offset = 1 + a_dim1;
      a -= a_offset;

      /* Function Body */
  /*     revised          Dieter Kraft, March 1983 */
      *mode = 2;
      if (*m <= 0 || *n <= 0) {
    goto L290;
      }
      *mode = 1;
      iter = 0;
      itmax = *n * 3;
  /* STEP ONE (INITIALIZE) */
      i__1 = *n;
      for (i__ = 1; i__ <= i__1; ++i__) {
  /* L100: */
    index[i__] = i__;
      }
      iz1 = 1;
      iz2 = *n;
      nsetp = 0;
      npp1 = 1;
      x[1] = zero;
      dcopy___(n, &x[1], &c__0, &x[1], &c__1);
  /* STEP TWO (COMPUTE DUAL VARIABLES) */
  /* .....ENTRY LOOP A */
  L110:
      if (iz1 > iz2 || nsetp >= *m) {
    goto L280;
      }
      i__1 = iz2;
      for (iz = iz1; iz <= i__1; ++iz) {
    j = index[iz];
  /* L120: */
    i__2 = *m - nsetp;
    w[j] = ddot_sl__(&i__2, &a[npp1 + j * a_dim1], &c__1, &b[npp1], &c__1)
      ;
      }
  /* STEP THREE (TEST DUAL VARIABLES) */
  L130:
      wmax = zero;
      i__2 = iz2;
      for (iz = iz1; iz <= i__2; ++iz) {
    j = index[iz];
    if (w[j] <= wmax) {
        goto L140;
    }
    wmax = w[j];
    izmax = iz;
  L140:
    ;
      }
  /* .....EXIT LOOP A */
      if (wmax <= zero) {
    goto L280;
      }
      iz = izmax;
      j = index[iz];
  /* STEP FOUR (TEST INDEX J FOR LINEAR DEPENDENCY) */
      asave = a[npp1 + j * a_dim1];
      i__2 = npp1 + 1;
      h12_(&c__1, &npp1, &i__2, m, &a[j * a_dim1 + 1], &c__1, &up, &z__[1], &
        c__1, &c__1, &c__0);
      unorm = dnrm2___(&nsetp, &a[j * a_dim1 + 1], &c__1);
      t = factor * (d__1 = a[npp1 + j * a_dim1], std::abs(d__1));
      d__1 = unorm + t;
      if (d__1 - unorm <= zero) {
    goto L150;
      }
      dcopy___(m, &b[1], &c__1, &z__[1], &c__1);
      i__2 = npp1 + 1;
      h12_(&c__2, &npp1, &i__2, m, &a[j * a_dim1 + 1], &c__1, &up, &z__[1], &
        c__1, &c__1, &c__1);
      if (z__[npp1] / a[npp1 + j * a_dim1] > zero) {
    goto L160;
      }
  L150:
      a[npp1 + j * a_dim1] = asave;
      w[j] = zero;
      goto L130;
  /* STEP FIVE (ADD COLUMN) */
  L160:
      dcopy___(m, &z__[1], &c__1, &b[1], &c__1);
      index[iz] = index[iz1];
      index[iz1] = j;
      ++iz1;
      nsetp = npp1;
      ++npp1;
      i__2 = iz2;
      for (jz = iz1; jz <= i__2; ++jz) {
    jj = index[jz];
  /* L170: */
    h12_(&c__2, &nsetp, &npp1, m, &a[j * a_dim1 + 1], &c__1, &up, &a[jj *
      a_dim1 + 1], &c__1, mda, &c__1);
      }
      k = std::min(npp1,*mda);
      w[j] = zero;
      i__2 = *m - nsetp;
      dcopy___(&i__2, &w[j], &c__0, &a[k + j * a_dim1], &c__1);
  /* STEP SIX (SOLVE LEAST SQUARES SUB-PROBLEM) */
  /* .....ENTRY LOOP B */
  L180:
      for (ip = nsetp; ip >= 1; --ip) {
    if (ip == nsetp) {
        goto L190;
    }
    d__1 = -z__[ip + 1];
    daxpy_sl__(&ip, &d__1, &a[jj * a_dim1 + 1], &c__1, &z__[1], &c__1);
  L190:
    jj = index[ip];
  /* L200: */
    z__[ip] /= a[ip + jj * a_dim1];
      }
      ++iter;
      if (iter <= itmax) {
    goto L220;
      }
  L210:
      *mode = 3;
      goto L280;
  /* STEP SEVEN TO TEN (STEP LENGTH ALGORITHM) */
  L220:
      alpha = one;
      jj = 0;
      i__2 = nsetp;
      for (ip = 1; ip <= i__2; ++ip) {
    if (z__[ip] > zero) {
        goto L230;
    }
    l = index[ip];
    t = -x[l] / (z__[ip] - x[l]);
    if (alpha < t) {
        goto L230;
    }
    alpha = t;
    jj = ip;
  L230:
    ;
      }
      i__2 = nsetp;
      for (ip = 1; ip <= i__2; ++ip) {
    l = index[ip];
  /* L240: */
    x[l] = (one - alpha) * x[l] + alpha * z__[ip];
      }
  /* .....EXIT LOOP B */
      if (jj == 0) {
    goto L110;
      }
  /* STEP ELEVEN (DELETE COLUMN) */
      i__ = index[jj];
  L250:
      x[i__] = zero;
      ++jj;
      i__2 = nsetp;
      for (j = jj; j <= i__2; ++j) {
    ii = index[j];
    index[j - 1] = ii;
    dsrotg_(&a[j - 1 + ii * a_dim1], &a[j + ii * a_dim1], &c__, &s);
    t = a[j - 1 + ii * a_dim1];
    dsrot_(n, &a[j - 1 + a_dim1], mda, &a[j + a_dim1], mda, &c__, &s);
    a[j - 1 + ii * a_dim1] = t;
    a[j + ii * a_dim1] = zero;
  /* L260: */
    dsrot_(&c__1, &b[j - 1], &c__1, &b[j], &c__1, &c__, &s);
      }
      npp1 = nsetp;
      --nsetp;
      --iz1;
      index[iz1] = i__;
      if (nsetp <= 0) {
    goto L210;
      }
      i__2 = nsetp;
      for (jj = 1; jj <= i__2; ++jj) {
    i__ = index[jj];
    if (x[i__] <= zero) {
        goto L250;
    }
  /* L270: */
      }
      dcopy___(m, &b[1], &c__1, &z__[1], &c__1);
      goto L180;
  /* STEP TWELVE (SOLUTION) */
  L280:
      k = std::min(npp1,*m);
      i__2 = *m - nsetp;
      *rnorm = dnrm2___(&i__2, &b[k], &c__1);
      if (npp1 > *m) {
    w[1] = zero;
    dcopy___(n, &w[1], &c__0, &w[1], &c__1);
      }
  /* END OF SUBROUTINE NNLS */
  L290:
      return 0;
  } /* nnls_ */

  int hfti_(double *a, int *mda, int *m, int *
    n, double *b, int *mdb, int *nb, double *tau, int
    *krank, double *rnorm, double *h__, double *g, int *
    ip)
  {
      /* Initialized data */

      static double zero = 0.;
      static double factor = .001;

      /* System generated locals */
      int a_dim1, a_offset, b_dim1, b_offset, i__1, i__2, i__3;
      double d__1;

      /* Local variables */
      static int i__, j, k, l;
      static int jb, kp1;
      static double tmp, hmax;
      static int lmax, ldiag;

  /*     RANK-DEFICIENT LEAST SQUARES ALGORITHM AS DESCRIBED IN: */
  /*     C.L.LAWSON AND R.J.HANSON, JET PROPULSION LABORATORY, 1973 JUN 12 */
  /*     TO APPEAR IN 'SOLVING LEAST SQUARES PROBLEMS', PRENTICE-HALL, 1974 */
  /*     A(*,*),MDA,M,N   THE ARRAY A INITIALLY CONTAINS THE M x N MATRIX A */
  /*                      OF THE LEAST SQUARES PROBLEM AX = B. */
  /*                      THE FIRST DIMENSIONING PARAMETER MDA MUST SATISFY */
  /*                      MDA >= M. EITHER M >= N OR M < N IS PERMITTED. */
  /*                      THERE IS NO RESTRICTION ON THE RANK OF A. */
  /*                      THE MATRIX A WILL BE MODIFIED BY THE SUBROUTINE. */
  /*     B(*,*),MDB,NB    IF NB = 0 THE SUBROUTINE WILL MAKE NO REFERENCE */
  /*                      TO THE ARRAY B. IF NB > 0 THE ARRAY B() MUST */
  /*                      INITIALLY CONTAIN THE M x NB MATRIX B  OF THE */
  /*                      THE LEAST SQUARES PROBLEM AX = B AND ON RETURN */
  /*                      THE ARRAY B() WILL CONTAIN THE N x NB SOLUTION X. */
  /*                      IF NB>1 THE ARRAY B() MUST BE DOUBLE SUBSCRIPTED */
  /*                      WITH FIRST DIMENSIONING PARAMETER MDB>=MAX(M,N), */
  /*                      IF NB=1 THE ARRAY B() MAY BE EITHER SINGLE OR */
  /*                      DOUBLE SUBSCRIPTED. */
  /*     TAU              ABSOLUTE TOLERANCE PARAMETER FOR PSEUDORANK */
  /*                      DETERMINATION, PROVIDED BY THE USER. */
  /*     KRANK            PSEUDORANK OF A, SET BY THE SUBROUTINE. */
  /*     RNORM            ON EXIT, RNORM(J) WILL CONTAIN THE EUCLIDIAN */
  /*                      NORM OF THE RESIDUAL VECTOR FOR THE PROBLEM */
  /*                      DEFINED BY THE J-TH COLUMN VECTOR OF THE ARRAY B. */
  /*     H(), G()         ARRAYS OF WORKING SPACE OF LENGTH >= N. */
  /*     IP()             INT ARRAY OF WORKING SPACE OF LENGTH >= N */
  /*                      RECORDING PERMUTATION INDICES OF COLUMN VECTORS */
      /* Parameter adjustments */
      --ip;
      --g;
      --h__;
      a_dim1 = *mda;
      a_offset = 1 + a_dim1;
      a -= a_offset;
      --rnorm;
      b_dim1 = *mdb;
      b_offset = 1 + b_dim1;
      b -= b_offset;

      /* Function Body */
      k = 0;
      ldiag = std::min(*m,*n);
      if (ldiag <= 0) {
    goto L270;
      }
  /*   COMPUTE LMAX */
      i__1 = ldiag;
      for (j = 1; j <= i__1; ++j) {
    if (j == 1) {
        goto L20;
    }
    lmax = j;
    i__2 = *n;
    for (l = j; l <= i__2; ++l) {
  /* Computing 2nd power */
        d__1 = a[j - 1 + l * a_dim1];
        h__[l] -= d__1 * d__1;
  /* L10: */
        if (h__[l] > h__[lmax]) {
      lmax = l;
        }
    }
    d__1 = hmax + factor * h__[lmax];
    if (d__1 - hmax > zero) {
        goto L50;
    }
  L20:
    lmax = j;
    i__2 = *n;
    for (l = j; l <= i__2; ++l) {
        h__[l] = zero;
        i__3 = *m;
        for (i__ = j; i__ <= i__3; ++i__) {
  /* L30: */
  /* Computing 2nd power */
      d__1 = a[i__ + l * a_dim1];
      h__[l] += d__1 * d__1;
        }
  /* L40: */
        if (h__[l] > h__[lmax]) {
      lmax = l;
        }
    }
    hmax = h__[lmax];
  /*   COLUMN INTERCHANGES IF NEEDED */
  L50:
    ip[j] = lmax;
    if (ip[j] == j) {
        goto L70;
    }
    i__2 = *m;
    for (i__ = 1; i__ <= i__2; ++i__) {
        tmp = a[i__ + j * a_dim1];
        a[i__ + j * a_dim1] = a[i__ + lmax * a_dim1];
  /* L60: */
        a[i__ + lmax * a_dim1] = tmp;
    }
    h__[lmax] = h__[j];
  /*   J-TH TRANSFORMATION AND APPLICATION TO A AND B */
  L70:
  /* Computing MIN */
    i__2 = j + 1;
    i__ = std::min(i__2,*n);
    i__2 = j + 1;
    i__3 = *n - j;
    h12_(&c__1, &j, &i__2, m, &a[j * a_dim1 + 1], &c__1, &h__[j], &a[i__ *
       a_dim1 + 1], &c__1, mda, &i__3);
  /* L80: */
    i__2 = j + 1;
    h12_(&c__2, &j, &i__2, m, &a[j * a_dim1 + 1], &c__1, &h__[j], &b[
      b_offset], &c__1, mdb, nb);
      }
  /*   DETERMINE PSEUDORANK */
      i__2 = ldiag;
      for (j = 1; j <= i__2; ++j) {
  /* L90: */
    if ((d__1 = a[j + j * a_dim1], std::abs(d__1)) <= *tau) {
        goto L100;
    }
      }
      k = ldiag;
      goto L110;
  L100:
      k = j - 1;
  L110:
      kp1 = k + 1;
  /*   NORM OF RESIDUALS */
      i__2 = *nb;
      for (jb = 1; jb <= i__2; ++jb) {
  /* L130: */
    i__1 = *m - k;
    rnorm[jb] = dnrm2___(&i__1, &b[kp1 + jb * b_dim1], &c__1);
      }
      if (k > 0) {
    goto L160;
      }
      i__1 = *nb;
      for (jb = 1; jb <= i__1; ++jb) {
    i__2 = *n;
    for (i__ = 1; i__ <= i__2; ++i__) {
  /* L150: */
        b[i__ + jb * b_dim1] = zero;
    }
      }
      goto L270;
  L160:
      if (k == *n) {
    goto L180;
      }
  /*   HOUSEHOLDER DECOMPOSITION OF FIRST K ROWS */
      for (i__ = k; i__ >= 1; --i__) {
  /* L170: */
    i__2 = i__ - 1;
    h12_(&c__1, &i__, &kp1, n, &a[i__ + a_dim1], mda, &g[i__], &a[
      a_offset], mda, &c__1, &i__2);
      }
  L180:
      i__2 = *nb;
      for (jb = 1; jb <= i__2; ++jb) {
  /*   SOLVE K*K TRIANGULAR SYSTEM */
    for (i__ = k; i__ >= 1; --i__) {
  /* Computing MIN */
        i__1 = i__ + 1;
        j = std::min(i__1,*n);
  /* L210: */
        i__1 = k - i__;
        b[i__ + jb * b_dim1] = (b[i__ + jb * b_dim1] - ddot_sl__(&i__1, &
          a[i__ + j * a_dim1], mda, &b[j + jb * b_dim1], &c__1)) /
          a[i__ + i__ * a_dim1];
    }
  /*   COMPLETE SOLUTION VECTOR */
    if (k == *n) {
        goto L240;
    }
    i__1 = *n;
    for (j = kp1; j <= i__1; ++j) {
  /* L220: */
        b[j + jb * b_dim1] = zero;
    }
    i__1 = k;
    for (i__ = 1; i__ <= i__1; ++i__) {
  /* L230: */
        h12_(&c__2, &i__, &kp1, n, &a[i__ + a_dim1], mda, &g[i__], &b[jb *
           b_dim1 + 1], &c__1, mdb, &c__1);
    }
  /*   REORDER SOLUTION ACCORDING TO PREVIOUS COLUMN INTERCHANGES */
  L240:
    for (j = ldiag; j >= 1; --j) {
        if (ip[j] == j) {
      goto L250;
        }
        l = ip[j];
        tmp = b[l + jb * b_dim1];
        b[l + jb * b_dim1] = b[j + jb * b_dim1];
        b[j + jb * b_dim1] = tmp;
  L250:
        ;
    }
      }
  L270:
      *krank = k;
      return 0;
  } /* hfti_ */

  int h12_(int *mode, int *lpivot, int *l1,
    int *m, double *u, int *iue, double *up, double *
    c__, int *ice, int *icv, int *ncv)
  {
      /* Initialized data */

      static double one = 1.;
      static double zero = 0.;

      /* System generated locals */
      int u_dim1, u_offset, i__1, i__2;
      double d__1;

      /* Local variables */
      static double b;
      static int i__, j, i2, i3, i4;
      static double cl, sm;
      static int incr;
      static double clinv;

  /*     C.L.LAWSON AND R.J.HANSON, JET PROPULSION LABORATORY, 1973 JUN 12 */
  /*     TO APPEAR IN 'SOLVING LEAST SQUARES PROBLEMS', PRENTICE-HALL, 1974 */
  /*     CONSTRUCTION AND/OR APPLICATION OF A SINGLE */
  /*     HOUSEHOLDER TRANSFORMATION  Q = I + U*(U**T)/B */
  /*     MODE    = 1 OR 2   TO SELECT ALGORITHM  H1  OR  H2 . */
  /*     LPIVOT IS THE INDEX OF THE PIVOT ELEMENT. */
  /*     L1,M   IF L1 <= M   THE TRANSFORMATION WILL BE CONSTRUCTED TO */
  /*            ZERO ELEMENTS INDEXED FROM L1 THROUGH M. */
  /*            IF L1 > M THE SUBROUTINE DOES AN IDENTITY TRANSFORMATION. */
  /*     U(),IUE,UP */
  /*            ON ENTRY TO H1 U() STORES THE PIVOT VECTOR. */
  /*            IUE IS THE STORAGE INCREMENT BETWEEN ELEMENTS. */
  /*            ON EXIT FROM H1 U() AND UP STORE QUANTITIES DEFINING */
  /*            THE VECTOR U OF THE HOUSEHOLDER TRANSFORMATION. */
  /*            ON ENTRY TO H2 U() AND UP */
  /*            SHOULD STORE QUANTITIES PREVIOUSLY COMPUTED BY H1. */
  /*            THESE WILL NOT BE MODIFIED BY H2. */
  /*     C()    ON ENTRY TO H1 OR H2 C() STORES A MATRIX WHICH WILL BE */
  /*            REGARDED AS A SET OF VECTORS TO WHICH THE HOUSEHOLDER */
  /*            TRANSFORMATION IS TO BE APPLIED. */
  /*            ON EXIT C() STORES THE SET OF TRANSFORMED VECTORS. */
  /*     ICE    STORAGE INCREMENT BETWEEN ELEMENTS OF VECTORS IN C(). */
  /*     ICV    STORAGE INCREMENT BETWEEN VECTORS IN C(). */
  /*     NCV    NUMBER OF VECTORS IN C() TO BE TRANSFORMED. */
  /*            IF NCV <= 0 NO OPERATIONS WILL BE DONE ON C(). */
      /* Parameter adjustments */
      u_dim1 = *iue;
      u_offset = 1 + u_dim1;
      u -= u_offset;
      --c__;

      /* Function Body */
      if (0 >= *lpivot || *lpivot >= *l1 || *l1 > *m) {
    goto L80;
      }
      cl = (d__1 = u[*lpivot * u_dim1 + 1], std::abs(d__1));
      if (*mode == 2) {
    goto L30;
      }
  /*     ****** CONSTRUCT THE TRANSFORMATION ****** */
      i__1 = *m;
      for (j = *l1; j <= i__1; ++j) {
    sm = (d__1 = u[j * u_dim1 + 1], std::abs(d__1));
  /* L10: */
    cl = std::max(sm,cl);
      }
      if (cl <= zero) {
    goto L80;
      }
      clinv = one / cl;
  /* Computing 2nd power */
      d__1 = u[*lpivot * u_dim1 + 1] * clinv;
      sm = d__1 * d__1;
      i__1 = *m;
      for (j = *l1; j <= i__1; ++j) {
  /* L20: */
  /* Computing 2nd power */
    d__1 = u[j * u_dim1 + 1] * clinv;
    sm += d__1 * d__1;
      }
      cl *= sqrt(sm);
      if (u[*lpivot * u_dim1 + 1] > zero) {
    cl = -cl;
      }
      *up = u[*lpivot * u_dim1 + 1] - cl;
      u[*lpivot * u_dim1 + 1] = cl;
      goto L40;
  /*     ****** APPLY THE TRANSFORMATION  I+U*(U**T)/B  TO C ****** */
  L30:
      if (cl <= zero) {
    goto L80;
      }
  L40:
      if (*ncv <= 0) {
    goto L80;
      }
      b = *up * u[*lpivot * u_dim1 + 1];
      if (b >= zero) {
    goto L80;
      }
      b = one / b;
      i2 = 1 - *icv + *ice * (*lpivot - 1);
      incr = *ice * (*l1 - *lpivot);
      i__1 = *ncv;
      for (j = 1; j <= i__1; ++j) {
    i2 += *icv;
    i3 = i2 + incr;
    i4 = i3;
    sm = c__[i2] * *up;
    i__2 = *m;
    for (i__ = *l1; i__ <= i__2; ++i__) {
        sm += c__[i3] * u[i__ * u_dim1 + 1];
  /* L50: */
        i3 += *ice;
    }
    if (sm == zero) {
        goto L70;
    }
    sm *= b;
    c__[i2] += sm * *up;
    i__2 = *m;
    for (i__ = *l1; i__ <= i__2; ++i__) {
        c__[i4] += sm * u[i__ * u_dim1 + 1];
  /* L60: */
        i4 += *ice;
    }
  L70:
    ;
      }
  L80:
      return 0;
  } /* h12_ */

  int ldl_(int *n, double *a, double *z__,
    double *sigma, double *w)
  {
      /* Initialized data */

      static double zero = 0.;
      static double one = 1.;
      static double four = 4.;
      static double epmach = 2.22e-16;

      /* System generated locals */
      int i__1, i__2;

      /* Local variables */
      static int i__, j;
      static double t, u, v;
      static int ij;
      static double tp, beta, gamma, alpha, delta;

  /*   LDL     LDL' - RANK-ONE - UPDATE */
  /*   PURPOSE: */
  /*           UPDATES THE LDL' FACTORS OF MATRIX A BY RANK-ONE MATRIX */
  /*           SIGMA*Z*Z' */
  /*   INPUT ARGUMENTS: (* MEANS PARAMETERS ARE CHANGED DURING EXECUTION) */
  /*     N     : ORDER OF THE COEFFICIENT MATRIX A */
  /*   * A     : POSITIVE DEFINITE MATRIX OF DIMENSION N; */
  /*             ONLY THE LOWER TRIANGLE IS USED AND IS STORED COLUMN BY */
  /*             COLUMN AS ONE DIMENSIONAL ARRAY OF DIMENSION N*(N+1)/2. */
  /*   * Z     : VECTOR OF DIMENSION N OF UPDATING ELEMENTS */
  /*     SIGMA : SCALAR FACTOR BY WHICH THE MODIFYING DYADE Z*Z' IS */
  /*             MULTIPLIED */
  /*   OUTPUT ARGUMENTS: */
  /*     A     : UPDATED LDL' FACTORS */
  /*   WORKING ARRAY: */
  /*     W     : VECTOR OP DIMENSION N (USED ONLY IF SIGMA .LT. ZERO) */
  /*   METHOD: */
  /*     THAT OF FLETCHER AND POWELL AS DESCRIBED IN : */
  /*     FLETCHER,R.,(1974) ON THE MODIFICATION OF LDL' FACTORIZATION. */
  /*     POWELL,M.J.D.      MATH.COMPUTATION 28, 1067-1078. */
  /*   IMPLEMENTED BY: */
  /*     KRAFT,D., DFVLR - INSTITUT FUER DYNAMIK DER FLUGSYSTEME */
  /*               D-8031  OBERPFAFFENHOFEN */
  /*   STATUS: 15. JANUARY 1980 */
  /*   SUBROUTINES REQUIRED: NONE */
      /* Parameter adjustments */
      --w;
      --z__;
      --a;

      /* Function Body */
      if (*sigma == zero) {
    goto L280;
      }
      ij = 1;
      t = one / *sigma;
      if (*sigma > zero) {
    goto L220;
      }
  /* PREPARE NEGATIVE UPDATE */
      i__1 = *n;
      for (i__ = 1; i__ <= i__1; ++i__) {
  /* L150: */
    w[i__] = z__[i__];
      }
      i__1 = *n;
      for (i__ = 1; i__ <= i__1; ++i__) {
    v = w[i__];
    t += v * v / a[ij];
    i__2 = *n;
    for (j = i__ + 1; j <= i__2; ++j) {
        ++ij;
  /* L160: */
        w[j] -= v * a[ij];
    }
  /* L170: */
    ++ij;
      }
      if (t >= zero) {
    t = epmach / *sigma;
      }
      i__1 = *n;
      for (i__ = 1; i__ <= i__1; ++i__) {
    j = *n + 1 - i__;
    ij -= i__;
    u = w[j];
    w[j] = t;
  /* L210: */
    t -= u * u / a[ij];
      }
  L220:
  /* HERE UPDATING BEGINS */
      i__1 = *n;
      for (i__ = 1; i__ <= i__1; ++i__) {
    v = z__[i__];
    delta = v / a[ij];
    if (*sigma < zero) {
        tp = w[i__];
    }
    if (*sigma > zero) {
        tp = t + delta * v;
    }
    alpha = tp / t;
    a[ij] = alpha * a[ij];
    if (i__ == *n) {
        goto L280;
    }
    beta = delta / tp;
    if (alpha > four) {
        goto L240;
    }
    i__2 = *n;
    for (j = i__ + 1; j <= i__2; ++j) {
        ++ij;
        z__[j] -= v * a[ij];
  /* L230: */
        a[ij] += beta * z__[j];
    }
    goto L260;
  L240:
    gamma = t / tp;
    i__2 = *n;
    for (j = i__ + 1; j <= i__2; ++j) {
        ++ij;
        u = a[ij];
        a[ij] = gamma * u + beta * z__[j];
  /* L250: */
        z__[j] -= v * u;
    }
  L260:
    ++ij;
  /* L270: */
    t = tp;
      }
  L280:
      return 0;
  /* END OF LDL */
  } /* ldl_ */

  double linmin_(int *mode, double *ax, double *bx, double *
    f, double *tol)
  {
      /* Initialized data */

      static double c__ = .381966011;
      static double eps = 1.5e-8;
      static double zero = 0.;

      /* System generated locals */
      double ret_val, d__1;

      /* Local variables */
      static double a, b, d__, e, m, p, q, r__, u, v, w, x, fu, fv, fw, fx,
        tol1, tol2;

  /*   LINMIN  LINESEARCH WITHOUT DERIVATIVES */
  /*   PURPOSE: */
  /*  TO FIND THE ARGUMENT LINMIN WHERE THE FUNCTION F TAKES IT'S MINIMUM */
  /*  ON THE INTERVAL AX, BX. */
  /*  COMBINATION OF GOLDEN SECTION AND SUCCESSIVE QUADRATIC INTERPOLATION. */
  /*   INPUT ARGUMENTS: (* MEANS PARAMETERS ARE CHANGED DURING EXECUTION) */
  /* *MODE   SEE OUTPUT ARGUMENTS */
  /*  AX     LEFT ENDPOINT OF INITIAL INTERVAL */
  /*  BX     RIGHT ENDPOINT OF INITIAL INTERVAL */
  /*  F      FUNCTION VALUE AT LINMIN WHICH IS TO BE BROUGHT IN BY */
  /*         REVERSE COMMUNICATION CONTROLLED BY MODE */
  /*  TOL    DESIRED LENGTH OF INTERVAL OF UNCERTAINTY OF FINAL RESULT */
  /*   OUTPUT ARGUMENTS: */
  /*  LINMIN ABSCISSA APPROXIMATING THE POINT WHERE F ATTAINS A MINIMUM */
  /*  MODE   CONTROLS REVERSE COMMUNICATION */
  /*         MUST BE SET TO 0 INITIALLY, RETURNS WITH INTERMEDIATE */
  /*         VALUES 1 AND 2 WHICH MUST NOT BE CHANGED BY THE USER, */
  /*         ENDS WITH CONVERGENCE WITH VALUE 3. */
  /*   WORKING ARRAY: */
  /*  NONE */
  /*   METHOD: */
  /*  THIS FUNCTION SUBPROGRAM IS A SLIGHTLY MODIFIED VERSION OF THE */
  /*  ALGOL 60 PROCEDURE LOCALMIN GIVEN IN */
  /*  R.P. BRENT: ALGORITHMS FOR MINIMIZATION WITHOUT DERIVATIVES, */
  /*              PRENTICE-HALL (1973). */
  /*   IMPLEMENTED BY: */
  /*     KRAFT, D., DFVLR - INSTITUT FUER DYNAMIK DER FLUGSYSTEME */
  /*                D-8031  OBERPFAFFENHOFEN */
  /*   STATUS: 31. AUGUST  1984 */
  /*   SUBROUTINES REQUIRED: NONE */
  /*  EPS = SQUARE - ROOT OF MACHINE PRECISION */
  /*  C = GOLDEN SECTION RATIO = (3-SQRT(5))/2 */
      switch (*mode) {
    case 1:  goto L10;
    case 2:  goto L55;
      }
  /*  INITIALIZATION */
      a = *ax;
      b = *bx;
      e = zero;
      v = a + c__ * (b - a);
      w = v;
      x = w;
      ret_val = x;
      *mode = 1;
      goto L100;
  /*  MAIN LOOP STARTS HERE */
  L10:
      fx = *f;
      fv = fx;
      fw = fv;
  L20:
      m = (a + b) * .5;
      tol1 = eps * std::abs(x) + *tol;
      tol2 = tol1 + tol1;
  /*  TEST CONVERGENCE */
      if ((d__1 = x - m, std::abs(d__1)) <= tol2 - (b - a) * .5) {
    goto L90;
      }
      r__ = zero;
      q = r__;
      p = q;
      if (std::abs(e) <= tol1) {
    goto L30;
      }
  /*  FIT PARABOLA */
      r__ = (x - w) * (fx - fv);
      q = (x - v) * (fx - fw);
      p = (x - v) * q - (x - w) * r__;
      q -= r__;
      q += q;
      if (q > zero) {
    p = -p;
      }
      if (q < zero) {
    q = -q;
      }
      r__ = e;
      e = d__;
  /*  IS PARABOLA ACCEPTABLE */
  L30:
      if (std::abs(p) >= (d__1 = q * r__, std::abs(d__1)) * .5 || p <= q * (a - x) || p >=
         q * (b - x)) {
    goto L40;
      }
  /*  PARABOLIC INTERPOLATION STEP */
      d__ = p / q;
  /*  F MUST NOT BE EVALUATED TOO CLOSE TO A OR B */
      if (u - a < tol2) {
    d__1 = m - x;
    d__ = d_sign(&tol1, &d__1);
      }
      if (b - u < tol2) {
    d__1 = m - x;
    d__ = d_sign(&tol1, &d__1);
      }
      goto L50;
  /*  GOLDEN SECTION STEP */
  L40:
      if (x >= m) {
    e = a - x;
      }
      if (x < m) {
    e = b - x;
      }
      d__ = c__ * e;
  /*  F MUST NOT BE EVALUATED TOO CLOSE TO X */
  L50:
      if (std::abs(d__) < tol1) {
    d__ = d_sign(&tol1, &d__);
      }
      u = x + d__;
      ret_val = u;
      *mode = 2;
      goto L100;
  L55:
      fu = *f;
  /*  UPDATE A, B, V, W, AND X */
      if (fu > fx) {
    goto L60;
      }
      if (u >= x) {
    a = x;
      }
      if (u < x) {
    b = x;
      }
      v = w;
      fv = fw;
      w = x;
      fw = fx;
      x = u;
      fx = fu;
      goto L85;
  L60:
      if (u < x) {
    a = u;
      }
      if (u >= x) {
    b = u;
      }
      if (fu <= fw || w == x) {
    goto L70;
      }
      if (fu <= fv || v == x || v == w) {
    goto L80;
      }
      goto L85;
  L70:
      v = w;
      fv = fw;
      w = u;
      fw = fu;
      goto L85;
  L80:
      v = u;
      fv = fu;
  L85:
      goto L20;
  /*  END OF MAIN LOOP */
  L90:
      ret_val = x;
      *mode = 3;
  L100:
      return ret_val;
  /*  END OF LINMIN */
  } /* linmin_ */

  /* ## Following a selection from BLAS Level 1 */
  int daxpy_sl__(int *n, double *da, double *dx,
    int *incx, double *dy, int *incy)
  {
      /* System generated locals */
      int i__1;

      /* Local variables */
      static int i__, m, ix, iy, mp1;

  /*     CONSTANT TIMES A VECTOR PLUS A VECTOR. */
  /*     USES UNROLLED LOOPS FOR INCREMENTS EQUAL TO ONE. */
  /*     JACK DONGARRA, LINPACK, 3/11/78. */
      /* Parameter adjustments */
      --dy;
      --dx;

      /* Function Body */
      if (*n <= 0) {
    return 0;
      }
      if (*da == 0.) {
    return 0;
      }
      if (*incx == 1 && *incy == 1) {
    goto L20;
      }
  /*        CODE FOR UNEQUAL INCREMENTS OR EQUAL INCREMENTS */
  /*        NOT EQUAL TO 1 */
      ix = 1;
      iy = 1;
      if (*incx < 0) {
    ix = (-(*n) + 1) * *incx + 1;
      }
      if (*incy < 0) {
    iy = (-(*n) + 1) * *incy + 1;
      }
      i__1 = *n;
      for (i__ = 1; i__ <= i__1; ++i__) {
    dy[iy] += *da * dx[ix];
    ix += *incx;
    iy += *incy;
  /* L10: */
      }
      return 0;
  /*        CODE FOR BOTH INCREMENTS EQUAL TO 1 */
  /*        CLEAN-UP LOOP */
  L20:
      m = *n % 4;
      if (m == 0) {
    goto L40;
      }
      i__1 = m;
      for (i__ = 1; i__ <= i__1; ++i__) {
    dy[i__] += *da * dx[i__];
  /* L30: */
      }
      if (*n < 4) {
    return 0;
      }
  L40:
      mp1 = m + 1;
      i__1 = *n;
      for (i__ = mp1; i__ <= i__1; i__ += 4) {
    dy[i__] += *da * dx[i__];
    dy[i__ + 1] += *da * dx[i__ + 1];
    dy[i__ + 2] += *da * dx[i__ + 2];
    dy[i__ + 3] += *da * dx[i__ + 3];
  /* L50: */
      }
      return 0;
  } /* daxpy_sl__ */

  int dcopy___(int *n, double *dx, int *incx,
    double *dy, int *incy)
  {
      /* System generated locals */
      int i__1;

      /* Local variables */
      static int i__, m, ix, iy, mp1;

  /*     COPIES A VECTOR, X, TO A VECTOR, Y. */
  /*     USES UNROLLED LOOPS FOR INCREMENTS EQUAL TO ONE. */
  /*     JACK DONGARRA, LINPACK, 3/11/78. */
      /* Parameter adjustments */
      --dy;
      --dx;

      /* Function Body */
      if (*n <= 0) {
    return 0;
      }
      if (*incx == 1 && *incy == 1) {
    goto L20;
      }
  /*        CODE FOR UNEQUAL INCREMENTS OR EQUAL INCREMENTS */
  /*        NOT EQUAL TO 1 */
      ix = 1;
      iy = 1;
      if (*incx < 0) {
    ix = (-(*n) + 1) * *incx + 1;
      }
      if (*incy < 0) {
    iy = (-(*n) + 1) * *incy + 1;
      }
      i__1 = *n;
      for (i__ = 1; i__ <= i__1; ++i__) {
    dy[iy] = dx[ix];
    ix += *incx;
    iy += *incy;
  /* L10: */
      }
      return 0;
  /*        CODE FOR BOTH INCREMENTS EQUAL TO 1 */
  /*        CLEAN-UP LOOP */
  L20:
      m = *n % 7;
      if (m == 0) {
    goto L40;
      }
      i__1 = m;
      for (i__ = 1; i__ <= i__1; ++i__) {
    dy[i__] = dx[i__];
  /* L30: */
      }
      if (*n < 7) {
    return 0;
      }
  L40:
      mp1 = m + 1;
      i__1 = *n;
      for (i__ = mp1; i__ <= i__1; i__ += 7) {
    dy[i__] = dx[i__];
    dy[i__ + 1] = dx[i__ + 1];
    dy[i__ + 2] = dx[i__ + 2];
    dy[i__ + 3] = dx[i__ + 3];
    dy[i__ + 4] = dx[i__ + 4];
    dy[i__ + 5] = dx[i__ + 5];
    dy[i__ + 6] = dx[i__ + 6];
  /* L50: */
      }
      return 0;
  } /* dcopy___ */

  double ddot_sl__(int *n, double *dx, int *incx, double *
    dy, int *incy)
  {
      /* System generated locals */
      int i__1;
      double ret_val;

      /* Local variables */
      static int i__, m, ix, iy, mp1;
      static double dtemp;

  /*     FORMS THE DOT PRODUCT OF TWO VECTORS. */
  /*     USES UNROLLED LOOPS FOR INCREMENTS EQUAL TO ONE. */
  /*     JACK DONGARRA, LINPACK, 3/11/78. */
      /* Parameter adjustments */
      --dy;
      --dx;

      /* Function Body */
      ret_val = 0.;
      dtemp = 0.;
      if (*n <= 0) {
    return ret_val;
      }
      if (*incx == 1 && *incy == 1) {
    goto L20;
      }
  /*        CODE FOR UNEQUAL INCREMENTS OR EQUAL INCREMENTS */
  /*          NOT EQUAL TO 1 */
      ix = 1;
      iy = 1;
      if (*incx < 0) {
    ix = (-(*n) + 1) * *incx + 1;
      }
      if (*incy < 0) {
    iy = (-(*n) + 1) * *incy + 1;
      }
      i__1 = *n;
      for (i__ = 1; i__ <= i__1; ++i__) {
    dtemp += dx[ix] * dy[iy];
    ix += *incx;
    iy += *incy;
  /* L10: */
      }
      ret_val = dtemp;
      return ret_val;
  /*        CODE FOR BOTH INCREMENTS EQUAL TO 1 */
  /*        CLEAN-UP LOOP */
  L20:
      m = *n % 5;
      if (m == 0) {
    goto L40;
      }
      i__1 = m;
      for (i__ = 1; i__ <= i__1; ++i__) {
    dtemp += dx[i__] * dy[i__];
  /* L30: */
      }
      if (*n < 5) {
    goto L60;
      }
  L40:
      mp1 = m + 1;
      i__1 = *n;
      for (i__ = mp1; i__ <= i__1; i__ += 5) {
    dtemp = dtemp + dx[i__] * dy[i__] + dx[i__ + 1] * dy[i__ + 1] + dx[
      i__ + 2] * dy[i__ + 2] + dx[i__ + 3] * dy[i__ + 3] + dx[i__ +
      4] * dy[i__ + 4];
  /* L50: */
      }
  L60:
      ret_val = dtemp;
      return ret_val;
  } /* ddot_sl__ */

  double dnrm2___(int *n, double *dx, int *incx)
  {
      /* Initialized data */

      static double zero = 0.;
      static double one = 1.;
      static double cutlo = 8.232e-11;
      static double cuthi = 1.304e19;

      /* Format strings */
      /* -- Unused -- */
      /* static char fmt_30[] = "";
      static char fmt_50[] = "";
      static char fmt_70[] = "";
      static char fmt_110[] = ""; */

      /* System generated locals */
      int i__1, i__2;
      double ret_val, d__1;


      /* Local variables */
      static int i__, j, nn;
      static double sum, xmax;
      static int next;
      static double hitest;

      /* Assigned format variables */
      /* static char *next_fmt = fmt_30; */ /* Avoid compiler warning */

      /* Parameter adjustments */
      --dx;

      /* Function Body */
  /*     EUCLIDEAN NORM OF THE N-VECTOR STORED IN DX() WITH STORAGE */
  /*     INCREMENT INCX . */
  /*     IF    N .LE. 0 RETURN WITH RESULT = 0. */
  /*     IF N .GE. 1 THEN INCX MUST BE .GE. 1 */
  /*           C.L.LAWSON, 1978 JAN 08 */
  /*     FOUR PHASE METHOD     USING TWO BUILT-IN CONSTANTS THAT ARE */
  /*     HOPEFULLY APPLICABLE TO ALL MACHINES. */
  /*         CUTLO = MAXIMUM OF  SQRT(U/EPS)   OVER ALL KNOWN MACHINES. */
  /*         CUTHI = MINIMUM OF  SQRT(V)       OVER ALL KNOWN MACHINES. */
  /*     WHERE */
  /*         EPS = SMALLEST NO. SUCH THAT EPS + 1. .GT. 1. */
  /*         U   = SMALLEST POSITIVE NO.   (UNDERFLOW LIMIT) */
  /*         V   = LARGEST  NO.            (OVERFLOW  LIMIT) */
  /*     BRIEF OUTLINE OF ALGORITHM.. */
  /*     PHASE 1    SCANS ZERO COMPONENTS. */
  /*     MOVE TO PHASE 2 WHEN A COMPONENT IS NONZERO AND .LE. CUTLO */
  /*     MOVE TO PHASE 3 WHEN A COMPONENT IS .GT. CUTLO */
  /*     MOVE TO PHASE 4 WHEN A COMPONENT IS .GE. CUTHI/M */
  /*     WHERE M = N FOR X() REAL AND M = 2*N FOR COMPLEX. */
  /*     VALUES FOR CUTLO AND CUTHI.. */
  /*     FROM THE ENVIRONMENTAL PARAMETERS LISTED IN THE IMSL CONVERTER */
  /*     DOCUMENT THE LIMITING VALUES ARE AS FOLLOWS.. */
  /*     CUTLO, S.P.   U/EPS = 2**(-102) FOR  HONEYWELL.  CLOSE SECONDS ARE */
  /*                   UNIVAC AND DEC AT 2**(-103) */
  /*                   THUS CUTLO = 2**(-51) = 4.44089E-16 */
  /*     CUTHI, S.P.   V = 2**127 FOR UNIVAC, HONEYWELL, AND DEC. */
  /*                   THUS CUTHI = 2**(63.5) = 1.30438E19 */
  /*     CUTLO, D.P.   U/EPS = 2**(-67) FOR HONEYWELL AND DEC. */
  /*                   THUS CUTLO = 2**(-33.5) = 8.23181D-11 */
  /*     CUTHI, D.P.   SAME AS S.P.  CUTHI = 1.30438D19 */
  /*     DATA CUTLO, CUTHI / 8.232D-11,  1.304D19 / */
  /*     DATA CUTLO, CUTHI / 4.441E-16,  1.304E19 / */
      if (*n > 0) {
    goto L10;
      }
      ret_val = zero;
      goto L300;
  L10:
      next = 0;
      /* next_fmt = fmt_30; */
      sum = zero;
      nn = *n * *incx;
  /*                       BEGIN MAIN LOOP */
      i__ = 1;
  L20:
      switch (next) {
    case 0: goto L30;
    case 1: goto L50;
    case 2: goto L70;
    case 3: goto L110;
      }
  L30:
      if ((d__1 = dx[i__], std::abs(d__1)) > cutlo) {
    goto L85;
      }
      next = 1;
      /* next_fmt = fmt_50 */;
      xmax = zero;
  /*                        PHASE 1.  SUM IS ZERO */
  L50:
      if (dx[i__] == zero) {
    goto L200;
      }
      if ((d__1 = dx[i__], std::abs(d__1)) > cutlo) {
    goto L85;
      }
  /*                        PREPARE FOR PHASE 2. */
      next = 2;
      /* next_fmt = fmt_70; */
      goto L105;
  /*                        PREPARE FOR PHASE 4. */
  L100:
      i__ = j;
      next = 3;
      /* next_fmt = fmt_110; */
      sum = sum / dx[i__] / dx[i__];
  L105:
      xmax = (d__1 = dx[i__], std::abs(d__1));
      goto L115;
  /*                   PHASE 2.  SUM IS SMALL. */
  /*                             SCALE TO AVOID DESTRUCTIVE UNDERFLOW. */
  L70:
      if ((d__1 = dx[i__], std::abs(d__1)) > cutlo) {
    goto L75;
      }
  /*                   COMMON CODE FOR PHASES 2 AND 4. */
  /*                   IN PHASE 4 SUM IS LARGE.  SCALE TO AVOID OVERFLOW. */
  L110:
      if ((d__1 = dx[i__], std::abs(d__1)) <= xmax) {
    goto L115;
      }
  /* Computing 2nd power */
      d__1 = xmax / dx[i__];
      sum = one + sum * (d__1 * d__1);
      xmax = (d__1 = dx[i__], std::abs(d__1));
      goto L200;
  L115:
  /* Computing 2nd power */
      d__1 = dx[i__] / xmax;
      sum += d__1 * d__1;
      goto L200;
  /*                  PREPARE FOR PHASE 3. */
  L75:
      sum = sum * xmax * xmax;
  /*     FOR REAL OR D.P. SET HITEST = CUTHI/N */
  /*     FOR COMPLEX      SET HITEST = CUTHI/(2*N) */
  L85:
      hitest = cuthi / (float) (*n);
  /*                   PHASE 3.  SUM IS MID-RANGE.  NO SCALING. */
      i__1 = nn;
      i__2 = *incx;
      for (j = i__; i__2 < 0 ? j >= i__1 : j <= i__1; j += i__2) {
    if ((d__1 = dx[j], std::abs(d__1)) >= hitest) {
        goto L100;
    }
  /* L95: */
  /* Computing 2nd power */
    d__1 = dx[j];
    sum += d__1 * d__1;
      }
      ret_val = sqrt(sum);
      goto L300;
  L200:
      i__ += *incx;
      if (i__ <= nn) {
    goto L20;
      }
  /*              END OF MAIN LOOP. */
  /*              COMPUTE SQUARE ROOT AND ADJUST FOR SCALING. */
      ret_val = xmax * sqrt(sum);
  L300:
      return ret_val;
  } /* dnrm2___ */

  int dsrot_(int *n, double *dx, int *incx,
    double *dy, int *incy, double *c__, double *s)
  {
      /* System generated locals */
      int i__1;

      /* Local variables */
      static int i__, ix, iy;
      static double dtemp;

  /*     APPLIES A PLANE ROTATION. */
  /*     JACK DONGARRA, LINPACK, 3/11/78. */
      /* Parameter adjustments */
      --dy;
      --dx;

      /* Function Body */
      if (*n <= 0) {
    return 0;
      }
      if (*incx == 1 && *incy == 1) {
    goto L20;
      }
  /*       CODE FOR UNEQUAL INCREMENTS OR EQUAL INCREMENTS NOT EQUAL */
  /*         TO 1 */
      ix = 1;
      iy = 1;
      if (*incx < 0) {
    ix = (-(*n) + 1) * *incx + 1;
      }
      if (*incy < 0) {
    iy = (-(*n) + 1) * *incy + 1;
      }
      i__1 = *n;
      for (i__ = 1; i__ <= i__1; ++i__) {
    dtemp = *c__ * dx[ix] + *s * dy[iy];
    dy[iy] = *c__ * dy[iy] - *s * dx[ix];
    dx[ix] = dtemp;
    ix += *incx;
    iy += *incy;
  /* L10: */
      }
      return 0;
  /*       CODE FOR BOTH INCREMENTS EQUAL TO 1 */
  L20:
      i__1 = *n;
      for (i__ = 1; i__ <= i__1; ++i__) {
    dtemp = *c__ * dx[i__] + *s * dy[i__];
    dy[i__] = *c__ * dy[i__] - *s * dx[i__];
    dx[i__] = dtemp;
  /* L30: */
      }
      return 0;
  } /* dsrot_ */

  int dsrotg_(double *da, double *db, double *c__,
    double *s)
  {
      /* Initialized data */

      static double one = 1.;
      static double zero = 0.;

      /* System generated locals */
      double d__1, d__2;

      /* Local variables */
      static double r__, z__, roe, scale;

  /*     CONSTRUCT GIVENS PLANE ROTATION. */
  /*     JACK DONGARRA, LINPACK, 3/11/78. */
  /*                    MODIFIED 9/27/86. */
      roe = *db;
      if (std::abs(*da) > std::abs(*db)) {
    roe = *da;
      }
      scale = std::abs(*da) + std::abs(*db);
      if (scale != zero) {
    goto L10;
      }
      *c__ = one;
      *s = zero;
      r__ = zero;
      goto L20;
  L10:
  /* Computing 2nd power */
      d__1 = *da / scale;
  /* Computing 2nd power */
      d__2 = *db / scale;
      r__ = scale * sqrt(d__1 * d__1 + d__2 * d__2);
      r__ = d_sign(&one, &roe) * r__;
      *c__ = *da / r__;
      *s = *db / r__;
  L20:
      z__ = *s;
      if (std::abs(*c__) > zero && std::abs(*c__) <= *s) {
    z__ = one / *c__;
      }
      *da = r__;
      *db = z__;
      return 0;
  } /* dsrotg_ */

  int dscal_sl__(int *n, double *da, double *dx,
    int *incx)
  {
      /* System generated locals */
      int i__1, i__2;

      /* Local variables */
      static int i__, m, mp1, nincx;

  /*     SCALES A VECTOR BY A CONSTANT. */
  /*     USES UNROLLED LOOPS FOR INCREMENT EQUAL TO ONE. */
  /*     JACK DONGARRA, LINPACK, 3/11/78. */
      /* Parameter adjustments */
      --dx;

      /* Function Body */
      if (*n <= 0) {
    return 0;
      }
      if (*incx == 1) {
    goto L20;
      }
  /*        CODE FOR INCREMENT NOT EQUAL TO 1 */
      nincx = *n * *incx;
      i__1 = nincx;
      i__2 = *incx;
      for (i__ = 1; i__2 < 0 ? i__ >= i__1 : i__ <= i__1; i__ += i__2) {
    dx[i__] = *da * dx[i__];
  /* L10: */
      }
      return 0;
  /*        CODE FOR INCREMENT EQUAL TO 1 */
  /*        CLEAN-UP LOOP */
  L20:
      m = *n % 5;
      if (m == 0) {
    goto L40;
      }
      i__2 = m;
      for (i__ = 1; i__ <= i__2; ++i__) {
    dx[i__] = *da * dx[i__];
  /* L30: */
      }
      if (*n < 5) {
    return 0;
      }
  L40:
      mp1 = m + 1;
      i__2 = *n;
      for (i__ = mp1; i__ <= i__2; i__ += 5) {
    dx[i__] = *da * dx[i__];
    dx[i__ + 1] = *da * dx[i__ + 1];
    dx[i__ + 2] = *da * dx[i__ + 2];
    dx[i__ + 3] = *da * dx[i__ + 3];
    dx[i__ + 4] = *da * dx[i__ + 4];
  /* L50: */
      }
      return 0;
  } /* dscal_sl__ */

  ///@endcond

} // <anonymous>

} //namespace Math
}} // namespace Mantid::Kernel
