//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidAlgorithms/MultipleScattering/LindleyMayersElasticCorrection.h"
#include "MantidKernel/MersenneTwister.h"
#include "MantidKernel/Statistics.h"
#include <cassert>
#include <cmath>

using Mantid::Kernel::MersenneTwister;
using Mantid::Kernel::getStatistics;
using Mantid::Kernel::StatOptions;
using std::pow;

namespace {
// Avoid typing static_casts everywhere
template <typename R, typename T> inline R to(const T val) {
  return static_cast<R>(val);
}

// The constants were set as default in the original fortran and their
// values came from what worked well for POLARIS at ISIS
/// Number of muR slices to take
size_t N_MUR_PTS = 21;
/// Number of radial points for cylindrical integration
size_t N_RAD = 29;
/// Number of theta points for cylindrical integration
size_t N_THETA = 29;
/// Number of second order event points
size_t N_SECOND = 10000;
/// 2pi
double TWOPI = 2.0 * M_PI;

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
 */
LindleyMayersElasticCorrection::LindleyMayersElasticCorrection(
    ScatteringCorrectionParameters params)
    : m_pars(params), m_muRrange(0.01, 4.0), m_rng(new MersenneTwister(1)) {}

/**
 * Destructor
 */
LindleyMayersElasticCorrection::~LindleyMayersElasticCorrection() {}

/**
 * Correct the given data for absorption and multiple scattering effects
 * @param tof Time-of-flight values in microseconds
 * @param signal Signal values to correct [In/Out]
 * @param errors Error values to correct [In/Out]
 */
void LindleyMayersElasticCorrection::apply(const std::vector<double> &tof,
                                           std::vector<double> &signal,
                                           std::vector<double> &errors) {
  UNUSED_ARG(tof);
  UNUSED_ARG(signal);
  UNUSED_ARG(errors);
  // Temporary storage
  std::vector<double> xmur(N_MUR_PTS + 1, 0.0), yabs(N_MUR_PTS + 1, 1.0),
      wabs(N_MUR_PTS + 1, 1.0), // absorption signals + weights
      yms(N_MUR_PTS + 1, 0.0),
      wms(N_MUR_PTS + 1, 100.0); // multiple scattering signals + weights

  // Main loop over mur. Limit is nrpts but vectors are nrpts+1. First value set
  // by initial values above
  const double deltaR = muRmax() - muRmin();
  for (size_t i = 0; i < N_MUR_PTS; ++i) {
    const double muR =
        muRmin() + to<double>(i) * deltaR / to<double>(N_MUR_PTS - 1);
    xmur[i + 1] = muR;

    auto attenuation = calculateSelfAttenuation(muR);
    const double absFactor = attenuation / (M_PI * muR * muR);
    yabs[i + 1] = 1. / absFactor;
    wabs[i + 1] = absFactor;
  }
}

/**
 * Calculate the self-attenutation factor for the given mur value
 * @param muR Single mu*r slice value
 * @return The self-attenuation factor for this sample
 */
double LindleyMayersElasticCorrection::calculateSelfAttenuation(const double muR) {
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
LindleyMayersElasticCorrection::calculateMS(const size_t irp, const double muR,
                                            const double abs) {
  // Radial coordinate raised to power 1/3 to ensure uniform density of points
  // across circle following discussion with W.G.Marshall (ISIS)
  const double radDistPower = 1. / 3.;
  double muH = muR * (m_pars.cylHeight / m_pars.cylRadius);
  seedRNG(irp + 1);

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
 * (Re-)seed the random number generator
 * @param seed Seed value for the random number generator
 */
void LindleyMayersElasticCorrection::seedRNG(const size_t seed) {
  m_rng->setSeed(seed);
}

} // namespace Algorithms
} // namespace Mantid
