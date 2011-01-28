//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/FFTSmooth2.h"
#include "MantidKernel/Exception.h"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/detail/classification.hpp>


#include <iostream>

namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(FFTSmooth2)

using namespace Kernel;
using namespace API;

/// Initialisation method. Declares properties to be used in algorithm.
void FFTSmooth2::init()
{
      declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace",
        "",Direction::Input), "The name of the input workspace.");
      declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace",
        "",Direction::Output), "The name of the output workspace.");

      BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
      mustBePositive->setLower(0);
      declareProperty("WorkspaceIndex",0,mustBePositive,"Spectrum index for smoothing");

      std::vector<std::string> type;
      //type.push_back("Truncation");
      type.push_back("Zeroing");
      type.push_back("Butterworth");
      declareProperty("Filter","Zeroing",new ListValidator(type),"The type of the applied filter");
      declareProperty("Params","",
          "The filter parameters:\n"
          "For Zeroing, 1 parameter: 'n' - an integer greater than 1 meaning that the Fourier coefficients with frequencies outside the 1/n of the original range will be set to zero.\n"
          "For Butterworth, 2 parameters: 'n' and 'order', giving the 1/n truncation and the smoothing order.\n" );

      declareProperty("IgnoreXBins",false,
          "Ignores the requirement that X bins be linear and of the same size.\n"
          "Set this to true if you are using log binning.\n"
          "The output X axis will be the same as the input either way.");

}

/// @cond 
// Anonymous namespace
namespace {
// Binary function struct for performing sqrt(x2 * y2) returning a double
struct toReal : std::binary_function <double,double,double> {
  // Binary function performing sqrt(x2 * y2) returning a double
  double operator() (const double& x, const double& y) const
    {return sqrt(x*x+y*y);}
};
}
/// @endcond

/** Executes the algorithm
 */
void FFTSmooth2::exec()
{
  m_inWS = getProperty("InputWorkspace");
  int spec = getProperty("WorkspaceIndex");
  IgnoreXBins = getProperty("IgnoreXBins");

  // Save the starting x value so it can be restored after all transforms.
  m_x0 = m_inWS->readX(spec)[0];

  // Symmetrize the input spectrum 
  int dn = m_inWS->readY(0).size();

  API::MatrixWorkspace_sptr symmWS = 
    API::WorkspaceFactory::Instance().create("Workspace2D",1,m_inWS->readX(0).size()+dn,m_inWS->readY(0).size()+dn);

  double dx = (m_inWS->readX(spec).back() - m_inWS->readX(spec).front()) / (m_inWS->readX(spec).size() - 1);
  for(int i=0;i<dn;i++)
  {
    symmWS->dataX(0)[dn + i] = m_inWS->readX(spec)[i];
    symmWS->dataY(0)[dn + i] = m_inWS->readY(spec)[i];

    symmWS->dataX(0)[dn - i] = m_x0 - dx*i;
    symmWS->dataY(0)[dn - i] = m_inWS->readY(spec)[i];
  }
  symmWS->dataY(0).front() = m_inWS->readY(spec).back();
  symmWS->dataX(0).front() = m_x0 - dx*dn;
  if (m_inWS->isHistogramData())
    symmWS->dataX(0).back() = m_inWS->readX(spec).back();

  //setProperty("OutputWorkspace",symmWS); return;

  // Forward Fourier transform
  IAlgorithm_sptr fft = createSubAlgorithm("RealFFT", 0, 0.5 );
  fft->setProperty("InputWorkspace",symmWS);
  fft->setProperty("WorkspaceIndex",0);
  fft->setProperty("IgnoreXBins", IgnoreXBins);
  try
  {
    fft->execute();
  }
  catch(...)
  {
    g_log.error("Error in direct FFT algorithm");
    throw;
  }

  m_unfilteredWS = fft->getProperty("OutputWorkspace");

  // Apply the filter
  std::string type = getProperty("Filter");
  // x - value correction doesn't work, so no truncation yet
  //if (type == "Truncation")
  //{
  //  std::string sn = getProperty("Params");
  //  int n;
  //  if (sn.empty()) n = 2;
  //  else
  //    n = atoi(sn.c_str());
  //  if (n <= 1) throw std::invalid_argument("Truncation parameter must be an integer > 1");
  //  truncate(n);
  //}else 
  if (type == "Zeroing")
  {
    std::string sn = getProperty("Params");
    int n;
    if (sn.empty()) n = 2;
    else
      n = atoi(sn.c_str());
    if (n <= 1) throw std::invalid_argument("Truncation parameter must be an integer > 1");
    zero(n);
  }
  else if (type == "Butterworth")
  {
    int n,
        order;

    std::string string_params = getProperty("Params");
    std::vector<std::string> params;
    boost::split( params, string_params, boost::algorithm::detail::is_any_ofF<char>(" ,:;\t"));
    if (params.size() != 2)
    { 
      n     = 2;
      order = 2;
    }
    else
    {
      std::string param0 = params.at(0);
      std::string param1 = params.at(1);
      n     = atoi(param0.c_str());
      order = atoi(param1.c_str());
    }
    if (n     <= 1) throw std::invalid_argument("Truncation parameter must be an integer > 1");
    if (order <  1) throw std::invalid_argument("Butterworth filter order must be an integer >= 1");
    Butterworth(n,order);
  }

  // Backward transform
  fft = createSubAlgorithm("RealFFT", 0.5, 1. );
  fft->setProperty("InputWorkspace",m_filteredWS);
  //fft->setProperty("Real",0);
  //fft->setProperty("Imaginary",1);
  fft->setProperty("Transform","Backward");
  fft->setProperty("IgnoreXBins", IgnoreXBins);
  try
  {
    fft->execute();
  }
  catch(...)
  {
    g_log.error("Error in inverse FFT algorithm");
    throw;
  }
  API::MatrixWorkspace_sptr tmpWS = fft->getProperty("OutputWorkspace");

  // Create output
  API::MatrixWorkspace_sptr outWS = 
    API::WorkspaceFactory::Instance().create(m_inWS,1,m_inWS->readX(0).size(),m_inWS->readY(0).size());

  dn = tmpWS->blocksize()/2;

  // x-value correction is needed if the size of the spectrum is changed (e.g. after truncation)
  // but it doesn't work accurately enough, so commented out
  //// Correct the x values:
  //m_x0 -= tmpWS->dataX(0)[dn];
  //if (tmpWS->isHistogramData())
  //{// Align centres of the in and out histograms. I am not sure here
  //  double dX = m_inWS->readX(0)[1] - m_inWS->readX(0)[0];
  //  double dx = tmpWS->readX(0)[1] - tmpWS->readX(0)[0];
  //  m_x0 += dX/2 - dx;
  //}
  //outWS->dataX(0).assign(tmpWS->readX(0).begin()+dn,tmpWS->readX(0).end());
  //outWS->dataY(0).assign(tmpWS->readY(0).begin()+dn,tmpWS->readY(0).end());
  //
  //std::transform( outWS->dataX(0).begin(), outWS->dataX(0).end(), outWS->dataX(0).begin(), 
  //  std::bind2nd(std::plus<double>(), m_x0) );

  outWS->dataX(0).assign(m_inWS->readX(0).begin(),m_inWS->readX(0).end());
  outWS->dataY(0).assign(tmpWS->readY(0).begin()+dn,tmpWS->readY(0).end());
  
  setProperty("OutputWorkspace",outWS);

}

/** Smoothing by truncation.
 *  @param n :: The order of truncation
 */
void FFTSmooth2::truncate(int n)
{
  int my = m_unfilteredWS->readY(0).size();
  int ny = my / n;

  double f = double(ny)/my;

  if (ny == 0) ny = 1;
  int nx = m_unfilteredWS->isHistogramData() ? ny + 1 : ny;
  m_filteredWS = API::WorkspaceFactory::Instance().create(m_unfilteredWS,2,nx,ny);

  const Mantid::MantidVec& Yr = m_unfilteredWS->readY(0);
  const Mantid::MantidVec& Yi = m_unfilteredWS->readY(1);
  const Mantid::MantidVec& X = m_unfilteredWS->readX(0);

  Mantid::MantidVec& yr = m_filteredWS->dataY(0);
  Mantid::MantidVec& yi = m_filteredWS->dataY(1);
  Mantid::MantidVec& xr = m_filteredWS->dataX(0);
  Mantid::MantidVec& xi = m_filteredWS->dataX(1);

  //int odd = ny % 2;

  yr.assign(Yr.begin(),Yr.begin()+ny);
  yi.assign(Yi.begin(),Yi.begin()+ny);
  xr.assign(X.begin(),X.begin()+nx);
  xi.assign(X.begin(),X.begin()+nx);

  std::transform(yr.begin(),yr.end(),yr.begin(),std::bind2nd(std::multiplies<double>(),f));
  std::transform(yi.begin(),yi.end(),yi.begin(),std::bind2nd(std::multiplies<double>(),f));

  //for(int i=0;i<=ny2;i++)
  //{
  //  double re = Yr[my2 - i] * f;
  //  double im = Yi[my2 - i] * f;
  //  double x = X[my2 - i];
  //  yr[ny2 - i] = re;
  //  yi[ny2 - i] = im;
  //  xr[ny2 - i] = x;
  //  xi[ny2 - i] = x;
  //  if (odd || i < ny2)
  //  {
  //    yr[ny2 + i] = re;
  //    if (i > 0) yi[ny2 + i] = -im;
  //    x = X[my2 + i];
  //    xr[ny2 + i] = x;
  //    xi[ny2 + i] = x;
  //  }
  //}

  //if (m_filteredWS->isHistogramData())
  //{
  //  xr[ny] = X[my2 + ny2 + odd];
  //  xi[ny] = xr[ny];
  //}

}

/** Smoothing by zeroing.
 *  @param n :: The order of truncation
 */
void FFTSmooth2::zero(int n)
{
  int mx = m_unfilteredWS->readX(0).size();
  int my = m_unfilteredWS->readY(0).size();
  int ny = my / n;

  if (ny == 0) ny = 1;

  m_filteredWS = API::WorkspaceFactory::Instance().create(m_unfilteredWS,2,mx,my);

  const Mantid::MantidVec& Yr = m_unfilteredWS->readY(0);
  const Mantid::MantidVec& Yi = m_unfilteredWS->readY(1);
  const Mantid::MantidVec& X = m_unfilteredWS->readX(0);

  Mantid::MantidVec& yr = m_filteredWS->dataY(0);
  Mantid::MantidVec& yi = m_filteredWS->dataY(1);
  Mantid::MantidVec& xr = m_filteredWS->dataX(0);
  Mantid::MantidVec& xi = m_filteredWS->dataX(1);

  xr.assign(X.begin(),X.end());
  xi.assign(X.begin(),X.end());
  yr.assign(Yr.size(),0);
  yi.assign(Yr.size(),0);

  for(int i=0;i<ny;i++)
  {
    //if (abs(my2-i) < ny2)
    //{
      yr[i] = Yr[i];
      yi[i] = Yi[i];
    //}
  }

}


/** Smoothing using Butterworth filter.
 *  @param n ::     The cutoff frequency control parameter.
 *               Cutoff frequency = my/n where my is the 
 *               number of sample points in the data.
 *               As with the "Zeroing" case, the cutoff
 *               frequency is truncated to an integer value
 *               and set to 1 if the truncated value was zero.
 *  @param order :: The order of the Butterworth filter, 1, 2, etc. 
 *               This must be a positive integer.
 */
void FFTSmooth2::Butterworth(int n, int order)
{
  int mx = m_unfilteredWS->readX(0).size();
  int my = m_unfilteredWS->readY(0).size();
  int ny = my / n;

  if (ny == 0) ny = 1;

  m_filteredWS = API::WorkspaceFactory::Instance().create(m_unfilteredWS,2,mx,my);

  const Mantid::MantidVec& Yr = m_unfilteredWS->readY(0);
  const Mantid::MantidVec& Yi = m_unfilteredWS->readY(1);
  const Mantid::MantidVec& X = m_unfilteredWS->readX(0);

  Mantid::MantidVec& yr = m_filteredWS->dataY(0);
  Mantid::MantidVec& yi = m_filteredWS->dataY(1);
  Mantid::MantidVec& xr = m_filteredWS->dataX(0);
  Mantid::MantidVec& xi = m_filteredWS->dataX(1);

  xr.assign(X.begin(),X.end());
  xi.assign(X.begin(),X.end());
  yr.assign(Yr.size(),0);
  yi.assign(Yr.size(),0);

  double cutoff = ny;
  double scale;
  for(int i=0;i<my;i++)
  {
    scale = 1.0/(1.0 + pow(i/cutoff, 2*order));
    yr[i] = scale * Yr[i];
    yi[i] = scale * Yi[i];
  }

}



} // namespace Algorithm
} // namespace Mantid
