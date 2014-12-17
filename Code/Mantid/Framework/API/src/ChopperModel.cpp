//
// Includes
//
#include "MantidAPI/ChopperModel.h"

#include "MantidKernel/Exception.h"
#include "MantidKernel/Strings.h"

#include <boost/lexical_cast.hpp>

namespace Mantid {
namespace API {
namespace {
// Parameter names
const char *ANGULAR_VEL = "AngularVelocity";
const char *JITTER = "JitterSigma";
}

/// Default constructor required by the factory
ChopperModel::ChopperModel()
    : m_exptRun(NULL), m_angularSpeed(0.0), m_angularSpeedLog(),
      m_jitterSigma(0.0), m_pulseVariance(0.0) {}

/**
 * Set the reference to the run object so that log
 * values can be used as values for the parameters
 * @param exptRun :: A reference to the run object to access logs
 */
void ChopperModel::setRun(const Run &exptRun) { m_exptRun = &exptRun; }

/**
 * Initialize the object with a string of parameters
 * @param params :: A key=value list of parameters
 */
void ChopperModel::initialize(const std::string &params) {
  if (params.empty())
    throw std::invalid_argument(
        "ChopperModel::initialize - Empty parameter string.");
  static const char *keyValSep = "=";
  static const char *listSep = ",";

  auto keyValues =
      Kernel::Strings::splitToKeyValues(params, keyValSep, listSep);
  if (keyValues.empty()) {
    throw std::invalid_argument(
        "ChopperModel::initialize - Parameter string was not empty but no "
        "values"
        " could be parsed. Check it is a comma-separated key=value string");
  }

  setBaseParameters(keyValues);

  for (auto iter = keyValues.begin(); iter != keyValues.end(); ++iter) {
    setParameterValue(iter->first, iter->second);
  }
}

/**
 * Returns the variance of the pulse through the chopper in seconds^2
 * for the current parameters
 * @return The variance of the pulse through the chopper in seconds^
 */
double ChopperModel::pulseTimeVariance() const {
  /// TODO: Check for caches & return cached value
  return calculatePulseTimeVariance();
}

/**
 * Set the rotation speed in Hz. It will be converted to rads/sec
 * @param value :: The rotation velocity in Hz
 */
void ChopperModel::setAngularVelocityInHz(const double value) {
  m_angularSpeedLog = "";
  m_angularSpeed = value * 2.0 * M_PI;
}

/**
 * Set the name of the log to use to retrieve the velocity
 * @param logName :: The name of a log
 */
void ChopperModel::setAngularVelocityLog(const std::string &logName) {
  m_angularSpeed = 0.0;
  m_angularSpeedLog = logName;
}

/**
 * Returns the current angular velocity in rads/sec.
 * If the log has been set it is used, else the double value is
 * taken
 * @returns The value of the angular velocity
 */
double ChopperModel::getAngularVelocity() const {
  if (m_angularSpeedLog.empty())
    return m_angularSpeed;
  else {
    return 2. * M_PI * exptRun().getLogAsSingleValue(m_angularSpeedLog);
  }
}

/**
 * Sets the chopper jitter sigma value in microseconds. This is the FWHH value.
 * @param value :: The FWHH value in microseconds for the chopper jitter
 */
void ChopperModel::setJitterFWHH(const double value) {
  m_jitterSigma = value / 1e6 / std::sqrt(std::log(256.0));
}

//-------------------------------------------------------------------------
// Private methods
//-------------------------------------------------------------------------

/**
 * Handle any base parameters before passing to derived
 * @param keyValues :: A reference to the set of key-value pairs. Any dealt with
 * here
 * are also removed
 */
void
ChopperModel::setBaseParameters(std::map<std::string, std::string> &keyValues) {
  auto iter = keyValues.find(ANGULAR_VEL);
  if (iter != keyValues.end()) {
    try {
      setAngularVelocityInHz(boost::lexical_cast<double>(iter->second));
    } catch (boost::bad_lexical_cast &) {
      // Assume the value is a log name
      setAngularVelocityLog(iter->second);
    }
    keyValues.erase(ANGULAR_VEL);
  }
  iter = keyValues.find(JITTER);
  if (iter != keyValues.end()) {
    setJitterFWHH(boost::lexical_cast<double>(iter->second));
    keyValues.erase(JITTER);
  }
}
}
}
