#include "MantidSINQ/PoldiUtilities/UncertainValue.h"
#include <stdexcept>
#include <cmath>
#include "boost/format.hpp"

namespace Mantid {
namespace Poldi {

UncertainValue::UncertainValue() : m_value(0.0), m_error(0.0) {}

UncertainValue::UncertainValue(double value) : m_value(value), m_error(0.0) {}

UncertainValue::UncertainValue(double value, double error) : m_value(value) {
  if (error < 0.0) {
    throw std::domain_error("Error cannot be below 0.");
  }

  m_error = error;
}

double UncertainValue::value() const { return m_value; }

double UncertainValue::error() const { return m_error; }

UncertainValue::operator double() const { return m_value; }

const UncertainValue
UncertainValue::plainAddition(const UncertainValue &left,
                              const UncertainValue &right) {
  return UncertainValue(left.m_value + right.m_value,
                        left.m_error + right.m_error);
}

bool UncertainValue::lessThanError(const UncertainValue &left,
                                   const UncertainValue &right) {
  return left.m_error < right.m_error;
}

double UncertainValue::valueToErrorRatio(const UncertainValue &uncertainValue) {
  if (uncertainValue.error() == 0.0) {
    throw std::domain_error("Division by zero is not defined.");
  }
  return uncertainValue.value() / uncertainValue.error();
}

double UncertainValue::errorToValueRatio(const UncertainValue &uncertainValue) {
  if (uncertainValue.value() == 0.0) {
    throw std::domain_error("Division by zero is not defined.");
  }

  return uncertainValue.error() / uncertainValue.value();
}

UncertainValue UncertainValue::operator*(double d) {
  return UncertainValue(m_value * d, m_error * d);
}

UncertainValue UncertainValue::operator/(double d) {
  if (d == 0.0) {
    throw std::domain_error("Divsion by 0 is not allowed.");
  }

  return UncertainValue(m_value / d, m_error / d);
}

UncertainValue UncertainValue::operator+(double d) {
  return UncertainValue(m_value + d, m_error);
}

UncertainValue UncertainValue::operator-(double d) {
  return UncertainValue(m_value - d, m_error);
}

UncertainValue operator*(double d, const UncertainValue &v) {
  return UncertainValue(d * v.value(), d * v.error());
}

UncertainValue operator/(double d, const UncertainValue &v) {
  if (v.value() == 0.0) {
    throw std::domain_error("Divsion by 0 is not allowed.");
  }

  return UncertainValue(d / v.value(), d / pow(v.value(), 2.0) * v.error());
}

UncertainValue operator+(double d, const UncertainValue &v) {
  return UncertainValue(d + v.value(), v.error());
}

UncertainValue operator-(double d, const UncertainValue &v) {
  return UncertainValue(d - v.value(), v.error());
}
}
}
