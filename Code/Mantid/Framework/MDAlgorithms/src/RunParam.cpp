//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidMDAlgorithms/RunParam.h"
#include "MantidAPI/IMDWorkspace.h"

#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/Tolerance.h"
#include "MantidGeometry/Math/mathSupport.h"
#include "MantidKernel/Matrix.h"

namespace Mantid
{
  namespace MDAlgorithms
  {
    // Constructor
    RunParam::RunParam()
    {
    }

    RunParam::RunParam(double ei,double psi,double elo,double ehi,
        double de, double x0, double xa, double x1,
        double wa, double ha, double s1, double s2,
        double s3, double s4, double s5, double thetam,
        int modModel, double pslit, double radius,
        double rho, double hz, double tjit,
        double as, double bs, double cs,
        double aa, double bb, double cc,
        double uh, double uk, double ul,
        double vh, double vk, double vl,
        double omega, double gs, double gl,
        double dpsi, double xh, double xk,
        double xl, double yh, double yk,
        double yl, double sx, double sy,
        double sz, int isam, double temp,
        double eta) :

        m_ei(ei), m_psi(psi), m_elo(elo),m_ehi(ehi),
        m_de(de), m_x0(x0), m_xa(xa), m_x1(x1),
        m_s1(s1), m_s2(s2),
        m_s3(s3), m_s4(s4), m_s5(s5), m_thetam(thetam),
        m_modModel(modModel),
        m_tjit(tjit),
        m_as(as), m_bs(bs), m_cs(cs),
        m_aa(aa), m_bb(bb), m_cc(cc),
        m_uh(uh), m_uk(uk), m_ul(ul),
        m_vh(vh), m_vk(vk), m_vl(vl),
        m_omega(omega), m_gs(gs), m_gl(gl),
        m_dpsi(dpsi), m_xh(xh), m_xk(xk),
        m_xl(xl), m_yh(yh), m_yk(yk),
        m_yl(yl), m_sx(sx), m_sy(sy),
        m_sz(sz), m_isam(isam), m_temp(temp)
    {
      setAa(aa);
      setBb(bb);
      setCc(cc);
      setHz(hz);
      setPslit(pslit);
      setRadius(radius);
      setRho(rho);
      setWa(wa);
      setHa(ha);
      setEta(eta);
      setTauChopperSignal();
    };

    RunParam::~RunParam()
    {}

    const double RunParam::mmTom = 1.e-3;
    /// @cond
    double RunParam::getEi() {return m_ei;}
    double RunParam::getPsi() {return m_psi;}
    double RunParam::getElo() {return m_elo;}
    double RunParam::getEhi() {return m_ehi;}
    double RunParam::getDe() {return m_de;}
    double RunParam::getX0() {return m_x0;}
    double RunParam::getXa() {return m_xa;}
    double RunParam::getX1() {return m_x1;}
    double RunParam::getWa() {return m_wa;}
    double RunParam::getHa() {return m_ha;}
    double RunParam::getS1() {return m_s1;}
    double RunParam::getS2() {return m_s2;}
    double RunParam::getS3() {return m_s3;}
    double RunParam::getS4() {return m_s4;}
    double RunParam::getS5() {return m_s5;}
    double RunParam::getThetam() {return m_thetam;}
    int RunParam::getModModel() {return m_modModel;}
    double RunParam::getPslit() {return m_pslit;}
    double RunParam::getRadius() {return m_radius;}
    double RunParam::getRho() {return m_rho;}
    double RunParam::getHz() {return m_hz;}
    double RunParam::getTjit() {return m_tjit;}
    double RunParam::getAs() {return m_as;}
    double RunParam::getBs() {return m_bs;}
    double RunParam::getCs() {return m_cs;}
    double RunParam::getAa() {return m_aa;}
    double RunParam::getBb() {return m_bb;}
    double RunParam::getCc() {return m_cc;}
    double RunParam::getUh() {return m_uh;}
    double RunParam::getUk() {return m_uk;}
    double RunParam::getUl() {return m_ul;}
    double RunParam::getVh() {return m_vh;}
    double RunParam::getVk() {return m_vk;}
    double RunParam::getVl() {return m_vl;}
    double RunParam::getOmega() {return m_omega;}
    double RunParam::getGs() {return m_gs;}
    double RunParam::getGl() {return m_gl;}
    double RunParam::getDpsi() {return m_dpsi;}
    double RunParam::getXh() {return m_xh;}
    double RunParam::getXk() {return m_xk;}
    double RunParam::getXl() {return m_xl;}
    double RunParam::getYh() {return m_yh;}
    double RunParam::getYk() {return m_yk;}
    double RunParam::getYl() {return m_yl;}
    double RunParam::getSx() {return m_sx;}
    double RunParam::getSy() {return m_sy;}
    double RunParam::getSz() {return m_sz;}
    int RunParam::getIsam() {return m_isam;}
    double RunParam::getTemp() {return m_temp;}
    double RunParam::getEta() {return m_eta;}

    void RunParam::setEi(const double val)
    {m_ei=val;}
    void RunParam::setPsi(const double val)
    {m_psi=val;}
    void RunParam::setElo(const double val)
    {m_elo=val;}
    void RunParam::setEhi(const double val)
    {m_ehi=val;}
    void RunParam::setDe(const double val)
    {m_de=val;}
    void RunParam::setX0(const double val)
    {m_x0=val;}
    void RunParam::setXa(const double val)
    {m_xa=val;}
    void RunParam::setX1(const double val)
    {m_x1=val;}
    void RunParam::setWa(const double val)
    {
      m_wa=val*mmTom; // mm to metres
    }
    void RunParam::setHa(const double val)
    {
      m_ha=val*mmTom; // mm to metres
    }
    void RunParam::setS1(const double val)
    {m_s1=val; m_moderatorChange=true;}
    void RunParam::setS2(const double val)
    {m_s2=val; m_moderatorChange=true;}
    void RunParam::setS3(const double val)
    {m_s3=val; m_moderatorChange=true;}
    void RunParam::setS4(const double val)
    {m_s4=val; m_moderatorChange=true;}
    void RunParam::setS5(const double val)
    {m_s5=val; m_moderatorChange=true;}
    void RunParam::setThetam(const double val)
    {m_thetam=val;}
    void RunParam::setModModel(const int val)
    {m_modModel=val; m_moderatorChange=true;}
    /// set pslit of chopper - input mm, internel m
    void RunParam::setPslit(const double val)
    {m_pslit=val*mmTom; }
    /// set radius of curvature of chopper - input mm, internel m
    void RunParam::setRadius(const double val)
    {m_radius=val*mmTom;m_chopChange=true;}
    /// set rho of chopper - input mm, internel m
    void RunParam::setRho(const double val)
    {m_rho=val*mmTom;m_chopChange=true;}
    /// Set frequency of chopper, internally use angular velocity
    void RunParam::setHz(const double val)
    {m_hz=val; m_angVel=val*2.*M_PI;m_chopChange=true;}
    void RunParam::setTjit(const double val)
    {m_tjit=val;m_chopChange=true;}
    void RunParam::setAs(const double val)
    {m_as=val;}
    void RunParam::setBs(const double val)
    {m_bs=val;}
    void RunParam::setCs(const double val)
    {m_cs=val;}
    void RunParam::setAa(const double val)
    {m_aa=val*M_PI/180.;}
    void RunParam::setBb(const double val)
    {m_bb=val*M_PI/180.;}
    void RunParam::setCc(const double val)
    {m_cc=val*M_PI/180.;}
    void RunParam::setUh(const double val)
    {m_uh=val;}
    void RunParam::setUk(const double val)
    {m_uk=val;}
    void RunParam::setUl(const double val)
    {m_ul=val;}
    void RunParam::setVh(const double val)
    {m_vh=val;}
    void RunParam::setVk(const double val)
    {m_vk=val;}
    void RunParam::setVl(const double val)
    {m_vl=val;}
    void RunParam::setOmega(const double val)
    {m_omega=val;}
    void RunParam::setGs(const double val)
    {m_gs=val;}
    void RunParam::setGl(const double val)
    {m_gl=val;}
    void RunParam::setDpsi(const double val)
    {m_dpsi=val;}
    void RunParam::setXh(const double val)
    {m_xh=val;}
    void RunParam::setXk(const double val)
    {m_xk=val;}
    void RunParam::setXl(const double val)
    {m_xl=val;}
    void RunParam::setYh(const double val)
    {m_yh=val;}
    void RunParam::setYk(const double val)
    {m_yk=val;}
    void RunParam::setYl(const double val)
    {m_yl=val;}
    void RunParam::setSx(const double val)
    {m_sx=val;}
    void RunParam::setSy(const double val)
    {m_sy=val;}
    void RunParam::setSz(const double val)
    {m_sz=val;}
    void RunParam::setIsam(const int val)
    {m_isam=val;}
    void RunParam::setTemp(const double val)
    {m_temp=val;}
    void RunParam::setEta(const double val)
    {
      m_eta=val;
      m_eta_sig = m_eta*(M_PI/180.)/(sqrt(log(256.0))); // Degrees FWHH -> std dev in radians
    }
    void RunParam::setRunLatticeMatrices( const boost::shared_ptr<Mantid::Geometry::OrientedLattice> lattice)
    {
      m_as=lattice->a1();
      m_bs=lattice->a2();
      m_cs=lattice->a3();
      m_aa=lattice->alpha1();
      m_bb=lattice->alpha2();
      m_cc=lattice->alpha3();
    }

    void RunParam::setTransforms()
    {
      // determine the transformation matrices for this run:
      // m_sampleLab - transform from sample coords to lab coords
      // m_uBinv     - transform from scattering plane to r.l.u.
      // yet to implement
    }

    // Set Tau for chopper according to model
    void RunParam::setTauChopperSignal()
    {
      m_tauChopperSignal = sqrt( tChopVariance() );
      m_tauChopperEffective = sqrt(6.)*m_tauChopperSignal; // FWHH of triangle with same variance as true distribution
    }

    double RunParam::getTauChopperSignal() const
    {
      return(m_tauChopperSignal);
    }

    // Aperture - select random point, assumes rectangle
    void RunParam::getAperturePoint( const double ran1, const double ran2, double & pW, double & pH) const
    {
      // default to rectangular shape
      pW = m_wa * (ran1 - 0.5);
      pH = m_ha * (ran2 - 0.5);
    }

    // select random point within sample, only cuboid model at present
    void RunParam::getSamplePoint(const double ranvar1, const double ranvar2, const double ranvar3, double & px, double & py, double & pz) const
    {
      if(m_isam == 1 )
      {
        // default shape is block
        px = m_sx * (ranvar1-0.5) ;
        py = m_sy * (ranvar2-0.5) ;
        pz = m_sz * (ranvar3-0.5) ;
      }
      else
      {
        px = 0.; py = 0.; pz = 0. ;
      }
    }

    // get Mosaic values of crystal assuming 2D Gaussian distribution
    void RunParam::getEta23( const double ranvar1, const double ranvar2, double & eta2, double & eta3) const
    {
       gasdev2d(ranvar1, ranvar2, eta2, eta3);
       eta2 = m_eta_sig * eta2;
       eta3 = m_eta_sig * eta3;
    }
    /*
     * Sample the the chopper time distribution
     */
    double RunParam::chopperTimeDist(const double ranvar) const
    {
      return ( m_dtChopEff * tridev(ranvar) );
    }
    /*
     * Get a value for the chopper jitter assuming a triangluar distribution
     */
    double RunParam::chopperJitter(const double ranvar) const
    {
      return ( m_tjitSig * sqrt(6.) * tridev(ranvar));
    }

    /**
    * tridev function from tobyfit - map a uniform random variable [0:1] to a triangular distribution [-1:1]
    * with peak at zero.
    */
    double RunParam::tridev(const double a) const
    {
        return( (a>0.5)? (1.0-sqrt(fabs(1.0-2.0*fabs(a-0.5)))):( -1.+sqrt( fabs(1.0-2.0*fabs(a-0.5) ) ) ) );
    }
    /**
    * gausdev function from tobyfit - take two uniform random values in [0:1] and map to two Gaussian distributed
    * values with mean zero and variance one.
    */
    void RunParam::gasdev2d(const double ran1, const double ran2, double & gaus1, double & gaus2) const
    {
         const double fac=sqrt(-2.*log(std::max(ran1,1e-20)));
         gaus1=fac*cos(2.*M_PI*ran2);
         gaus2=fac*sin(2.*M_PI*ran2);
    }

    /*
     * Use tchop to estimate chopper time variance
     */
    double RunParam::tChopVariance() const
    {
      return( tChop(m_pslit, m_radius, m_rho, m_angVel, m_ei));
    }

    /*
     *   Calculates the variance of the time pulse through a rotor at
     *  any energy. Answer in seconds**2
     *
     *    pSlit   slit thickness (m)
     *    radius  slit package diameter? (m)
     *    rho     slit radius of curvature (m)
     *    angVel  angular frequency of rotor (rad/sec)
     *    ei      energy the rotor has been phased for (meV)
     *    return tausqr:  variance of time width of rotor
     *
     *
     *  Exit
     *  ====
     *    p,R,rho,w,ei unchanged
     *    tausqr returned
     */
    double RunParam::tChop(const double pSlit, const double radius, const double rho, const double angVel, const double eI ) const
    {
      const double x=(pSlit/(2.0*radius*angVel));
      return( (x*x/6.0) * gsqrChop (pSlit, radius, rho, angVel, eI) );
    }

    /*
     * !
!   Calculates the function GSQR defined in eqn A.20 on p106 of T.G.Perring's
!  thesis:
!     variance_of_t = (DT**2)/6  * GSQR
!!
!    p       slit thickness (m)
!    R       slit package diameter (m)
!    rho     slit radius of curvature (m)
!    w       angular frequency of rotor (rad/sec)
!    ei      energy the rotor has been phased for (meV)
!.
!  Exit
!  ====
!    p,R,rho,w,ei unchanged

     */
    double RunParam::gsqrChop (const double pSlit, const double radius, const double rho, const double angVel, const double eI) const
    {

    if ((pSlit == 0.0) || (radius == 0.0) || (rho == 0.0) || (angVel == 0.0) || (eI == 0.0))
    {
      return(0.0);
      // TODO throw error
    }

   // Calculate parameter gam:

    const double vel=437.39158*sqrt(eI);
    const double gam=( 2.0*(radius*radius)/pSlit ) * fabs(1.0/rho - 2.0*angVel/vel);

   //  Find regime and calculate variance:

    if (gam >= 4.0)
    {
      return(0.);
      // TODO throw error
    }
    else
    {
      if (gam <= 1.0)
      {
        const double gam2=gam*gam; const double gam4=gam2*gam2;
        return( (1.0-gam4/10.0) / (1.0-gam2/6.0) );
      }
      else
      {
        const double groot=sqrt(gam);
        return( 0.6*gam*((groot-2.0)*(groot-2.0))*(groot+8.0)/(groot+4.0) );
      }
    }
    return(0.);
    }

    // Moderator related functions

    /*
    */
    double RunParam::getTauModeratorSignal() const
    {
       return( 1.e-6*sqrt(3.*m_s1*m_s1+m_s3*(2.-m_s3)*m_s2*m_s2) );
    }

    /*
    */
    double RunParam::getTauModeratorMean() const
    {
       return( 1.e-6*(3.*m_s1 + m_s3*m_s2) );
    }

    double RunParam::getTauModeratorAverageUs() const
    {
      return( 3.*m_s1+m_s3*m_s2);
    }

    /*
     * Simple energy resolution model only based on moderator and chopper
     * @param eps energy tramsfer (meV)
     * @param x2 sample - detector distance (m)
     * @return std deviation of energy resolution (meV)
     */
    double RunParam::energyResolutionModChop(const double eps, const double x2 ) const
    {
       if(m_ei<0. || m_ei<eps )
         throw std::runtime_error("Energy range problem in energyResolutionModChop");
       const double f = 1./2.0721418;
       const double wi = sqrt(m_ei*f);
       const double wf = sqrt((m_ei-eps)*f);
       const double veli = 629.62237*wi;
       const double tim = m_x0/veli;
       const double wf2wi3 = (wf/wi)*(wf/wi)*(wf/wi);
       const double tmp1 = (getTauModeratorSignal()/tim)*(1.+m_x1/x2*wf2wi3);
       const double tmp2 = (getTauChopperSignal()/tim)*(1.+(m_x0+m_x1)/x2*wf2wi3);
       const double tmp = tmp1*tmp1+tmp2*tmp2;
       return ( 2.*m_ei*sqrt(tmp) );
    }

    /*
     * This takes a random variable [0,1] and returns a value for the departure time from the moderator surface.
     * The Model used depends on internal settings, though only one currently implemented.
     */
    double RunParam::moderatorDepartTime(const double randomVar) const
    {
      if(m_modModel==1)
      {
        double xTemp = moderatorTimeLookUp(randomVar);
        if(xTemp>0.999) xTemp=0.999;
        return( 1.e-6*getTauModeratorAverageUs()*(xTemp/(1.-xTemp)-1.0));
      }
      return(0.);
    }

    /*
     * This should take a random variable [0:1] and use a table lookup to return the function value
     * At present it just calls the function which is way too expensive. TODO add lookup table
     */
    double RunParam::moderatorTimeLookUp( const double randomVar) const
    {
      return(areaToTIK(randomVar,m_s1,m_s2,m_s3)); // This is too expensive, need to do table look up as Tobyfit
    }
    /*
     * Returns the value of T such that the integral of a normalised Ikeda-Carpenter function, M(T), from 0 to T = area
     *  T is returned in reduced coordinates x such that:
     *  x = T/(T+<T>)  where <T> = 3*tauf + R*taus = mean of T for M(T)
     *  Note 0=< x =<1
     */
    double RunParam::areaToTIK(const double area, const double tauF, const double tauS, const double r) const
    {
      if(area<=0.)
        return(0.);
      if(area>=1.)
        return(1.);
      const double tol=1.e-10;
      m_tauF=tauF; m_tauS=tauS; m_r=r; m_offset=area; // pass data behind scenes
      return (zeroBrent(0.,1.,tol));
    }
    /**
     *  Calculates the area of the Ikeda-Carpenter moderator lineshape integrated from 0 to x.
     *  Fairly robust: it deals with special cases correctly e.g. tauf=0 and/or taus=0 etc.
     *  (the exception is for R=1 in the short time limit where the
     *  integral is O(x^4) but the routine calculates as the difference of two functions that are O(x^3)).
     *
     *  Assumes that tauf>=0, taus>=0, 0=<R=<1; if tauf and taus =/= 0 then tauf=<taus.
     *
     */
     double RunParam::areaIK(const double x, const double tauF, const double tauS, const double r) const
     {
       const double c3= 1.6666666666666666667e-01,    c4=-1.2500000000000000000e-01,   c5= 5.0000000000000000000e-02,
           c6=-1.3888888888888888889e-02,    c7= 2.9761904761904761905e-03,   c8=-5.2083333333333333333e-04,
           c9= 7.7160493827160493827e-05,   c10=-9.9206349206349206349e-06,  c11= 1.1273448773448773449e-06,
           c12=-1.1482216343327454439e-07,   c13= 1.0598968932302265636e-08;

       if(x>=0.)
       {
         if(tauF!=0.)
         {
           const double ax=x/tauF;
           double funAx, funGx;
           if(fabs(ax)<=0.1)
           {
             funAx = c3+ax*(c4+ax*(c5+ax*(c6+ax*(c7+ax*(c8+ax*(c9+ax*(c10+ax*(c11+ax*(c12+ax*c13)))))))));
           }
           else
             funAx = (1.0 - exp(-(ax))*(1.0+(ax)+0.5*(ax*ax))) / (ax*ax*ax);
           if(tauS!=0. && r!=0.)
           {
             const double gx=x*(1.0/tauF - 1.0/tauS);
             if(gx<0.1)
               funGx = c3+gx*(c4+gx*(c5+gx*(c6+gx*(c7+gx*(c8+gx*(c9+gx*(c10+gx*(c11+gx*(c12+gx*c13)))))))));
             else
               funGx = (1.0 - exp(-(gx))*(1.0+(gx)+0.5*(gx*gx))) / (gx*gx*gx);
             return((ax*ax*ax)*(funAx - r*funGx*exp(-(x/tauS))));
           }
           else
             return((ax*ax*ax)*funAx);
         }
         if(tauS!=0. && r!=0.)
           return((1.0-r) + r*(1.0-exp(-(x/tauS))));
         else
           return(1.);
       }
       return(0.);
     }

     /*
      * Objective function wrapper for zeroBrent which finds x such that this is zero
      */
     double RunParam::funAreaToTIK(const double x) const
     {
       if(x<=0.)
         return(-m_offset);
       if(x>=1.)
         return(1.-m_offset);
       const double t = (3.*m_tauF+m_r*m_tauS)*x/(1.-x);
       return(areaIK(t,m_tauF,m_tauS,m_r)-m_offset);
     }

     //
     double RunParam::zeroBrent ( const double a, const double b, const double t/*, double (*f) ( const double x ) const*/ ) const

     //****************************************************************************
     //
     //  Purpose:
     //
     //    zeroBrent seeks the root of a function F(X) in an interval [A,B].
     //
     //  Discussion:
     //
     //    The interval [A,B] must be a change of sign interval for F.
     //    That is, F(A) and F(B) must be of opposite signs.  Then
     //    assuming that F is continuous implies the existence of at least
     //    one value C between A and B for which F(C) = 0.
     //
     //    The location of the zero is determined to within an accuracy
     //    of 6 * MACHEPS * fabs ( C ) + 2 * T.
     //
     //  Licensing:
     //
     //    This code is distributed under the GNU LGPL license.
     //
     //  Modified:
     //
     //    13 April 2008
     //
     //  Author:
     //
     //    Original FORTRAN77 version by Richard Brent.
     //    C++ version by John Burkardt.
     //
     //  Reference:
     //
     //    Richard Brent,
     //    Algorithms for Minimization Without Derivatives,
     //    Dover, 2002,
     //    ISBN: 0-486-41998-3,
     //    LC: QA402.5.B74.
     //
     //  Parameters:
     //
     //    Input, double A, B, the endpoints of the change of sign interval.
     //
     //    Input, double T, a positive error tolerance.
     //
     //    Input, double F ( double X ), the name of a user-supplied function
     //    which evaluates the function whose zero is being sought.
     //
     //    Output, double ZERO, the estimated value of a zero of
     //    the function F.
     //
     {
       double c;
       double d;
       double e;
       double fa;
       double fb;
       double fc;
       double m;
       double macheps;
       double p;
       double q;
       double r;
       double s;
       double sa;
       double sb;
       double tol;
       //
       //  Make local copies of A and B.
       //
       sa = a;
       sb = b;
       fa =funAreaToTIK( sa );
       fb =funAreaToTIK( sb );

       c = sa;
       fc = fa;
       e = sb - sa;
       d = e;

       macheps = 1e-14; // more than sufficient for Tobyfit

       for (int i=0; true ;  i++)
       {
         if ( fabs ( fc ) < fabs ( fb ) )
         {
           sa = sb;
           sb = c;
           c = sa;
           fa = fb;
           fb = fc;
           fc = fa;
         }

         tol = 2.0 * macheps * fabs( sb ) + t;
         m = 0.5 * ( c - sb );

         if ( fabs ( m ) <= tol || fb == 0.0 )
         {
           break;
         }

         if ( fabs ( e ) < tol || fabs ( fa ) <= fabs ( fb ) )
         {
           e = m;
           d = e;
         }
         else
         {
           s = fb / fa;

           if ( sa == c )
           {
             p = 2.0 * m * s;
             q = 1.0 - s;
           }
           else
           {
             q = fa / fc;
             r = fb / fc;
             p = s * ( 2.0 * m * a * ( q - r ) - ( sb - sa ) * ( r - 1.0 ) );
             q = ( q - 1.0 ) * ( r - 1.0 ) * ( s - 1.0 );
           }

           if ( 0.0 < p )
           {
             q = - q;
           }
           else
           {
             p = - p;
           }

           s = e;
           e = d;

           if ( 2.0 * p < 3.0 * m * q - fabs ( tol * q ) &&
               p < fabs ( 0.5 * s * q ) )
           {
             d = p / q;
           }
           else
           {
             e = m;
             d = e;
           }
         }
         sa = sb;
         fa = fb;

         if ( tol < fabs ( d ) )
         {
           sb = sb + d;
         }
         else if ( 0.0 < m )
         {
           sb = sb + tol;
         }
         else
         {
           sb = sb - tol;
         }

         fb =funAreaToTIK( sb );

         if ( ( 0.0 < fb && 0.0 < fc ) || ( fb <= 0.0 && fc <= 0.0 ) )
         {
           c = sa;
           fc = fa;
           e = sb - sa;
           d = e;
         }
       }
       return sb;
     }

     //


     void RunParam::readData(const std::string file)
     {
       UNUSED_ARG(file);
     }
     void RunParam::writeData(const std::string file)
     {
       UNUSED_ARG(file);
     }

     /// @endcond
  }
}
