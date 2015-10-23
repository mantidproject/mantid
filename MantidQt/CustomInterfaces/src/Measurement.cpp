#include "MantidQtCustomInterfaces/Measurement.h"

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
      m_type(type), m_angle(angle), m_run(run), m_valid(true) {

  if (m_measurementId.empty()) {
    m_valid = false;
  } else if (m_subId.empty()) {
    m_valid = false;
  } else if (m_label.empty()) {
    m_valid = false;
  } else if (m_type.empty()) {
    m_valid = false;
  } else if (m_run.empty()) {
    m_valid = false;
  }
}

/**
 * Constructor making an invalid Measurement
 */
Measurement::Measurement() : m_valid(false), m_angle(0) {}

/**
 * Copy constructor
 * @param other
 */
Measurement::Measurement(const Measurement &other)
    : m_measurementId(other.m_measurementId), m_subId(other.m_subId),
      m_label(other.m_label), m_type(other.m_type), m_angle(other.m_angle),
      m_run(other.m_run), m_valid(other.m_valid) {}

/// Destructor
Measurement::~Measurement() {}

/**
 * InvalidMeasurement static creational method
 * @return Invalid measurement
 */
Measurement Measurement::InvalidMeasurement() { return Measurement(); }

bool Measurement::isUseable() const { return m_valid; }

Measurement::IDType Measurement::id() const { return m_measurementId; }

Measurement::IDType Measurement::subId() const { return m_subId; }

std::string Measurement::label() const { return m_label; }

std::string Measurement::type() const { return m_type; }

double Measurement::angle() const { return m_angle; }

std::string Measurement::run() const { return m_run; }

} // namespace CustomInterfaces
} // namespace Mantid
