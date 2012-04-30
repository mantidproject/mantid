//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidMDAlgorithms/DampedHisenbergFMSW.h"

#include <math.h>

#include "MantidKernel/Tolerance.h"
#include "MantidGeometry/Math/mathSupport.h"
#include "MantidKernel/Matrix.h"

namespace Mantid
{
  namespace MDAlgorithms
  {

    using namespace Mantid::Kernel;
    using namespace Mantid::API;

    DECLARE_FUNCTION(DampedHisenbergFMSW)

    DampedHisenbergFMSW::DampedHisenbergFMSW()
    {
      declareParameter("Amplitude", 0.0);
      declareParameter("Gap", 0.0);
      declareParameter("JS1", 0.0);
      declareParameter("JS2", 0.0);
      declareParameter("JS3", 0.0);
      declareParameter("Gamma", 0.0);
      m_lovesey = false; // these false correspond to TF 111 model
      m_lorentzian = false;
    }

    DampedHisenbergFMSW::DampedHisenbergFMSW(std::vector<std::string> extraParams)
    {
      declareParameter("Amplitude", 0.0);
      declareParameter("Gap", 0.0);
      declareParameter("JS1", 0.0);
      declareParameter("JS2", 0.0);
      declareParameter("JS3", 0.0);
      declareParameter("Gamma", 0.0);
      for(size_t i=0;i<extraParams.size();i++)
        declareParameter(extraParams[i],0.0);
      m_lovesey = false;
      m_lorentzian = false;
    }

    //

    // This is a broad model
    bool DampedHisenbergFMSW::userModelIsBroad() const
    {return true;};
    //
    /**
     * sqw_broad model 111/112/121/122 from tobyfit
     */
    void DampedHisenbergFMSW::userSqw(const boost::shared_ptr<Mantid::MDAlgorithms::RunParam> run, const std::vector<double> & params,
        const std::vector<double> & qE, std::vector<double> & result) const
    {

      const double eps = qE[3];
      Mantid::Kernel::V3D qlab=Mantid::Kernel::V3D(qE[0],qE[1],qE[2]);
      const double qsqr = qlab.norm2();
      const double temp=run->getTemp();
      // Get Q in r.l.u. using the uBInv matrix for this run:
      //Mantid::Kernel::V3D qrlu = m_runData->uBInv()*qlab;
      const Mantid::Kernel::V3D qhkl=run->getCubInvMat()*qlab;
      const double qh = qhkl[0]; //
      const double qk = qhkl[1];
      const double ql = qhkl[2];
      const double t2mev = 1.0/11.6045;

      const double amp = params[0];
      const double gap = params[1];
      const double js1 = params[2];
      const double js2 = params[3];
      const double js3 = params[4];
      const double gam = params[5];
      double spin, dampingScale;
      if(m_lovesey )
      {
        spin = params[6];
        dampingScale = params[7];
      }
      //    Heisenberg dispersion relation up to 3rd nearest neighbours:
      const double sxsqr = pow(sin(M_PI*qh),2);
      const double sysqr = pow(sin(M_PI*qk),2);
      const double szsqr = pow(sin(M_PI*ql),2);
      const double wdisp = gap
          +  4.0*js1* (sxsqr+sysqr+szsqr)
          + 16.0*js2*((sxsqr+sysqr+szsqr)-(sxsqr*sysqr+sysqr*szsqr+szsqr*sxsqr))
          + 16.0*js3*((sxsqr+sysqr+szsqr)-2.0*(sxsqr*sysqr+sysqr*szsqr+szsqr*sxsqr)
              +  4.0*sxsqr*sysqr*szsqr);
      //    Calculate damping term:
      double gamma=0.;
      if ( ! m_lovesey )
        gamma = gam;
      else
        gamma = dampingScale*gam * js1*gamFM ((wdisp-gap)/js1, t2mev*temp/js1, spin);

      //    Calculate the weight:
      double weight;
      if ( ! m_lorentzian )
        weight = amp * bose(eps,temp) * pow(magneticForm(qsqr),2) *
        (4.0*gamma*wdisp)/(M_PI*(pow(pow(eps,2)-pow(wdisp,2),2)+4.0*pow(gamma*eps,2))) ;
      else
        weight = amp * bose(eps,temp) * pow((magneticForm(qsqr)),2) *
        (gamma/(M_PI*eps*(pow(eps-wdisp,2)+pow(gamma,2))));

      // a vector of length one is returned for broad models - two values are returned for sharp models
      if(result.size()==0)
        result.push_back(weight);
      else
        result.at(0)=weight;
    }
    // read parameter values - don't call every time
    void DampedHisenbergFMSW::getParams(std::vector<double> & params) const
    {
      params.push_back(getParameter("Amplitude"));
      params.push_back(getParameter("Gap"));
      params.push_back(getParameter("JS1"));
      params.push_back(getParameter("JS2"));
      params.push_back(getParameter("JS3"));
      params.push_back(getParameter("Gamma"));
    }

    double DampedHisenbergFMSW::gamFM (const double e, const double t, const double s) const
    {
      //
      // Calculates the damping for a classical Heisenberg FM interpolating between eqns 9.89 and 9.90
      // in Lovesey vol.II to obtain an expression covering all temperatures in the long wavelength limit.
      // See notes on GMR data analysis.
      //
      //     e  hbar.w / 2JS
      //     t  kT / 2JS
      //     s  spin (0.5, 1, 1.5, 2...)
      //
      //     gamFM   gam / 2JS
      //
      double zi,gamFm;
      const double clo=8.0*2.153733998748905e-4, chi=8.0*1.497758900089e-4, zi0=5.009958e-2;

      zi = e/t;
      if (fabs(zi) > zi0)
        gamFm = (chi/pow(s,2)) * pow(t,4) * sqrt(pow(fabs(zi),3));
      else
        gamFm = (clo/pow(s,2)) * pow(t,4) * (pow(zi,2))*( pow(log(fabs(zi)),2)/6.0 - log(fabs(zi))/1.8 - 0.05 );

      return gamFm;
    }

  }
}

