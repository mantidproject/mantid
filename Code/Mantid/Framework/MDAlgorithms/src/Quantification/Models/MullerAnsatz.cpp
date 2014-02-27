// Includes
#include "MantidMDAlgorithms/Quantification/Models/MullerAnsatz.h"

#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/Math/Distributions/BoseEinsteinDistribution.h"

#include <cmath>

namespace Mantid
{
  namespace MDAlgorithms
  {
    DECLARE_FOREGROUNDMODEL(MullerAnsatz);

    using Kernel::Math::BoseEinsteinDistribution;

    struct AnsatzParameters
    {
/*
    p(1)	A, the intensity scale factor in the Mueller Ansatz formalism.
!		p(2)	J (maximum of lower bound is at pi*J/2)
!
!		p(19) = 1,2,3 for chains along a*, b*, c* respectively 
!				Default is along c* if any other value given
!
!		p(20) = 0		isotropic magnetic form factor
!			  = 1,2,3	"dx2-y2" form factor, normal to plane of orbital being a*, b*, c* respectively
!				Default is isotropic form factor

*/
      /// Enumerate parameter positions
      enum {Ampliture,J_bound};

      /// Number of parameters
      enum{NPARAMS = 2};
      /// Parameter names, same order as above
      static char * PAR_NAMES[NPARAMS];
      /// N attrs
      enum {NATTS = 3};
      /// Attribute names
      static char * ATTR_NAMES[NATTS];

      /// 2 \pi

    };

    char* AnsatzParameters::PAR_NAMES[AnsatzParameters::NPARAMS] = {"Amplitude","J_boundLimit"};
    char* AnsatzParameters::ATTR_NAMES[AnsatzParameters::NATTS] = {"IonName","ChainDirection","MagneticFFDirection"};
    //static 
    /**
     * Initialize the model
     */
    void MullerAnsatz::init()
    {
      // Default form factor. Can be overridden with the FormFactorIon attribute
      //setFormFactorIon("Fe2"); 

      // Declare parameters that participate in fitting
      for(unsigned int i = 0; i < AnsatzParameters::NPARAMS; ++i)
      {
        declareParameter(AnsatzParameters::PAR_NAMES[i], 0.0);
      }

      // Declare fixed attributes
      declareAttribute(AnsatzParameters::ATTR_NAMES[0], API::IFunction::Attribute("Fe"));
      for(unsigned int i = 1; i < AnsatzParameters::NATTS; ++i)
      {
        declareAttribute(AnsatzParameters::ATTR_NAMES[i], API::IFunction::Attribute(int(1)));
      }
    }

    /**
     * Called when an attribute is set from the Fit string
     * @param name :: The name of the attribute
     * @param attr :: The value of the attribute
     */
    void MullerAnsatz::setAttribute(const std::string & name, const API::IFunction::Attribute& attr)
    {

      if(name == AnsatzParameters::ATTR_NAMES[1])
      {
        m_ChainDirection = ChainDirection(attr.asInt());
      }
      else if(name == AnsatzParameters::ATTR_NAMES[2])
      {
        m_FFDirection = MagneticFFDirection(attr.asInt());
      }
     else 
      ForegroundModel::setAttribute(name, attr); // pass it on the base
    }

    static const double TWO_PI = 2.*M_PI;
    static const double PI_BY2 = 0.5*M_PI;
    /**
     * Calculates the scattering intensity
     * @param exptSetup :: Details of the current experiment
     * @param point :: The axis values for the current point in Q-W space: Qx, Qy, Qz, DeltaE. These contain the U matrix
     * rotation already.
     * @return The weight contributing from this point
     */
    double MullerAnsatz::scatteringIntensity(const API::ExperimentInfo & exptSetup, const std::vector<double> & point) const
    {

      const double qx(point[0]), qy(point[1]), qz(point[2]), eps(point[3]);
      const double qsqr = qx*qx + qy*qy + qz*qz;
      const double Amplitude = getCurrentParameterValue(AnsatzParameters::Ampliture);
      const double J_bound = getCurrentParameterValue(AnsatzParameters::J_bound);

    //  const double epssqr = eps*eps;

    //  // Transform the HKL only requires B matrix & goniometer (R) as ConvertToMD should have already
    //  // handled addition of U matrix
    //  // qhkl = (1/2pi)(RB)^-1(qxyz)
      const Geometry::OrientedLattice & lattice = exptSetup.sample().getOrientedLattice();
      const Kernel::DblMatrix & gr = exptSetup.run().getGoniometerMatrix();
      const Kernel::DblMatrix & bmat = lattice.getB();

      double weight,qchain,cos_beta_sqr;
      double qh,qk,ql;

      ForegroundModel::convertToHKL(exptSetup,qx,qy,qz,qh,qk,ql);

      //	Orientation of the chain:
      switch(m_ChainDirection)
      {
      case(Along_a):
        {
          qchain = qh;
          break;
        }
      case(Along_b):
        {
          qchain = qk;
          break;
        }
      case(Along_c):
        {
          qchain = qk;
          break;
        }
      default:
        qchain = ql;
      }
      double wl = PI_BY2*J_bound*std::fabs(sin(TWO_PI*qchain));
      double wu = M_PI*J_bound*std::fabs(sin(M_PI*qchain));
      if (eps > (wl+FLT_EPSILON) && eps <= wu)
      {
    //	Orientation of the hole orbital
        double cos_beta_sqr;
        switch(m_FFDirection)
        {
        case(NormalTo_a):
          { 
            //cos_beta_sqr = (qh*arlu(1))**2 / qsqr;
            break;
          }
        case(NormalTo_b):
          {
            //cos_beta_sqr = (qk*arlu(2))**2 / qsqr;
            break;
          }
        case(NormalTo_c):
          {
           // cos_beta_sqr = ql*arlu(3))**2 / qsqr;
            break;
          }

        case(Isotropic): 
        default:
          //weight = Amplitude*(sigma_mag/pi)* (bose(eps,temp)/eps) * (form_table(qsqr))**2 / sqrt((eps-wl)*(eps+wl));
          return weight;
        }

        //weight = Amplitude*(sigma_mag/pi)* (bose(eps,temp)/eps) *
        //                         (amff_cu3d(qsqr,cos_beta_sqr))**2 /  sqrt((eps-wl)*(eps+wl));
        return weight;
      }
      else
        return 0;

/*



    iorient = nint(p(20))
    qsqr = qx**2 + qy**2 + qz**2
    if (iorient .eq. 1) then
      cos_beta_sqr = (qh*arlu(1))**2 / qsqr
    elseif (iorient .eq. 2) then
      cos_beta_sqr = (qk*arlu(2))**2 / qsqr
    elseif (iorient .eq. 3) then
      cos_beta_sqr = (ql*arlu(3))**2 / qsqr
    endif
          
!	Get spectral weight
    wl = piby2*p(2)*abs(sin(twopi*qchain))
    wu = pi*p(2)*abs(sin(pi*qchain))
    if (eps .gt. (wl+small) .and. eps .le. wu) then
              if (iorient .ge. 1 .and. iorient .le. 3) then
                  weight = p(1)*(sigma_mag/pi)* (bose(eps,temp)/eps) *
     &                                   (amff_cu3d(qsqr,cos_beta_sqr))**2 / sqrt(eps**2-wl**2)
              else
        weight = p(1)*(sigma_mag/pi)* (bose(eps,temp)/eps) * (form_table(qsqr))**2 / sqrt(eps**2-wl**2)
              endif  
    else
      weight = 0.0d0
    endif
*/

  

      return weight;
    }
  }
}
