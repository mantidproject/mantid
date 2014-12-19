//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/FFTSmooth.h"
#include "MantidKernel/Exception.h"

#include <iostream>
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid {
namespace Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(FFTSmooth)

using namespace Kernel;
using namespace API;

/// Initialisation method. Declares properties to be used in algorithm.
void FFTSmooth::init() {
  declareProperty(new WorkspaceProperty<API::MatrixWorkspace>(
                      "InputWorkspace", "", Direction::Input),
                  "The name of the input workspace.");
  declareProperty(new WorkspaceProperty<API::MatrixWorkspace>(
                      "OutputWorkspace", "", Direction::Output),
                  "The name of the output workspace.");

  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty("WorkspaceIndex", 0, mustBePositive,
                  "Spectrum index for smoothing");

  std::vector<std::string> type;
  // type.push_back("Truncation");
  type.push_back("Zeroing");
  declareProperty("Filter", "Zeroing",
                  boost::make_shared<StringListValidator>(type),
                  "The type of the applied filter");
  declareProperty("Params", "", "The filter parameters");
}

/** Executes the algorithm
 */
void FFTSmooth::exec() {
  m_inWS = getProperty("InputWorkspace");
  int spec = getProperty("WorkspaceIndex");

  // Save the starting x value so it can be restored after all transforms.
  m_x0 = m_inWS->readX(spec)[0];

  // Symmetrize the input spectrum
  int dn = static_cast<int>(m_inWS->readY(0).size());

  API::MatrixWorkspace_sptr symmWS = API::WorkspaceFactory::Instance().create(
      "Workspace2D", 1, m_inWS->readX(0).size() + dn,
      m_inWS->readY(0).size() + dn);

  double dx = (m_inWS->readX(spec).back() - m_inWS->readX(spec).front()) /
              static_cast<double>(m_inWS->readX(spec).size() - 1);
  for (int i = 0; i < dn; i++) {
    symmWS->dataX(0)[dn + i] = m_inWS->readX(spec)[i];
    symmWS->dataY(0)[dn + i] = m_inWS->readY(spec)[i];

    symmWS->dataX(0)[dn - i] = m_x0 - dx * i;
    symmWS->dataY(0)[dn - i] = m_inWS->readY(spec)[i];
  }
  symmWS->dataY(0).front() = m_inWS->readY(spec).back();
  symmWS->dataX(0).front() = m_x0 - dx * dn;
  if (m_inWS->isHistogramData())
    symmWS->dataX(0).back() = m_inWS->readX(spec).back();

  // setProperty("OutputWorkspace",symmWS); return;

  // Forward Fourier transform
  IAlgorithm_sptr fft = createChildAlgorithm("RealFFT", 0, 0.5);
  fft->setProperty("InputWorkspace", symmWS);
  fft->setProperty("WorkspaceIndex", 0);
  try {
    fft->execute();
  } catch (...) {
    g_log.error("Error in direct FFT algorithm");
    throw;
  }

  m_unfilteredWS = fft->getProperty("OutputWorkspace");

  // Apply the filter
  std::string type = getProperty("Filter");
  // x - value correction doesn't work, so no truncation yet
  // if (type == "Truncation")
  //{
  //  std::string sn = getProperty("Params");
  //  int n;
  //  if (sn.empty()) n = 2;
  //  else
  //    n = atoi(sn.c_str());
  //  if (n <= 1) throw std::invalid_argument("Truncation parameter must be an
  //  integer > 1");
  //  truncate(n);
  //}else
  if (type == "Zeroing") {
    std::string sn = getProperty("Params");
    int n;
    if (sn.empty())
      n = 2;
    else
      n = atoi(sn.c_str());
    if (n < 1)
      throw std::invalid_argument(
          "Truncation parameter must be an integer > 1");
    zero(n);
  }

  // Backward transform
  fft = createChildAlgorithm("RealFFT", 0.5, 1.);
  fft->setProperty("InputWorkspace", m_filteredWS);
  // fft->setProperty("Real",0);
  // fft->setProperty("Imaginary",1);
  fft->setProperty("Transform", "Backward");
  try {
    fft->execute();
  } catch (...) {
    g_log.error("Error in inverse FFT algorithm");
    throw;
  }
  API::MatrixWorkspace_sptr tmpWS = fft->getProperty("OutputWorkspace");

  // Create output
  API::MatrixWorkspace_sptr outWS = API::WorkspaceFactory::Instance().create(
      m_inWS, 1, m_inWS->readX(0).size(), m_inWS->readY(0).size());

  dn = static_cast<int>(tmpWS->blocksize()) / 2;

  // x-value correction is needed if the size of the spectrum is changed (e.g.
  // after truncation)
  // but it doesn't work accurately enough, so commented out
  //// Correct the x values:
  // m_x0 -= tmpWS->dataX(0)[dn];
  // if (tmpWS->isHistogramData())
  //{// Align centres of the in and out histograms. I am not sure here
  //  double dX = m_inWS->readX(0)[1] - m_inWS->readX(0)[0];
  //  double dx = tmpWS->readX(0)[1] - tmpWS->readX(0)[0];
  //  m_x0 += dX/2 - dx;
  //}
  // outWS->dataX(0).assign(tmpWS->readX(0).begin()+dn,tmpWS->readX(0).end());
  // outWS->dataY(0).assign(tmpWS->readY(0).begin()+dn,tmpWS->readY(0).end());
  //
  // std::transform( outWS->dataX(0).begin(), outWS->dataX(0).end(),
  // outWS->dataX(0).begin(),
  //  std::bind2nd(std::plus<double>(), m_x0) );

  outWS->dataX(0).assign(m_inWS->readX(0).begin(), m_inWS->readX(0).end());
  outWS->dataY(0).assign(tmpWS->readY(0).begin() + dn, tmpWS->readY(0).end());

  setProperty("OutputWorkspace", outWS);
}

/** Smoothing by truncation.
 *  @param n :: The order of truncation
 */
void FFTSmooth::truncate(int n) {
  int my = static_cast<int>(m_unfilteredWS->readY(0).size());
  int ny = my / n;

  double f = double(ny) / my;

  if (ny == 0)
    ny = 1;
  int nx = m_unfilteredWS->isHistogramData() ? ny + 1 : ny;
  m_filteredWS =
      API::WorkspaceFactory::Instance().create(m_unfilteredWS, 2, nx, ny);

  const Mantid::MantidVec &Yr = m_unfilteredWS->readY(0);
  const Mantid::MantidVec &Yi = m_unfilteredWS->readY(1);
  const Mantid::MantidVec &X = m_unfilteredWS->readX(0);

  Mantid::MantidVec &yr = m_filteredWS->dataY(0);
  Mantid::MantidVec &yi = m_filteredWS->dataY(1);
  Mantid::MantidVec &xr = m_filteredWS->dataX(0);
  Mantid::MantidVec &xi = m_filteredWS->dataX(1);

  // int odd = ny % 2;

  yr.assign(Yr.begin(), Yr.begin() + ny);
  yi.assign(Yi.begin(), Yi.begin() + ny);
  xr.assign(X.begin(), X.begin() + nx);
  xi.assign(X.begin(), X.begin() + nx);

  std::transform(yr.begin(), yr.end(), yr.begin(),
                 std::bind2nd(std::multiplies<double>(), f));
  std::transform(yi.begin(), yi.end(), yi.begin(),
                 std::bind2nd(std::multiplies<double>(), f));

  // for(int i=0;i<=ny2;i++)
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

  // if (m_filteredWS->isHistogramData())
  //{
  //  xr[ny] = X[my2 + ny2 + odd];
  //  xi[ny] = xr[ny];
  //}
}

/** Smoothing by zeroing.
 *  @param n :: The order of truncation
 */
void FFTSmooth::zero(int n) {
  int mx = static_cast<int>(m_unfilteredWS->readX(0).size());
  int my = static_cast<int>(m_unfilteredWS->readY(0).size());
  int ny = my / n;

  if (ny == 0)
    ny = 1;

  m_filteredWS =
      API::WorkspaceFactory::Instance().create(m_unfilteredWS, 2, mx, my);

  const Mantid::MantidVec &Yr = m_unfilteredWS->readY(0);
  const Mantid::MantidVec &Yi = m_unfilteredWS->readY(1);
  const Mantid::MantidVec &X = m_unfilteredWS->readX(0);

  Mantid::MantidVec &yr = m_filteredWS->dataY(0);
  Mantid::MantidVec &yi = m_filteredWS->dataY(1);
  Mantid::MantidVec &xr = m_filteredWS->dataX(0);
  Mantid::MantidVec &xi = m_filteredWS->dataX(1);

  xr.assign(X.begin(), X.end());
  xi.assign(X.begin(), X.end());
  yr.assign(Yr.size(), 0);
  yi.assign(Yr.size(), 0);

  for (int i = 0; i < ny; i++) {
    // if (abs(my2-i) < ny2)
    //{
    yr[i] = Yr[i];
    yi[i] = Yi[i];
    //}
  }
}

} // namespace Algorithm
} // namespace Mantid
