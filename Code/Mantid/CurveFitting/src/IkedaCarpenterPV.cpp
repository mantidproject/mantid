//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/IkedaCarpenterPV.h"
#include <gsl/gsl_sf_erf.h>
#include <gsl/gsl_multifit_nlin.h>
#include "MantidCurveFitting/SpecialFunctionSupport.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/IInstrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include <cmath>


namespace Mantid
{
namespace CurveFitting
{

using namespace Kernel;
using namespace SpecialFunctionSupport;

DECLARE_FUNCTION(IkedaCarpenterPV)

// Get a reference to the logger
Kernel::Logger& IkedaCarpenterPV::g_log = Kernel::Logger::get("IkedaCarpenterPV");

double IkedaCarpenterPV::centre()const 
{
  return getParameter("X0");
}

void IkedaCarpenterPV::setHeight(const double h) 
{


//  m_height = h;

  setParameter("I",1);
  double h0 = height();

  //double oldIntensity = getParameter("I");

  setParameter("I", h/h0);

  /*if (m_width != 1.0)
    setParameter("I",h);
  else
    setParameter("I",0.5*m_width*m_height);*/
};


double IkedaCarpenterPV::height()const 
{
  double h0;
 
  double toCentre = centre();
  
  constFunction(&h0, &toCentre, 1);

  std::cerr<<"height = "<<h0<<"\n";
  return h0;

 /* if (m_width != 1.0)
    return getParameter("I");
  else
    return getParameter("I")/(0.5*m_width);*/

};

double IkedaCarpenterPV::width()const 
{
  // here estimated to sum of the HWHM of the gaussian and lorentzian part...
  //double eta = getParameter("Eta");

  // eta should always be between zero and one but before putting in codes to
  // constrain this do the following

  //if (eta >= 0 && eta <= 1.0)
  //  return 0.5*(eta*getParameter("Gamma")+(1-eta)*sqrt(getParameter("SigmaSquared"))*2);
  //else
  //  return 0.5*(getParameter("Gamma")+sqrt(getParameter("SigmaSquared"))*2);

  return sqrt(getParameter("SigmaSquared"))*2;
};

void IkedaCarpenterPV::setWidth(const double w) 
{
  setParameter("SigmaSquared", w*w/4);

  //if ( m_height != 0.0 )
    //setParameter("I",getParameter("I")*w/m_width);
    //m_width = w;
};

void IkedaCarpenterPV::setCentre(const double c) 
{
  setParameter("X0",c);
};


void IkedaCarpenterPV::init()
{
  declareParameter("I", 0.0);
  declareParameter("Alpha0",1.6);
  declareParameter("Alpha1",1.5);
  declareParameter("Beta0",31.9);
  declareParameter("Kappa",46.0);
  declareParameter("SigmaSquared",1.0);
  declareParameter("Gamma",1.0);
  declareParameter("Eta",0.0);
  declareParameter("X0",0.0);

  m_width = 0.0; m_height = 0.0; 
}


/** Method for updating m_waveLength, although don't do this if m_waveLengthFixed = true.
 *  Also if size of m_waveLength is equal to number of data (for a new instance of this 
 *  class this vector is empty initially) then don't recalculate it.
 *
 *  @param xValues x values
 *  @param nData length of xValues
 */
void IkedaCarpenterPV::calWavelengthAtEachDataPoint(const double* xValues, const int& nData) const
{
  if (!m_waveLengthFixed)
  { 
    // if wavelength vector already have the right size no need for resizing it
    // further we make the assumption that no need to recalculate this vector if
    // it already has the right size

    if (m_waveLength.size() != nData)
    {
      // This peak shape requires the fit to be done in TOF units

      if ( m_workspace->getAxis(0)->unit()->unitID().compare("TOF") != 0 )
      {
        g_log.warning() << "IkedaCarpenterPV function should ONLY be used when working with x-axis unit = TOF\n";
      }

      m_waveLength.resize(nData);

      // Get the geometric information for this detector
      
      API::IInstrument_const_sptr instrument = m_workspace->getInstrument();
      Geometry::IObjComponent_const_sptr sample = instrument->getSample();
      const double l1 = instrument->getSource()->getDistance(*sample);
      Geometry::IDetector_sptr det = m_workspace->getDetector(m_workspaceIndex);  // i is the workspace index
      const double l2 = det->getDistance(*sample);
      const double twoTheta = m_workspace->detectorTwoTheta(det);
     
      Mantid::Kernel::Unit_const_sptr wavelength = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");
      for (int i = 0; i < nData; i++)
      {
        m_waveLength[i] = xValues[i];
      }
      std::vector<double> y; // Create an empty vector, it's not used in fromTOF
      wavelength->fromTOF(m_waveLength,y,l1,l2,twoTheta,0,0.0,0.0);
    }
  }
}

void IkedaCarpenterPV::constFunction(double* out, const double* xValues, const int& nData) const
{
    const double& I = getParameter("I");
    const double& alpha0 =getParameter("Alpha0");
    const double& alpha1 = getParameter("Alpha1");
    const double& beta0 = getParameter("Beta0");
    const double& kappa = getParameter("Kappa");
    const double& sigmaSquared = getParameter("SigmaSquared");
    const double& gamma = getParameter("Gamma");
    const double& eta = getParameter("Eta");
    const double& X0 = getParameter("X0");

    const double beta = 1/beta0;

    // equations taken from Fullprof manual

    const double k = 0.05;   

    double u,v,s,r;
    double yu, yv, ys, yr;

    const double someConst = 1/sqrt(2.0*sigmaSquared);

    double R, Nu, Nv, Ns, Nr, N;

    std::complex<double> zs, zu, zv, zr;

    double alpha, a_minus, a_plus, x, y, z;

    // update wavelength vector
    calWavelengthAtEachDataPoint(xValues, nData);

    for (int i = 0; i < nData; i++) {
        double diff=xValues[i]-X0;

        if (m_waveLengthFixed)
        {
          // this is to allow unit testing when a workspace is not available
          R = exp(-81.799/(m_waveLength[0]*m_waveLength[0]*kappa));
          alpha = 1.0 / (alpha0+m_waveLength[0]*alpha1);
        }
        else
        {
          R = exp(-81.799/(m_waveLength[i]*m_waveLength[i]*kappa));
          alpha = 1.0 / (alpha0+m_waveLength[i]*alpha1);
        }

        a_minus = alpha*(1-k);
        a_plus = alpha*(1+k);
        x=a_minus-beta;
        y=alpha-beta;
        z=a_plus-beta; 

        Nu=1-R*a_minus/x;
        Nv=1-R*a_plus/z;
        Ns=-2*(1-R*alpha/y);
        Nr=2*R*alpha*alpha*beta*k*k/(x*y*z);

        u=a_minus*(a_minus*sigmaSquared-2*diff)/2.0;
        v=a_plus*(a_plus*sigmaSquared-2*diff)/2.0;
        s=alpha*(alpha*sigmaSquared-2*diff)/2.0;
        r=beta*(beta*sigmaSquared-2*diff)/2.0;

        yu = (a_minus*sigmaSquared-diff)*someConst;
        yv = (a_plus*sigmaSquared-diff)*someConst;
        ys = (alpha*sigmaSquared-diff)*someConst;
        yr = (beta*sigmaSquared-diff)*someConst;

        zs = std::complex<double>(-alpha*diff, 0.5*alpha*gamma);
        zu = (1-k)*zs;
        zv = (1-k)*zs;
        zr = std::complex<double>(-beta*diff, 0.5*beta*gamma);

        N = 0.25*alpha*(1-k*k)/(k*k);

        out[i] = I*N*( (1-eta)*(Nu*exp(u)*gsl_sf_erfc(yu)+Nv*exp(v)*gsl_sf_erfc(yv) + 
                        Ns*exp(s)*gsl_sf_erfc(ys)+Nr*exp(r)*gsl_sf_erfc(yr)) -
                 eta*2.0/M_PI*(Nu*exponentialIntegral(zu).imag()+Nv*exponentialIntegral(zv).imag()
                              +Ns*exponentialIntegral(zs).imag()+Nr*exponentialIntegral(zr).imag()) );
    }

}

void IkedaCarpenterPV::function(double* out, const double* xValues, const int& nData)
{
    const double& I = getParameter("I");
    const double& alpha0 =getParameter("Alpha0");
    const double& alpha1 = getParameter("Alpha1");
    const double& beta0 = getParameter("Beta0");
    const double& kappa = getParameter("Kappa");
    const double& sigmaSquared = getParameter("SigmaSquared");
    const double& gamma = getParameter("Gamma");
    const double& eta = getParameter("Eta");
    const double& X0 = getParameter("X0");

    const double beta = 1/beta0;

    // equations taken from Fullprof manual

    const double k = 0.05;   

    double u,v,s,r;
    double yu, yv, ys, yr;

    const double someConst = 1/sqrt(2.0*sigmaSquared);

    double R, Nu, Nv, Ns, Nr, N;

    std::complex<double> zs, zu, zv, zr;

    double alpha, a_minus, a_plus, x, y, z;

    // update wavelength vector
    calWavelengthAtEachDataPoint(xValues, nData);

    for (int i = 0; i < nData; i++) {
        double diff=xValues[i]-X0;

        if (m_waveLengthFixed)
        {
          // this is to allow unit testing when a workspace is not available
          R = exp(-81.799/(m_waveLength[0]*m_waveLength[0]*kappa));
          alpha = 1.0 / (alpha0+m_waveLength[0]*alpha1);
        }
        else
        {
          R = exp(-81.799/(m_waveLength[i]*m_waveLength[i]*kappa));
          alpha = 1.0 / (alpha0+m_waveLength[i]*alpha1);
        }

        a_minus = alpha*(1-k);
        a_plus = alpha*(1+k);
        x=a_minus-beta;
        y=alpha-beta;
        z=a_plus-beta; 

        Nu=1-R*a_minus/x;
        Nv=1-R*a_plus/z;
        Ns=-2*(1-R*alpha/y);
        Nr=2*R*alpha*alpha*beta*k*k/(x*y*z);

        u=a_minus*(a_minus*sigmaSquared-2*diff)/2.0;
        v=a_plus*(a_plus*sigmaSquared-2*diff)/2.0;
        s=alpha*(alpha*sigmaSquared-2*diff)/2.0;
        r=beta*(beta*sigmaSquared-2*diff)/2.0;

        yu = (a_minus*sigmaSquared-diff)*someConst;
        yv = (a_plus*sigmaSquared-diff)*someConst;
        ys = (alpha*sigmaSquared-diff)*someConst;
        yr = (beta*sigmaSquared-diff)*someConst;

        zs = std::complex<double>(-alpha*diff, 0.5*alpha*gamma);
        zu = (1-k)*zs;
        zv = (1-k)*zs;
        zr = std::complex<double>(-beta*diff, 0.5*beta*gamma);

        N = 0.25*alpha*(1-k*k)/(k*k);

        out[i] = I*N*( (1-eta)*(Nu*exp(u)*gsl_sf_erfc(yu)+Nv*exp(v)*gsl_sf_erfc(yv) + 
                        Ns*exp(s)*gsl_sf_erfc(ys)+Nr*exp(r)*gsl_sf_erfc(yr)) -
                 eta*2.0/M_PI*(Nu*exponentialIntegral(zu).imag()+Nv*exponentialIntegral(zv).imag()
                              +Ns*exponentialIntegral(zs).imag()+Nr*exponentialIntegral(zr).imag()) );
    }
}



} // namespace CurveFitting
} // namespace Mantid
