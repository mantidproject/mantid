// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/FFTSmooth.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TextAxis.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/HistogramBuilder.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid {
namespace Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(FFTSmooth)

using namespace Kernel;
using namespace API;
using namespace DataObjects;
using namespace HistogramData;

/// Initialisation method. Declares properties to be used in algorithm.
void FFTSmooth::init() {
  declareProperty(std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "The name of the input workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "The name of the output workspace.");

  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty("WorkspaceIndex", 0, mustBePositive,
                  "Workspace index for smoothing");

  std::vector<std::string> type{"Zeroing"};
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
  double x0 = m_inWS->x(spec)[0];

  // Symmetrize the input spectrum
  int dn = static_cast<int>(m_inWS->y(0).size());

  HistogramBuilder builder;
  builder.setX(m_inWS->x(0).size() + dn);
  builder.setY(m_inWS->y(0).size() + dn);
  builder.setDistribution(m_inWS->isDistribution());
  API::MatrixWorkspace_sptr symmWS =
      create<Workspace2D>(*m_inWS, 1, builder.build());

  double dx = (m_inWS->x(spec).back() - m_inWS->x(spec).front()) /
              static_cast<double>(m_inWS->x(spec).size() - 1);

  auto &symX = symmWS->mutableX(0);
  auto &symY = symmWS->mutableY(0);

  for (int i = 0; i < dn; i++) {
    symX[dn + i] = m_inWS->x(spec)[i];
    symY[dn + i] = m_inWS->y(spec)[i];

    symX[dn - i] = x0 - dx * i;
    symY[dn - i] = m_inWS->y(spec)[i];
  }
  symmWS->mutableY(0).front() = m_inWS->y(spec).back();
  symmWS->mutableX(0).front() = x0 - dx * dn;
  if (m_inWS->isHistogramData())
    symmWS->mutableX(0).back() = m_inWS->x(spec).back();

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

  if (type == "Zeroing") {
    std::string sn = getProperty("Params");
    int n;
    if (sn.empty())
      n = 2;
    else
      n = std::stoi(sn);
    if (n < 1)
      throw std::invalid_argument(
          "Truncation parameter must be an integer > 1");
    zero(n);
  }

  // Backward transform
  fft = createChildAlgorithm("RealFFT", 0.5, 1.);
  fft->setProperty("InputWorkspace", m_filteredWS);
  fft->setProperty("Transform", "Backward");
  try {
    fft->execute();
  } catch (...) {
    g_log.error("Error in inverse FFT algorithm");
    throw;
  }
  API::MatrixWorkspace_sptr tmpWS = fft->getProperty("OutputWorkspace");

  // Create output
  builder.setX(m_inWS->x(0).size());
  builder.setY(m_inWS->y(0).size());
  builder.setDistribution(m_inWS->isDistribution());
  API::MatrixWorkspace_sptr outWS =
      create<MatrixWorkspace>(*m_inWS, 1, builder.build());

  dn = static_cast<int>(tmpWS->blocksize()) / 2;

  outWS->setSharedX(0, m_inWS->sharedX(0));
  outWS->mutableY(0).assign(tmpWS->y(0).cbegin() + dn, tmpWS->y(0).cend());

  setProperty("OutputWorkspace", outWS);
}

/** Smoothing by truncation.
 *  @param n :: The order of truncation
 */
void FFTSmooth::truncate(int n) {
  int my = static_cast<int>(m_unfilteredWS->y(0).size());
  int ny = my / n;

  double f = double(ny) / my;

  if (ny == 0)
    ny = 1;
  int nx = m_unfilteredWS->isHistogramData() ? ny + 1 : ny;
  HistogramBuilder builder;
  builder.setX(nx);
  builder.setY(ny);
  builder.setDistribution(m_unfilteredWS->isDistribution());
  m_filteredWS = create<MatrixWorkspace>(*m_unfilteredWS, 2, builder.build());

  auto &Yr = m_unfilteredWS->y(0);
  auto &Yi = m_unfilteredWS->y(1);
  auto &X = m_unfilteredWS->x(0);

  auto &yr = m_filteredWS->mutableY(0);
  auto &yi = m_filteredWS->mutableY(1);
  auto &xr = m_filteredWS->mutableX(0);
  auto &xi = m_filteredWS->mutableX(1);

  yr.assign(Yr.begin(), Yr.begin() + ny);
  yi.assign(Yi.begin(), Yi.begin() + ny);
  xr.assign(X.begin(), X.begin() + nx);
  xi.assign(X.begin(), X.begin() + nx);

  std::transform(yr.begin(), yr.end(), yr.begin(),
                 std::bind2nd(std::multiplies<double>(), f));
  std::transform(yi.begin(), yi.end(), yi.begin(),
                 std::bind2nd(std::multiplies<double>(), f));
}

/** Smoothing by zeroing.
 *  @param n :: The order of truncation
 */
void FFTSmooth::zero(int n) {
  int mx = static_cast<int>(m_unfilteredWS->x(0).size());
  int my = static_cast<int>(m_unfilteredWS->y(0).size());
  int ny = my / n;

  if (ny == 0)
    ny = 1;

  HistogramBuilder builder;
  builder.setX(mx);
  builder.setY(my);
  builder.setDistribution(m_unfilteredWS->isDistribution());
  m_filteredWS = create<MatrixWorkspace>(*m_unfilteredWS, 2, builder.build());

  m_filteredWS->setSharedX(0, m_unfilteredWS->sharedX(0));
  m_filteredWS->setSharedX(1, m_unfilteredWS->sharedX(0));

  std::copy(m_unfilteredWS->y(0).cbegin(), m_unfilteredWS->y(0).begin() + ny,
            m_filteredWS->mutableY(0).begin());

  std::copy(m_unfilteredWS->y(1).cbegin(), m_unfilteredWS->y(1).begin() + ny,
            m_filteredWS->mutableY(1).begin());
}

} // namespace Algorithms
} // namespace Mantid
