//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/IkedaCarpenterPV1D.h"
#include <gsl/gsl_sf_erf.h>
#include <gsl/gsl_multifit_nlin.h>
#include "MantidCurveFitting/SpecialFunctionSupport.h"
#include "MantidKernel/UnitFactory.h"

namespace Mantid
{
namespace CurveFitting
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(IkedaCarpenterPV1D)

using namespace Kernel;
//using API::MatrixWorkspace_const_sptr;
using namespace SpecialFunctionSupport;

void IkedaCarpenterPV1D::declareParameters()
{
  BoundedValidator<double> *positiveDouble = new BoundedValidator<double>();
  positiveDouble->setLower(std::numeric_limits<double>::min());

  declareProperty("I", 0.0, "Magnitude of peak (default 0)", Direction::InOut);
  declareProperty("Alpha0",1.6, positiveDouble, "Used to model fast decay constant (default 1.6)", Direction::InOut);
  declareProperty("Alpha1",1.5, positiveDouble->clone(), "Used to model fast decay constant (default 1.5)", Direction::InOut);
  declareProperty("Beta0",31.9, positiveDouble->clone(), "Inverse of slow decay constant (default 31.9)", Direction::InOut);
  declareProperty("Kappa",46.0, positiveDouble->clone(), "Controls contribution of slow decay term (default 46.0)", Direction::InOut);
  declareProperty("SigmaSquared",1.0, positiveDouble->clone(), 
    "standard deviation squared (Guassian broadening) (default 1.0)", Direction::InOut);
  declareProperty("Gamma",1.0, positiveDouble->clone(),
    "Lorentzian broadening  (default 1.0)", Direction::InOut);

  BoundedValidator<double> *percentageValue = new BoundedValidator<double>(0.0,1.0);
  declareProperty("Eta",0.0, percentageValue,
    "Mixing parameter (percentage in terms of Lorentzian part) (default 0.0)", Direction::InOut);

  declareProperty("X0",0.0, "Peak position (default 0)", Direction::InOut);
  declareProperty("BG", 0.0, "Constant background value (default 0)", Direction::InOut);
}

void IkedaCarpenterPV1D::afterDataRangedDetermined(const int& m_minX, const int& m_maxX)
{
  if (!mWaveLengthFixed)
  { 
    API::MatrixWorkspace_const_sptr workspace = getProperty("InputWorkspace");
    int histNumber = getProperty("WorkspaceIndex");

    // Get the geometric information for this detector
    API::IInstrument_const_sptr instrument = workspace->getInstrument();
    Geometry::IObjComponent_const_sptr sample = instrument->getSample();
    const double l1 = instrument->getSource()->getDistance(*sample);
    Geometry::IDetector_sptr det = workspace->getDetector(histNumber);  // i is the workspace index
    const double l2 = det->getDistance(*sample);
    const double twoTheta = workspace->detectorTwoTheta(det);
   
    Mantid::Kernel::Unit_const_sptr wavelength = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");
    mWaveLength = workspace->readX(histNumber); // Copy the TOF values for the spectrum of interest
    MantidVec y; // Create an empty vector, it's not used in fromTOF
    wavelength->fromTOF(mWaveLength,y,l1,l2,twoTheta,0,0.0,0.0);

    if (m_minX > 0)
      mWaveLength.erase(mWaveLength.begin(), mWaveLength.begin()+m_minX);
    
    mWaveLength.resize(m_maxX - m_minX);
  }




}

void IkedaCarpenterPV1D::function(const double* in, double* out, const double* xValues, const int& nData)
{
    const double& I = in[0];
    const double& alpha0 = in[1];
    const double& alpha1 = in[2];
    const double& beta0 = in[3];
    const double& kappa = in[4];
    const double& sigmaSquared = in[5];
    const double& gamma = in[6];
    const double& eta = in[7];
    const double& X0 = in[8];
    const double& BG = in[9];

    const double beta = 1/beta0;

    // equations taken from Fullprof manual

    const double k = 0.05;   

    double u,v,s,r;
    double yu, yv, ys, yr;

    const double someConst = 1/sqrt(2.0*sigmaSquared);

    double R, Nu, Nv, Ns, Nr, N;

    std::complex<double> zs, zu, zv, zr;

    double alpha, a_minus, a_plus, x, y, z;

    for (int i = 0; i < nData; i++) {
        double diff=xValues[i]-X0;

        if (mWaveLengthFixed)
        {
          R = exp(-81.799/(mWaveLength[0]*mWaveLength[0]*kappa));
          alpha = 1.0 / (alpha0+mWaveLength[0]*alpha1);
        }
        else
        {
          R = exp(-81.799/(mWaveLength[i]*mWaveLength[i]*kappa));
          alpha = 1.0 / (alpha0+mWaveLength[i]*alpha1);
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
                              +Ns*exponentialIntegral(zs).imag()+Nr*exponentialIntegral(zr).imag()) )
                 + BG;
    }
}


} // namespace CurveFitting
} // namespace Mantid
