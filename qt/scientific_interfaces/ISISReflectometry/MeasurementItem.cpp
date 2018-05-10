#include "MeasurementItem.h"
#include <sstream>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

/**
 * Constructor
 * @param measurementItemId
 * @param subId
 * @param label
 * @param type
 * @param angle
 * @param run
 * @param title
 */
MeasurementItem::MeasurementItem(
    const MeasurementItem::IDType &measurementItemId,
    const MeasurementItem::IDType &subId, const std::string &label,
    const std::string &type, const double angle, const std::string &run,
    const std::string &title)
    : m_measurementItemId(measurementItemId), m_subId(subId), m_label(label),
      m_type(type), m_angle(angle), m_run(run), m_title(title) {

  std::string accumulatedProblems;
  if (m_measurementItemId.empty()) {
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
MeasurementItem::MeasurementItem(const std::string &why)
    : m_angle(0), m_whyUnuseable(why) {}

/**
 * Copy constructor
 * @param other
 */
MeasurementItem::MeasurementItem(const MeasurementItem &other)
    : m_measurementItemId(other.m_measurementItemId), m_subId(other.m_subId),
      m_label(other.m_label), m_type(other.m_type), m_angle(other.m_angle),
      m_run(other.m_run), m_title(other.m_title),
      m_whyUnuseable(other.m_whyUnuseable) {}

/// Destructor
MeasurementItem::~MeasurementItem() {}

/**
 * InvalidMeasurement static creational method
 * @return Invalid measurement
 */
MeasurementItem
MeasurementItem::InvalidMeasurementItem(const std::string &why) {
  return MeasurementItem(why);
}

bool MeasurementItem::isUseable() const { return m_whyUnuseable.empty(); }

MeasurementItem::IDType MeasurementItem::id() const {
  return m_measurementItemId;
}

MeasurementItem::IDType MeasurementItem::subId() const { return m_subId; }

std::string MeasurementItem::label() const { return m_label; }

std::string MeasurementItem::type() const { return m_type; }

double MeasurementItem::angle() const { return m_angle; }

std::string MeasurementItem::run() const { return m_run; }

std::string MeasurementItem::title() const { return m_title; }

std::string MeasurementItem::angleStr() const {
  std::stringstream buffer;
  buffer << angle();
  return buffer.str();
}

MeasurementItem &MeasurementItem::operator=(const MeasurementItem &other) {
  if (&other != this) {
    m_measurementItemId = other.id();
    m_subId = other.subId();
    m_label = other.label();
    m_type = other.type();
    m_angle = other.angle();
    m_run = other.run();
    m_whyUnuseable = other.whyUnuseable();
  }
  return *this;
}

std::string MeasurementItem::whyUnuseable() const { return m_whyUnuseable; }

} // namespace CustomInterfaces
} // namespace MantidQt
