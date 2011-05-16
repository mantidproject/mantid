#include "MantidAlgorithms/FFTDerivative.h"

#include <algorithm>
#include <functional>

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FFTDerivative)

using namespace Mantid::Kernel;
using namespace Mantid::API;

void FFTDerivative::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,"Input workspace for differentiation"));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output,"Workspace with result derivatives"));
}

void FFTDerivative::exec()
{

  MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outWS;

  size_t n = inWS->getNumberHistograms();
  API::Progress progress(this,0,1,n);

  int ny = inWS->readY(0).size();
  int nx = inWS->readX(0).size();

  // Workspace for holding a copy of a spectrum. Each spectrum is symmetrized to minimize
  // possible edge effects.
  MatrixWorkspace_sptr copyWS = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
    (Mantid::API::WorkspaceFactory::Instance().create(inWS,1,nx+ny,ny+ny));

  bool isHist = (nx != ny);

  for(int spec = 0; spec < n; ++spec)
  {

    const Mantid::MantidVec& x0 = inWS->readX(spec);
    const Mantid::MantidVec& y0 = inWS->readY(spec);

    Mantid::MantidVec& x1 = copyWS->dataX(0);
    Mantid::MantidVec& y1 = copyWS->dataY(0);

    double xx = 2*x0[0];

    x1[ny] = x0[0];
    y1[ny] = y0[0];

    for(int i = 1; i < ny; ++i)
    {
      int j1 = ny - i;
      int j2 = ny + i;
      x1[j1] = xx - x0[i];
      x1[j2] = x0[i];
      y1[j1] = y1[j2] = y0[i];
    }

    x1[0] = 2*x1[1] - x1[2];
    y1[0] = y0.back();

    if (isHist)
    {
      x1[y1.size()] = x0[ny];
    }

    // Transform symmetrized spectrum
    IAlgorithm_sptr fft = createSubAlgorithm("RealFFT");
    fft->setProperty("InputWorkspace",copyWS);
    fft->setProperty("WorkspaceIndex",0);
    fft->setProperty("Transform","Forward");
    fft->execute();

    MatrixWorkspace_sptr transWS = fft->getProperty("OutputWorkspace");

    Mantid::MantidVec& nu = transWS->dataX(0);
    Mantid::MantidVec& re = transWS->dataY(0);
    Mantid::MantidVec& im = transWS->dataY(1);

    // Multiply the transform by 2*pi*i*w
    for(int j=0; j < re.size(); ++j)
    {
      double w = 2 * M_PI * nu[j];
      double a =  re[j]*w;
      double b = -im[j]*w;
      re[j] = b;
      im[j] = a;
    }

    // Inverse transform
    fft = createSubAlgorithm("RealFFT");
    fft->setProperty("InputWorkspace",transWS);
    fft->setProperty("Transform","Backward");
    fft->execute();

    transWS = fft->getProperty("OutputWorkspace");

    int m2 = transWS->readY(0).size() / 2;
    int my = m2 + (transWS->readY(0).size() % 2 ? 1 : 0);
    int mx = my + (transWS->isHistogramData() ? 1 : 0);

    if (!outWS)
    {
      outWS = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
        (Mantid::API::WorkspaceFactory::Instance().create(inWS,n,mx,my));
    }

    // Save the upper half of the inverse transform for output
    Mantid::MantidVec& x = outWS->dataX(spec);
    Mantid::MantidVec& y = outWS->dataY(spec);
    double dx = x1[0];
    std::copy(transWS->dataX(0).begin() + m2,transWS->dataX(0).end(),x.begin());
    std::transform(x.begin(),x.end(),x.begin(),std::bind2nd(std::plus<double>(),dx));
    std::copy(transWS->dataY(0).begin() + m2,transWS->dataY(0).end(),y.begin());

    progress.report();

  }

  setProperty("OutputWorkspace",outWS);

}

} // Algorithms
} // Mandid

