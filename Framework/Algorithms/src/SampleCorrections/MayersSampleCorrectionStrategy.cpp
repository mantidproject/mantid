//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidAlgorithms/SampleCorrections/MayersSampleCorrectionStrategy.h"
#include "MantidKernel/MersenneTwister.h"
#include "MantidKernel/Statistics.h"
#include "MantidKernel/Math/ChebyshevPolyFit.h"
#include "MantidKernel/Math/Distributions/ChebyshevSeries.h"
#include <cassert>
#include <cmath>

using Mantid::Kernel::ChebyshevPolyFit;
using Mantid::Kernel::ChebyshevSeries;
using Mantid::Kernel::getStatistics;
using Mantid::Kernel::MersenneTwister;
using Mantid::Kernel::StatOptions;
using std::pow;

namespace {

// The constants were set as default in the original fortran
/// Number of muR slices to take
size_t N_MUR_PTS = 21;
/// Number of radial points for cylindrical integration
size_t N_RAD = 29;
/// Number of theta points for cylindrical integration
size_t N_THETA = 29;
/// Number of second order event points
size_t N_SECOND = 10000;
/// Order of polynomial used to fit generated points
size_t N_POLY_ORDER = 4;
/// 2pi
double TWOPI = 2.0 * M_PI;

// Avoid typing static_casts everywhere
template <typename R, typename T> inline R to(const T val) {
  return static_cast<R>(val);
}

//-----------------------------------------------------------------------------
// Utility functions
//-----------------------------------------------------------------------------

/**
 * Integrate by simpsons rule
 * @param y Collection of y values at points separated by dy
 * @param dx Delta value for integral
 * @return The value of the sum using Simpson's rule
 */
double integrate(const std::vector<double> &y, const double dx) {
  assert(y.size() > 3);
  // strictly Simpson's rule says that n should be even but the old fortran
  // didn't...
  // sum even and odd terms excluding the front/back
  double sumEven(0.0), sumOdd(0.0);
  auto itend = y.end() - 1;
  for (auto it = y.begin() + 1; it != itend;) {
    sumOdd += *(it++);
    if (it == itend)
      break;
    sumEven += *(it++);
  }
  return dx * (y.front() + 4.0 * sumOdd + 2.0 * sumEven + y.back()) / 3.0;
}
}

namespace Mantid {
namespace Algorithms {

//-----------------------------------------------------------------------------
// Public methods
//-----------------------------------------------------------------------------
/**
 * Constructor
 * @param params Defines the required parameters for the correction
 * @param tof The TOF values corresponding to the signals to correct.
 *      Number of tof values must match number of signal/error or be 1 greater
 * @param sigIn Values of the signal that will be corrected
 * @param errIn Values of the errors that will be corrected
 */
MayersSampleCorrectionStrategy::MayersSampleCorrectionStrategy(
    MayersSampleCorrectionStrategy::Parameters params,
    const std::vector<double> &tof, const std::vector<double> &sigIn,
    const std::vector<double> &errIn)
    : m_pars(params), m_tof(tof), m_sigin(sigIn), m_errin(errIn),
      m_histogram(tof.size() == sigIn.size() + 1),
      m_muRrange(calculateMuRange()), m_rng(new MersenneTwister(1)) {
  // Sanity check
  assert(sigIn.size() == tof.size() || sigIn.size() == tof.size() - 1);
  assert(errIn.size() == tof.size() || sigIn.size() == tof.size() - 1);

  if (!(m_tof.front() < m_tof.back())) {
    throw std::invalid_argument(
        "TOF values are expected to be monotonically increasing");
  }
}

/**
 * Destructor
 */
MayersSampleCorrectionStrategy::~MayersSampleCorrectionStrategy() {}

/**
 * Correct the data for absorption and multiple scattering effects. Allows
 * both histogram or point data. For histogram the TOF is taken to be
 * the mid point of a bin
 * @param sigOut Signal values to correct [In/Out]
 * @param errOut Error values to correct [In/Out]
 */
void MayersSampleCorrectionStrategy::apply(std::vector<double> &sigOut,
                                           std::vector<double> &errOut) {
  const size_t nsig(m_sigin.size());
  // Sanity check
  assert(sigOut.size() == m_sigin.size());
  assert(errOut.size() == m_errin.size());

  // Temporary storage
  std::vector<double> xmur(N_MUR_PTS + 1, 0.0),
      yabs(N_MUR_PTS + 1, 1.0), // absorption signals
      wabs(N_MUR_PTS + 1, 1.0), // absorption weights
      yms(0),                   // multiple scattering signals
      wms(0);                   // multiple scattering weights
  if (m_pars.mscat) {
    yms.resize(N_MUR_PTS + 1, 0.0);
    wms.resize(N_MUR_PTS + 1, 100.0);
  }

  // Main loop over mur. Limit is nrpts but vectors are nrpts+1. First value set
  // by initial values above
  const double dmuR = (muRmax() - muRmin()) / to<double>(N_MUR_PTS - 1);
  for (size_t i = 1; i < N_MUR_PTS + 1; ++i) {
    const double muR = muRmin() + to<double>(i - 1) * dmuR;
    xmur[i] = muR;

    auto attenuation = calculateSelfAttenuation(muR);
    const double absFactor = attenuation / (M_PI * muR * muR);
    // track these
    yabs[i] = 1. / absFactor;
    wabs[i] = absFactor;
    if (m_pars.mscat) {
      // ratio of second/first scatter
      auto mscat = calculateMS(i, muR, attenuation);
      yms[i] = mscat.first;
      wms[i] = mscat.second;
    }
  }

  // Fit polynomials to absorption values to interpolate to input data range
  ChebyshevPolyFit polyfit(N_POLY_ORDER);
  auto absCoeffs = polyfit(xmur, yabs, wabs);
  decltype(absCoeffs) msCoeffs(0);
  if (m_pars.mscat)
    msCoeffs = polyfit(xmur, yms, wms);

  // corrections to input
  const double muMin(xmur.front()), muMax(xmur.back()),
      flightPath(m_pars.l1 + m_pars.l2),
      vol(M_PI * m_pars.cylHeight * pow(m_pars.cylRadius, 2));
  //  Oct 2003 discussion with Jerry Mayers:
  //  1E-22 factor in formula for RNS was introduced by Jerry to keep
  //   multiple scattering correction close to 1
  const double rns = (vol * 1e6) * (m_pars.rho * 1e24) * 1e-22;
  ChebyshevSeries chebyPoly(N_POLY_ORDER);

  for (size_t i = 0; i < nsig; ++i) {
    const double sigt = sigmaTotal(flightPath, tof(i));
    const double rmu = muR(sigt);
    // Varies between [-1,+1]
    const double xcap = ((rmu - muMin) - (muMax - rmu)) / (muMax - muMin);
    double corrfact = chebyPoly(absCoeffs, xcap);
    if (m_pars.mscat) {
      const double msVal = chebyPoly(msCoeffs, xcap);
      const double beta = m_pars.sigmaSc * msVal / sigt;
      corrfact *= (1.0 - beta) / rns;
    }
    // apply correction
    const double yin(m_sigin[i]), ein(m_errin[i]);
    sigOut[i] = yin * corrfact;
    errOut[i] = sigOut[i] * ein / yin;
  }
}

/**
 * Calculate the self-attenutation factor for the given mur value
 * @param muR Single mu*r slice value
 * @return The self-attenuation factor for this sample
 */
double
MayersSampleCorrectionStrategy::calculateSelfAttenuation(const double muR) {
  // Integrate over the cylindrical coordinates
  // Constants for calculation
  const double dyr = muR / to<double>(N_RAD - 1);
  const double dyth = TWOPI / to<double>(N_THETA - 1);
  const double muRSq = muR * muR;

  // Store values at each point
  std::vector<double> yr(N_RAD), yth(N_THETA);
  for (size_t i = 0; i < N_RAD; ++i) {
    const double r0 = to<double>(i) * dyr;

    for (size_t j = 0; j < N_THETA; ++j) {
      const double theta = to<double>(j) * dyth;
      // distance to vertical axis
      double fact1 = muRSq - std::pow(r0 * sin(theta), 2);
      if (fact1 < 0.0)
        fact1 = 0.0;
      // + final distance to scatter point
      const double mul1 = sqrt(fact1) + r0 * cos(theta);
      // exit distance after scatter
      double fact2 = muRSq - std::pow(r0 * sin(m_pars.twoTheta - theta), 2);
      if (fact2 < 0.0)
        fact2 = 0.0;
      const double mul2 =
          (sqrt(fact2) - r0 * cos(m_pars.twoTheta - theta)) / cos(m_pars.phi);
      yth[j] = exp(-mul1 - mul2);
    }

    yr[i] = r0 * integrate(yth, dyth);
  }
  return integrate(yr, dyr);
}

/**
 * Calculate the multiple scattering correction factor and weight for the given
 * mur value
 * @param irp Index of current mur point (assumed zero based)
 * @param muR Single \f$\mu*r\f$ slice value
 * @param abs Absorption and self-attenuation factor (\f$A_s\f$ in Mayers paper)
 * @return A pair of (factor,weight)
 */
std::pair<double, double>
MayersSampleCorrectionStrategy::calculateMS(const size_t irp, const double muR,
                                            const double abs) {
  // Radial coordinate raised to power 1/3 to ensure uniform density of points
  // across circle following discussion with W.G.Marshall (ISIS)
  const double radDistPower = 1. / 3.;
  double muH = muR * (m_pars.cylHeight / m_pars.cylRadius);
  seedRNG(irp);

  // Take an average over a number of sets of second scatters
  const size_t nsets(10);
  std::vector<double> deltas(nsets, 0.0);
  for (size_t j = 0; j < nsets; ++j) {
    double sum = 0.0;
    for (size_t i = 0; i < N_SECOND; ++i) {
      // Random (r,theta,z)
      const double r1 = pow(m_rng->nextValue(), radDistPower) * muR;
      const double r2 = pow(m_rng->nextValue(), radDistPower) * muR;
      const double z1 = m_rng->nextValue() * muH;
      const double z2 = m_rng->nextValue() * muH;
      const double th1 = m_rng->nextValue() * TWOPI;
      const double th2 = m_rng->nextValue() * TWOPI;
      double fact1 = pow(muR, 2) - std::pow(r1 * sin(th1), 2);
      if (fact1 < 0.0)
        fact1 = 0.0;
      // Path into first point
      const double mul1 = sqrt(fact1) + r1 * cos(th1);
      double fact2 = pow(muR, 2) - pow(r2 * sin(m_pars.twoTheta - th2), 2);
      if (fact2 < 0.0)
        fact2 = 0.0;
      // Path out from final point
      const double mul2 =
          (sqrt(fact2) - r2 * cos(m_pars.twoTheta - th2)) / cos(m_pars.phi);
      // Path between point 1 & 2
      const double mul12 =
          sqrt(pow(r1 * cos(th1) - r2 * cos(th2), 2) +
               pow(r1 * sin(th1) - r2 * sin(th2), 2) + pow(z1 - z2, 2));
      if (mul12 < 0.01)
        continue;
      sum += exp(-(mul1 + mul2 + mul12)) / pow(mul12, 2);
    }
    const double beta =
        pow(M_PI * muR * muR * muH, 2) * sum / to<double>(N_SECOND);
    const double delta = 0.25 * beta / (M_PI * abs * muH);
    deltas[j] = delta;
  }
  auto stats =
      getStatistics(deltas, StatOptions::Mean | StatOptions::CorrectedStdDev);
  return std::make_pair(stats.mean, stats.mean / stats.standard_deviation);
}

//-----------------------------------------------------------------------------
// Private methods
//-----------------------------------------------------------------------------
/**
 * Calculate the mu*r range required to cover the given tof range. It requires
 * that the internal parameters have been set.
 * @return A pair of (min,max) values for muR for the given time of flight
 */
std::pair<double, double>
MayersSampleCorrectionStrategy::calculateMuRange() const {
  const double flightPath(m_pars.l1 + m_pars.l2);
  const double tmin(tof(0)), tmax(tof(m_sigin.size() - 1));
  return std::make_pair(muR(flightPath, tmin), muR(flightPath, tmax));
}

/**
 * Calculate the value of mu*r for the given flightPath and time of flight
 * @param flightPath Total distance travelled in metres
 * @param tof The time of flight in microseconds
 * @return The mu*r value
 */
double MayersSampleCorrectionStrategy::muR(const double flightPath,
                                           const double tof) const {
  return muR(sigmaTotal(flightPath, tof));
}

/**
 * Calculate the value of mu*r for the given total scattering cross section
 * @param sigt The total scattering cross section (barns)
 * @return The mu*r value
 */
double MayersSampleCorrectionStrategy::muR(const double sigt) const {
  // Dimensionless number - rho in (1/Angtroms^3), sigt in barns
  // (1/Angstrom = 1e8/cm) * (barn = 1e-24cm) --> factors cancel out
  return m_pars.rho * sigt * (m_pars.cylRadius * 1e2);
}

/**
 * Calculate the value of the total scattering cross section for the given
 * flightPath and time of flight
 * @param flightPath Total distance travelled in metres
 * @param tof The time of flight in microseconds
 * @return The total scattering cross section in barns
 */
double MayersSampleCorrectionStrategy::sigmaTotal(const double flightPath,
                                                  const double tof) const {
  // sigabs = sigabs(@2200(m/s)^-1)*2200 * velocity;
  const double sigabs = m_pars.sigmaAbs * 2200.0 * tof * 1e-6 / flightPath;
  return sigabs + m_pars.sigmaSc;
}

/**
 * Return the TOF for the given index of the signal value, taking into account
 * if we have a histogram. Histograms will use the mid point of the bin
 * as the TOF value. Note that there is no range check for the index.
 * @param i Index of the signal value
 * @return The associated TOF value
 */
double MayersSampleCorrectionStrategy::tof(const size_t i) const {
  return m_histogram ? 0.5 * (m_tof[i] + m_tof[i + 1]) : m_tof[i];
}

/**
 * (Re-)seed the random number generator
 * @param seed Seed value for the random number generator
 */
void MayersSampleCorrectionStrategy::seedRNG(const size_t seed) {
  m_rng->setSeed(seed);
}

} // namespace Algorithms
} // namespace Mantid
