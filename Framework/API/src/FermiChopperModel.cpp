#include "MantidAPI/FermiChopperModel.h"

#include "MantidKernel/Exception.h"
#include "MantidKernel/PhysicalConstants.h"

#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>

#include <cmath>
#include <sstream>

namespace Mantid {
namespace API {
namespace {
/// Parameters
const char *CHOPPER_RADIUS = "ChopperRadius";
const char *SLIT_THICK = "SlitThickness";
const char *SLIT_RADIUS = "SlitRadius";
const char *INCIDENT_ENERGY = "Ei";
}

/// Default constructor required by the factory
FermiChopperModel::FermiChopperModel()
    : ChopperModel(), m_chopperRadius(0.0), m_slitThickness(0.0),
      m_slitRadius(0.0), m_incidentEnergy(0.0), m_incidentEnergyLog() {}

/// @returns a clone of the current object
boost::shared_ptr<ChopperModel> FermiChopperModel::clone() const {
  return boost::make_shared<FermiChopperModel>(*this);
}

/**
 * Set the radius of the chopper in metres
 * @param value :: The value, in metres, of the radius of the chopper
 */
void FermiChopperModel::setChopperRadius(const double value) {
  m_chopperRadius = value;
}

/**
 * Set the thickness of the slit in metres
 * @param value :: The value, in metres, of the radius of the chopper
 */
void FermiChopperModel::setSlitThickness(const double value) {
  m_slitThickness = value;
}

/**
 * Set the radius of curvature of the slit in metres
 * @param value :: The value, in metres, of the radius of the chopper
 */
void FermiChopperModel::setSlitRadius(const double value) {
  m_slitRadius = value;
}

/**
 *  Set the incident energy in meV
 * @param value :: The value of the incident energy in meV
 */
void FermiChopperModel::setIncidentEnergy(const double value) {
  m_incidentEnergy = value;
  m_incidentEnergyLog = "";
}

/**
 * Set the log used to access the Ei
 * @param logName :: The log used to access the Ei value
 */
void FermiChopperModel::setIncidentEnergyLog(const std::string &logName) {
  m_incidentEnergyLog = logName;
  m_incidentEnergy = 0.0;
}

/// @returns the current incident energy in meV
double FermiChopperModel::getIncidentEnergy() const {
  if (m_incidentEnergyLog.empty())
    return m_incidentEnergy;
  else {
    return exptRun().getLogAsSingleValue(m_incidentEnergyLog);
  }
}

/**
 * Returns a time sampled from the distribution given a flat random number
 * @param randomNo :: A flat random number in the range [0,1]
 * @return A time sample from a triangular distribution with peak at min=-1,
 * max=1 and peak at 0
 */
double FermiChopperModel::sampleTimeDistribution(const double randomNo) const {
  if (randomNo >= 0.0 && randomNo <= 1.0) {
    const double effectiveTime = std::sqrt(6.0 * pulseTimeVariance());
    return effectiveTime * sampleFromTriangularDistribution(randomNo);
  } else {
    std::ostringstream os;
    os << "FermiChopperModel::sampleTimeDistribution - Random number must be "
          "flat between [0,1]. Current value=" << randomNo;
    throw std::invalid_argument(os.str());
  }
}

/**
 * Returns a time sampled from the jitter distribution
 * @param randomNo :: A flat random number in the range [0,1]
 * @return A time sample from a jitter distribution with peak at min=-1, max=1
 * and peak at 0
 */
double
FermiChopperModel::sampleJitterDistribution(const double randomNo) const {
  const double jitSig = getStdDevJitter();
  if (jitSig > 0.0) // Avoid sampling if unnecessary
  {
    const double effectiveJitter = std::sqrt(6.0) * jitSig;
    return effectiveJitter * sampleFromTriangularDistribution(randomNo);
  }
  return 0.0;
}

//----------------------------------------------------------------------------------------
// Private members
//----------------------------------------------------------------------------------------
/**
 * Sets a parameter from a name & string value
 * @param name :: The name of the parameter
 * @param value :: The value as a string
 */
void FermiChopperModel::setParameterValue(const std::string &name,
                                          const std::string &value) {
  if (name == INCIDENT_ENERGY) {
    try {
      setIncidentEnergy(boost::lexical_cast<double>(value));
    } catch (boost::bad_lexical_cast &) {
      setIncidentEnergyLog(value);
    }
    return;
  }

  const double valueAsDbl = boost::lexical_cast<double>(value);
  if (name == CHOPPER_RADIUS) {
    setChopperRadius(valueAsDbl);
  } else if (name == SLIT_THICK) {
    setSlitThickness(valueAsDbl);
  } else if (name == SLIT_RADIUS) {
    setSlitRadius(valueAsDbl);
  } else {
    throw std::invalid_argument(
        "FermiChopperModel::setParameterValue - Unknown parameter: " + name);
  }
}

/**
 * Calculate the variance of a the time pulse through this chopper in \f$s^2\f$
 * \f[\tau^2 = \frac{\Delta_T^2 R_f}{6.0}\f] where \f$R_f\f$ is defined by
 * FermiChopperModel::regimerFactor"("const double")const"
 * @return The value of the variance of the time pulse after passing through the
 * chopper
 * in \f$s^2\f$^2
 */
double FermiChopperModel::calculatePulseTimeVariance() const {
  const double mevToSpeedSq =
      2.0 * PhysicalConstants::meV / PhysicalConstants::NeutronMass;

  const double omega = getAngularVelocity();
  const double ei = getIncidentEnergy();

  const double deltaT = 0.5 * m_slitThickness / m_chopperRadius / omega;
  const double inverseSlitSpeed = 0.5 / omega / m_slitRadius;
  const double inverseNeutronSpeed = 1.0 / std::sqrt(ei * mevToSpeedSq);
  const double gamma = 2.0 * m_chopperRadius / deltaT *
                       std::fabs(inverseSlitSpeed - inverseNeutronSpeed);
  double regime(0.0);
  try {
    regime = regimeFactor(gamma);
  } catch (std::invalid_argument &exc) {
    std::string msg = exc.what();
    std::ostringstream os;
    os << "\nComponent values: chopper radius=" << m_chopperRadius
       << ",deltaT=" << deltaT << ",slitRadius=" << m_slitRadius << ",Ei=" << ei
       << ",omega=" << omega << ",slitThickness=" << m_slitThickness;
    throw std::invalid_argument(msg + os.str());
  }

  return deltaT * deltaT * regime / 6.0;
}

/**
 * Computes the value of
 *    \f[\frac{(1-\gamma^4/10)}{(1-\gamma^2/6)}\f]  if \f$ 0 \leq \gamma < 1\f$
 *    \f[\frac{3}{5}\frac{\gamma(\sqrt{\gamma}-2)^2(\sqrt{\gamma}+8)}{(\sqrt{\gamma}+4)}\f]
 *if \f$ 1 \leq \gamma < 4 \f$
 *    raises an error for \f$ 4 \leq \gamma\f$
 *
 * @returns A single unit-less value
 */
double FermiChopperModel::regimeFactor(const double gamma) const {
  if (gamma < 1.0) {
    const double gammaSq = gamma * gamma;
    return (1.0 - (gammaSq * gammaSq / 10.0)) / (1 - (gammaSq / 6.0));
  } else if (gamma < 4.0) {
    const double sqrtGamma = std::sqrt(gamma);
    const double numerator =
        gamma * std::pow((sqrtGamma - 2.0), 2) * (sqrtGamma + 8.0);
    const double denominator = sqrtGamma + 4.0;

    return 0.6 * (numerator) / (denominator);
  } else {
    std::ostringstream os;
    os << "FermiChopperModel::regimeFactor - gamma is greater than 4! "
          "Behaviour is undefined. Value=" << gamma;
    throw std::invalid_argument(os.str());
  }
}

/**
 * Map a flat random number to a triangular distibution of unit area, with max
 * height at zero and [xmin,xmax]=[-1,1]
 * @param randomNo :: A flat random number
 * @returns The number mapped to the triangular distribution
 */
double FermiChopperModel::sampleFromTriangularDistribution(
    const double randomNo) const {
  double xmin(-1.0);
  double offset = std::sqrt(std::fabs(1.0 - 2.0 * std::fabs(randomNo - 0.5)));
  if (randomNo > 0.5) {
    xmin = 1.0;
    offset *= -1.0;
  }
  return xmin + offset;
}
}
}
