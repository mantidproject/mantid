// Includes
#include "MantidMDAlgorithms/Quantification/Models/Strontium122.h"

#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/Math/Distributions/BoseEinsteinDistribution.h"
#include "MantidKernel/MagneticIon.h"
#include "MantidKernel/PhysicalConstants.h"

#include <cmath>

namespace Mantid
{
  namespace MDAlgorithms
  {
    using PhysicalConstants::MagneticIon;
    using Kernel::Math::BoseEinsteinDistribution;

    namespace // anonymous
    {
      /// Enumerate parameter positions
      enum { Seff = 0, J1a = 1, J1b = 2, J2 = 3, SJc = 4, GammaSlope = 5, MultEps = 6, TwinType = 7};
      /// Number of parameters
      const unsigned int NPARAMS = 8;
      /// Parameter names, same order as above
      const char * PAR_NAMES[8] = { "Seff", "J1a", "J1b", "J2", "SJc", "GammaSlope", "MultEps", "TwinType" };
    }

    Strontium122::Strontium122()
      : m_formFactorTable(500, PhysicalConstants::getMagneticIon("Fe", 2), /*J*/0, /*L*/0)
    {}

    /**
     * Declares the parameters that should participate in fitting
     */
    void Strontium122::declareParameters()
    {
      for(unsigned int i = 0; i < NPARAMS; ++i)
      {
        declareParameter(PAR_NAMES[i], 0.0);
      }
    }

    /**
     * Calculates the scattering intensity
     * @param exptSetup :: Details of the current experiment
     * @param point :: The axis values for the current point in Q-W space: Qx, Qy, Qz, DeltaE
     * @return The weight contributing from this point
     */
    double Strontium122::scatteringIntensity(const API::ExperimentInfo & exptSetup, const std::vector<double> & point) const
    {
      const double qx(point[0]), qy(point[1]), qz(point[2]), eps(point[3]);
      const double qsqr = qx*qx + qy*qy + qz*qz;
      const double epssqr = eps*eps;

      const Kernel::V3D qLab(qy,qz,qx); // TobyFit has axis beam=x,up=z. Mantid has beam=z,up=y
      const Geometry::OrientedLattice & latticeRot = exptSetup.sample().getOrientedLattice();
      const double twoPi = 2.*M_PI;

      const Kernel::V3D qHKL = latticeRot.hklFromQ(qLab);
      // Mantid definition doesn't contain 2*pi in definition of reciprocal lattice parameters
      const double qh(qHKL[0]/twoPi), qk(qHKL[1]/twoPi), ql(qHKL[2]/twoPi);
      const double astar = twoPi*latticeRot.b1(); //arlu(1)
      const double bstar = twoPi*latticeRot.b2(); //arlu(2)
      const double cstar = twoPi*latticeRot.b3(); //arlu(3)

      const double tempInK = exptSetup.getLogAsSingleValue("temperature_log");
      const double boseFactor = BoseEinsteinDistribution::np1Eps(eps,tempInK);
      const double magFormFactorSqr = std::pow(m_formFactorTable.value(qsqr), 2);

      const double s_eff = getCurrentParameterValue(Seff);
      const double sj_1a = getCurrentParameterValue(J1a);
      const double sj_1b = getCurrentParameterValue(J1b);
      const double sj_2 = getCurrentParameterValue(J2);
      const double sj_c = getCurrentParameterValue(SJc);
      const double sjplus  = sj_1a + (2.0*sj_2);
      const double sk_ab = 0.5*(std::sqrt(std::pow(sjplus+sj_c, 2) + 10.5625) - (sjplus + sj_c));
      const double sk_c = sk_ab;
      const double gam = eps*getCurrentParameterValue(GammaSlope);
      const double twinType = getCurrentParameterValue(TwinType);
      const bool multEps = (getCurrentParameterValue(MultEps) > 0.0);

      double weight(0.0);
      if (twinType > -0.5) // add weight of notional crystal orientation
      {
        const double a_q = 2.0*( sj_1b*(std::cos(M_PI*qk)-1) + sj_1a + 2.0*sj_2 + sj_c ) + (3.0*sk_ab+sk_c);
        const double a_qsqr = a_q*a_q;
        const double d_q = 2.0*( sj_1a*std::cos(M_PI*qh) + 2.0*sj_2*std::cos(M_PI*qh)*std::cos(M_PI*qk) + sj_c*std::cos(M_PI*ql) );
        const double c_anis = sk_ab - sk_c;

        const double wdisp1sqr = std::abs(a_qsqr - std::pow(d_q+c_anis,2));
        const double wdisp1 = std::sqrt(wdisp1sqr);
        const double wdisp2sqr = std::abs(a_qsqr - std::pow(d_q-c_anis,2));
        const double wdisp2 = std::sqrt(wdisp2sqr);

        const double fourGammaEpsSqr = 4.0*std::pow((gam*eps), 2);
        const double wt1 = boseFactor*(4.0*gam*wdisp1)/(M_PI*(std::pow(epssqr - wdisp1sqr,2) + fourGammaEpsSqr));
        const double wt2 = boseFactor*(4.0*gam*wdisp2)/(M_PI*(std::pow(epssqr - wdisp2sqr,2) + fourGammaEpsSqr));
        const double s_yy = s_eff*((a_q-d_q-c_anis)/wdisp1)*wt1;
        const double s_zz = s_eff*((a_q-d_q+c_anis)/wdisp2)*wt2;

        weight += 291.2 * magFormFactorSqr*((1.0 - std::pow(qk*bstar, 2)/qsqr)*s_yy + (1.0 - std::pow(ql*cstar,2)/qsqr)*s_zz);
      }
      if(twinType < 0.5)
      {
        const double th =-qk*bstar/astar;   // actual qk (rlu) in the twin
        const double tk = qh*astar/bstar;   // actual qk (rlu) in the twin
        const double a_q = 2.0*( sj_1b*(std::cos(M_PI*tk)-1.0) + sj_1a + 2.0*sj_2 + sj_c ) + (3.0*sk_ab + sk_c);
        const double a_qsqr = a_q*a_q;
        const double d_q = 2.0*( sj_1a*std::cos(M_PI*th) + 2.0*sj_2*std::cos(M_PI*th)*std::cos(M_PI*tk) + sj_c*std::cos(M_PI*ql) );
        const double c_anis = sk_ab - sk_c;

        const double wdisp1sqr = std::abs(a_qsqr - std::pow(d_q+c_anis,2));
        const double wdisp1 = std::sqrt(wdisp1sqr);
        const double wdisp2sqr = std::abs(a_qsqr - std::pow(d_q-c_anis,2));
        const double wdisp2 = std::sqrt(wdisp2sqr);

        const double fourGammaEpsSqr = 4.0*std::pow((gam*eps), 2);
        const double wt1 = boseFactor*(4.0*gam*wdisp1)/(M_PI*(std::pow(epssqr - wdisp1sqr,2) + fourGammaEpsSqr));
        const double wt2 = boseFactor*(4.0*gam*wdisp2)/(M_PI*(std::pow(epssqr - wdisp2sqr,2) + fourGammaEpsSqr));
        const double s_yy = s_eff*((a_q-d_q-c_anis)/wdisp1)*wt1;
        const double s_zz = s_eff*((a_q-d_q+c_anis)/wdisp2)*wt2;

        weight += 291.2 * magFormFactorSqr*((1.0 - std::pow(tk*bstar,2)/qsqr)*s_yy + (1.0 - std::pow(ql*cstar, 2)/qsqr)*s_zz);
      }

      if(multEps)
      {
        weight *= eps;
      }

      return weight;
    }
  }
}
