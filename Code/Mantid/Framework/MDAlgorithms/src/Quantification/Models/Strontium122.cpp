// Includes
#include "MantidMDAlgorithms/Quantification/Models/Strontium122.h"

#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/Math/Distributions/BoseEinsteinDistribution.h"

#include <cmath>

namespace Mantid
{
  namespace MDAlgorithms
  {
    DECLARE_FOREGROUNDMODEL(Strontium122);

    using Kernel::Math::BoseEinsteinDistribution;

    namespace // anonymous
    {
      /// Enumerate parameter positions
      enum { Seff = 0, J1a = 1, J1b = 2, J2 = 3, SJc = 4, GammaSlope = 5 };

      /// Number of parameters
      const unsigned int NPARAMS = 6;
      /// Parameter names, same order as above
      const char * PAR_NAMES[NPARAMS] = { "Seff", "J1a", "J1b", "J2", "SJc", "GammaSlope" };
      /// N attrs
      const unsigned int NATTS = 2;
      /// Attribute names
      const char * ATTR_NAMES[NATTS] = { "MultEps", "TwinType" };

      /// 2 \pi
      const double TWO_PI = 2.*M_PI;
    }

    /**
     * Initialize the model
     */
    void Strontium122::init()
    {
      // Default form factor. Can be overridden with the FormFactorIon attribute
      setFormFactorIon("Fe2"); 

      // Declare parameters that participate in fitting
      for(unsigned int i = 0; i < NPARAMS; ++i)
      {
        declareParameter(PAR_NAMES[i], 0.0);
      }

      // Declare fixed attributes
      for(unsigned int i = 0; i < NATTS; ++i)
      {
        declareAttribute(ATTR_NAMES[i], API::IFunction::Attribute(1));
      }
    }

    /**
     * Called when an attribute is set from the Fit string
     * @param name :: The name of the attribute
     * @param attr :: The value of the attribute
     */
    void Strontium122::setAttribute(const std::string & name, const API::IFunction::Attribute& attr)
    {
      if(name == ATTR_NAMES[0])
      {
        m_multEps = (attr.asInt() > 0);
      }
      else if(name == ATTR_NAMES[1])
      {
        m_twinType = attr.asInt();
      }
      else ForegroundModel::setAttribute(name, attr); // pass it on the base
    }

    /**
     * Calculates the scattering intensity
     * @param exptSetup :: Details of the current experiment
     * @param point :: The axis values for the current point in Q-W space: Qx, Qy, Qz, DeltaE. These contain the U matrix
     * rotation already.
     * @return The weight contributing from this point
     */
    double Strontium122::scatteringIntensity(const API::ExperimentInfo & exptSetup, const std::vector<double> & point) const
    {
      const double qx(point[0]), qy(point[1]), qz(point[2]), eps(point[3]);
      const double qsqr = qx*qx + qy*qy + qz*qz;
      const double epssqr = eps*eps;

      // Transform the HKL only requires B matrix & goniometer (R) as ConvertToMD should have already
      // handled addition of U matrix
      // qhkl = (1/2pi)(RB)^-1(qxyz)
      const Geometry::OrientedLattice & lattice = exptSetup.sample().getOrientedLattice();
      const Kernel::DblMatrix & gr = exptSetup.run().getGoniometerMatrix();
      const Kernel::DblMatrix & bmat = lattice.getUB();

      // Avoid doing inversion with Matrix class as it forces memory allocations
      // M^-1 = (1/|M|)*M^T
      double rb00(0.0), rb01(0.0), rb02(0.0),
             rb10(0.0), rb11(0.0), rb12(0.0),
             rb20(0.0), rb21(0.0), rb22(0.0);
      for(unsigned int i = 0; i < 3; ++i)
      {
        rb00 += gr[0][i]*bmat[i][0];
        rb01 += gr[0][i]*bmat[i][1];
        rb02 += gr[0][i]*bmat[i][2];

        rb10 += gr[1][i]*bmat[i][0];
        rb11 += gr[1][i]*bmat[i][1];
        rb12 += gr[1][i]*bmat[i][2];

        rb20 += gr[2][i]*bmat[i][0];
        rb21 += gr[2][i]*bmat[i][1];
        rb22 += gr[2][i]*bmat[i][2];
      }
      // 2pi*determinant. The tobyFit definition of rl vector has extra 2pi factor in it
      const double twoPiDet= TWO_PI*(rb00*(rb11*rb22 - rb12*rb21) -
                                     rb01*(rb10*rb22 - rb12*rb20) +
                                     rb02*(rb10*rb21 - rb11*rb20));

      const double qh = ((rb11*rb22 - rb12*rb21)*qx + (rb02*rb21 - rb01*rb22)*qy + (rb01*rb12 - rb02*rb11)*qz)/twoPiDet;
      const double qk = ((rb12*rb20 - rb10*rb22)*qx + (rb00*rb22 - rb02*rb20)*qy + (rb02*rb10 - rb00*rb12)*qz)/twoPiDet;
      const double ql = ((rb10*rb21 - rb11*rb20)*qx + (rb01*rb20 - rb00*rb21)*qy + (rb00*rb11 - rb01*rb10)*qz)/twoPiDet;

      // Lattice parameters
      double ca1 = std::cos(lattice.beta1());
      double ca2 = std::cos(lattice.beta2());
      double ca3 = std::cos(lattice.beta3());
      double sa1 = std::abs(std::sin(lattice.beta1()));
      double sa2 = std::abs(std::sin(lattice.beta2()));
      double sa3 = std::abs(std::sin(lattice.beta3()));

      const double factor = std::sqrt(1.0 + 2.0*(ca1*ca2*ca3) - (ca1*ca1 + ca2*ca2 + ca3*ca3));
      const double arlu1 = (TWO_PI/lattice.a())*(sa1/factor); // Lattice parameters in r.l.u
      const double arlu2 = (TWO_PI/lattice.b())*(sa2/factor);
      const double arlu3 = (TWO_PI/lattice.c())*(sa3/factor);

      const double tempInK = exptSetup.getLogAsSingleValue("temperature_log");
      const double boseFactor = BoseEinsteinDistribution::np1Eps(eps,tempInK);
      const double magFormFactorSqr = std::pow(formFactor(qsqr), 2);

      const double s_eff = getCurrentParameterValue(Seff);
      const double sj_1a = getCurrentParameterValue(J1a);
      const double sj_1b = getCurrentParameterValue(J1b);
      const double sj_2 = getCurrentParameterValue(J2);
      const double sj_c = getCurrentParameterValue(SJc);
      const double sjplus  = sj_1a + (2.0*sj_2);
      const double sk_ab = 0.5*(std::sqrt(std::pow(sjplus+sj_c, 2) + 10.5625) - (sjplus + sj_c));
      const double sk_c = sk_ab;
      const double gam = eps*getCurrentParameterValue(GammaSlope);

      // Avoid doing some multiplication twice
      const double fourGammaEpsSqr = 4.0*std::pow((gam*eps), 2);
      const double fourGammaBose = 4.0*gam*boseFactor;

      double weight(0.0);
      if(m_twinType >= 0) // add weight of notional crystal orientation
      {
        const double a_q = 2.0*( sj_1b*(std::cos(M_PI*qk)-1) + sj_1a + 2.0*sj_2 + sj_c ) + (3.0*sk_ab+sk_c);
        const double a_qsqr = a_q*a_q;
        const double d_q = 2.0*( sj_1a*std::cos(M_PI*qh) + 2.0*sj_2*std::cos(M_PI*qh)*std::cos(M_PI*qk) + sj_c*std::cos(M_PI*ql) );
        const double c_anis = sk_ab - sk_c;

        const double wdisp1sqr = std::abs(a_qsqr - std::pow(d_q+c_anis,2));
        const double wdisp1 = std::sqrt(wdisp1sqr);
        const double wdisp2sqr = std::abs(a_qsqr - std::pow(d_q-c_anis,2));
        const double wdisp2 = std::sqrt(wdisp2sqr);

        const double wt1 = fourGammaBose*wdisp1/(M_PI*(std::pow(epssqr - wdisp1sqr,2) + fourGammaEpsSqr));
        const double wt2 = fourGammaBose*wdisp2/(M_PI*(std::pow(epssqr - wdisp2sqr,2) + fourGammaEpsSqr));
        const double s_yy = s_eff*((a_q-d_q-c_anis)/wdisp1)*wt1;
        const double s_zz = s_eff*((a_q-d_q+c_anis)/wdisp2)*wt2;

        weight += 291.2 * magFormFactorSqr*((1.0 - std::pow(qk*arlu2, 2)/qsqr)*s_yy + (1.0 - std::pow(ql*arlu3,2)/qsqr)*s_zz);
      }
      if(m_twinType <= 0)
      {
        const double th =-qk*arlu2/arlu1;   // actual qk (rlu) in the twin
        const double tk = qh*arlu1/arlu2;   // actual qk (rlu) in the twin
        const double a_q = 2.0*( sj_1b*(std::cos(M_PI*tk)-1.0) + sj_1a + 2.0*sj_2 + sj_c ) + (3.0*sk_ab + sk_c);
        const double a_qsqr = a_q*a_q;
        const double d_q = 2.0*( sj_1a*std::cos(M_PI*th) + 2.0*sj_2*std::cos(M_PI*th)*std::cos(M_PI*tk) + sj_c*std::cos(M_PI*ql) );
        const double c_anis = sk_ab - sk_c;

        const double wdisp1sqr = std::abs(a_qsqr - std::pow(d_q+c_anis,2));
        const double wdisp1 = std::sqrt(wdisp1sqr);
        const double wdisp2sqr = std::abs(a_qsqr - std::pow(d_q-c_anis,2));
        const double wdisp2 = std::sqrt(wdisp2sqr);

        const double wt1 = fourGammaBose*wdisp1/(M_PI*(std::pow(epssqr - wdisp1sqr,2) + fourGammaEpsSqr));
        const double wt2 = fourGammaBose*wdisp2/(M_PI*(std::pow(epssqr - wdisp2sqr,2) + fourGammaEpsSqr));
        const double s_yy = s_eff*((a_q-d_q-c_anis)/wdisp1)*wt1;
        const double s_zz = s_eff*((a_q-d_q+c_anis)/wdisp2)*wt2;

        weight += 291.2 * magFormFactorSqr*((1.0 - std::pow(tk*arlu2,2)/qsqr)*s_yy + (1.0 - std::pow(ql*arlu3, 2)/qsqr)*s_zz);
      }

      if(m_multEps)
      {
        weight *= eps;
      }
      return weight;
    }
  }
}
