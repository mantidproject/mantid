#include "MantidQtCustomInterfaces/Measurement.h"
#include <string>
#include <sstream>

namespace MantidQt {
namespace CustomInterfaces {

/**
 * Constructor
 * @param measurementId
 * @param subId
 * @param label
 * @param type
 * @param angle
 * @param run
 */
Measurement::Measurement(const Measurement::IDType &measurementId,
                         const Measurement::IDType &subId,
                         const std::string &label, const std::string &type,
                         const double angle, const std::string &run)
    : m_measurementId(measurementId), m_subId(subId), m_label(label),
      m_type(type), m_angle(angle), m_run(run) {

  std::string accumulatedProblems;
  if (m_measurementId.empty()) {
    accumulatedProblems += "No measurement id. ";
  } else if (m_subId.empty()) {
    accumulatedProblems += "No sub id. ";
  } else if (m_run.empty()) {
    accumulatedProblems += "No run";
  }
  m_whyUnuseable = accumulatedProblems;
}

/**
 * Constructor making an invalid Measurement
 */
Measurement::Measurement(const std::string &why)
    : m_angle(0), m_whyUnuseable(why) {}

/**
 * Copy constructor
 * @param other
 */
Measurement::Measurement(const Measurement &other)
    : m_measurementId(other.m_measurementId), m_subId(other.m_subId),
      m_label(other.m_label), m_type(other.m_type), m_angle(other.m_angle),
      m_run(other.m_run), m_whyUnuseable(other.m_whyUnuseable) {}

/// Destructor
Measurement::~Measurement() {}

/**
 * InvalidMeasurement static creational method
 * @return Invalid measurement
 */
Measurement Measurement::InvalidMeasurement(const std::string &why) {
  return Measurement(why);
}

bool Measurement::isUseable() const { return m_whyUnuseable.empty(); }

Measurement::IDType Measurement::id() const { return m_measurementId; }

Measurement::IDType Measurement::subId() const { return m_subId; }

std::string Measurement::label() const { return m_label; }

std::string Measurement::type() const { return m_type; }

double Measurement::angle() const { return m_angle; }

std::string Measurement::run() const { return m_run; }

std::string Measurement::angleStr() const {
  std::stringstream buffer;
  buffer << angle();
  return buffer.str();
}

Measurement &Measurement::operator=(const Measurement &other) {
  if (&other != this) {
    m_measurementId = other.id();
    m_subId = other.subId();
    m_label = other.label();
    m_type = other.type();
    m_angle = other.angle();
    m_run = other.run();
    m_whyUnuseable = other.whyUnuseable();
  }
  return *this;
}

std::string Measurement::whyUnuseable() const { return m_whyUnuseable; }

} // namespace CustomInterfaces
} // namespace Mantid
