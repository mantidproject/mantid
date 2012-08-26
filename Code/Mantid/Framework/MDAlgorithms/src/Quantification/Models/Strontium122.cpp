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
    DECLARE_FOREGROUNDMODEL(Strontium122);

    using PhysicalConstants::MagneticIon;
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
     *  Declare fixed attributes
     */
    void Strontium122::declareAttributes()
    {
      for(unsigned int i = 0; i < NATTS; ++i)
      {
        declareAttribute(ATTR_NAMES[i], API::IFunction::Attribute(1));
      }
    }

    /**
     * Called when an attribute is set
     * @param name :: The name of the attribute
     * @param attr :: The value of the attribute
     */
    void Strontium122::setAttribute(const std::string & name, const API::IFunction::Attribute& attr)
    {
      int asInt = attr.asInt();
      if(name == ATTR_NAMES[0])
      {
        m_multEps = (asInt > 0);
      }
      else if(name == ATTR_NAMES[1])
      {
        m_twinType = asInt;
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

      const Geometry::OrientedLattice & lattice = exptSetup.sample().getOrientedLattice();
      const Kernel::DblMatrix & gr = exptSetup.run().getGoniometerMatrix();
      const Kernel::DblMatrix & ub = lattice.getUB();

      double ub00(0.0), ub01(0.0), ub02(0.0),
             ub10(0.0), ub11(0.0), ub12(0.0),
             ub20(0.0), ub21(0.0), ub22(0.0);
      for(unsigned int i = 0; i < 3; ++i)
      {
        ub00 += gr[0][i]*ub[i][0];
        ub01 += gr[0][i]*ub[i][1];
        ub02 += gr[0][i]*ub[i][2];

        ub10 += gr[1][i]*ub[i][0];
        ub11 += gr[1][i]*ub[i][1];
        ub12 += gr[1][i]*ub[i][2];

        ub20 += gr[2][i]*ub[i][0];
        ub21 += gr[2][i]*ub[i][1];
        ub22 += gr[2][i]*ub[i][2];
      }
      static const double twoPi = 2.*M_PI;
      // 2pi*determinant. The tobyFit definition of rl vector has extra 2pi factor in it
      const double twoPiDet= twoPi*(ub00*(ub11*ub22 - ub12*ub21) -
                                    ub01*(ub10*ub22 - ub12*ub20) +
                                    ub02*(ub10*ub21 - ub11*ub20));

      double qh = ((ub11*ub22 - ub12*ub21)*qx + (ub02*ub21 - ub01*ub22)*qy + (ub01*ub12 - ub02*ub11)*qz)/twoPiDet;
      double qk = ((ub12*ub20 - ub10*ub22)*qx + (ub00*ub22 - ub02*ub20)*qy + (ub02*ub10 - ub00*ub12)*qz)/twoPiDet;
      double ql = ((ub10*ub21 - ub11*ub20)*qx + (ub01*ub20 - ub00*ub21)*qy + (ub00*ub11 - ub01*ub10)*qz)/twoPiDet;

      double astar = twoPi*lattice.b1(); //arlu(1)
      double bstar = twoPi*lattice.b2(); //arlu(2)
      double cstar = twoPi*lattice.b3(); //arlu(3)
      
      // Transform from Mantidto TF coordinates
      std::swap(qh,ql);
      std::swap(astar,cstar);
      std::swap(qk,ql);
      std::swap(bstar,cstar);
      
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

        const double fourGammaEpsSqr = 4.0*std::pow((gam*eps), 2);
        const double wt1 = boseFactor*(4.0*gam*wdisp1)/(M_PI*(std::pow(epssqr - wdisp1sqr,2) + fourGammaEpsSqr));
        const double wt2 = boseFactor*(4.0*gam*wdisp2)/(M_PI*(std::pow(epssqr - wdisp2sqr,2) + fourGammaEpsSqr));
        const double s_yy = s_eff*((a_q-d_q-c_anis)/wdisp1)*wt1;
        const double s_zz = s_eff*((a_q-d_q+c_anis)/wdisp2)*wt2;

        weight += 291.2 * magFormFactorSqr*((1.0 - std::pow(qk*bstar, 2)/qsqr)*s_yy + (1.0 - std::pow(ql*cstar,2)/qsqr)*s_zz);
      }
      if(m_twinType <= 0)
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

      if(m_multEps)
      {
        weight *= eps;
      }

      return weight;
    }
  }
}
