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


double IkedaCarpenterPV::centre()const 
{
  return getParameter("X0");
}

double IkedaCarpenterPV::height()const 
{
  return getParameter("I");
};

double IkedaCarpenterPV::width()const 
{
  // here estimated to sum of the HWHM of the gaussian and lorentzian part...
  double eta = getParameter("Eta");

  // eta should always be between zero and one but before putting in codes to
  // constrain this do the following

  if (eta >= 0 && eta <= 1.0)
    return 0.5*(eta*getParameter("Gamma")+(1-eta)*sqrt(getParameter("SigmaSquared"))*2);
  else
    return 0.5*(getParameter("Gamma")+sqrt(getParameter("SigmaSquared"))*2);
};

void IkedaCarpenterPV::setCentre(const double c) 
{
  getParameter("X0") = c;
};

void IkedaCarpenterPV::setHeight(const double h) 
{
  m_height = h;

  if (m_width != 0.0)
    getParameter("I") = h;
  else
    getParameter("I") = 0.25*m_width*m_height;
};

void IkedaCarpenterPV::setWidth(const double w) 
{
  getParameter("SigmaSquared") = w*w/80.0;  

  getParameter("Gamma") = w/32.0; 

  m_width = w;

  if ( m_height != 0.0 )
    getParameter("I") = 0.25*m_width*m_height;
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

void IkedaCarpenterPV::calWavelengthAtEachDataPoint(const double* xValues, const int& nData)
{
  if (!m_waveLengthFixed)
  { 
    // if wavelength vector already have the right size no need for resizing it

    if (m_waveLength.size() != nData)
    {
      m_waveLength.resize(nData);
    }

    // Get the geometric information for this detector
    
    API::IInstrument_const_sptr instrument = m_workspace->getInstrument();
    Geometry::IObjComponent_const_sptr sample = instrument->getSample();
    const double l1 = instrument->getSource()->getDistance(*sample);
    Geometry::IDetector_sptr det = m_workspace->getDetector(m_workspaceIndex);  // i is the workspace index
    const double l2 = det->getDistance(*sample);
    const double twoTheta = m_workspace->detectorTwoTheta(det);
   
    Mantid::Kernel::Unit_const_sptr wavelength = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");
    //mWaveLength = workspace->readX(m_workspaceIndex); // Copy the TOF values for the spectrum of interest
    for (int i = 0; i < nData; i++)
    {
      m_waveLength[i] = xValues[i];
    }
    std::vector<double> y; // Create an empty vector, it's not used in fromTOF
    wavelength->fromTOF(m_waveLength,y,l1,l2,twoTheta,0,0.0,0.0);
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
