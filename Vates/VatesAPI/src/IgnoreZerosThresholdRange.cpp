#include "MantidVatesAPI/IgnoreZerosThresholdRange.h"
#include <stdexcept>

namespace Mantid {
namespace VATES {

/**
Constructor.
*/
IgnoreZerosThresholdRange::IgnoreZerosThresholdRange() : m_min(1), m_max(1) {}

/**
Constructor.
*/
IgnoreZerosThresholdRange::IgnoreZerosThresholdRange(signal_t min, signal_t max)
    : m_min(min), m_max(max) {}

/**
Indicates wheter execution has occured or not.
return : true always.
*/
bool IgnoreZerosThresholdRange::hasCalculated() const { return true; }

/**
Destructor.
*/
IgnoreZerosThresholdRange::~IgnoreZerosThresholdRange() {}

/**
Do nothing calculate method.
*/
void IgnoreZerosThresholdRange::calculate() {}

/**
Minimum value getter.
@return The minimum value.
*/
double IgnoreZerosThresholdRange::getMinimum() const { return m_min; }

/**
Maximum value getter.
@return The maximum value.
*/
double IgnoreZerosThresholdRange::getMaximum() const { return m_max; }

/**
Virtual constructor clone method.
@return clone of original.
*/
IgnoreZerosThresholdRange *IgnoreZerosThresholdRange::clone() const {
  return new IgnoreZerosThresholdRange(this->m_min, this->m_max);
}

/**
Determine whether the signal is withing range.
@param signal value
@return true if the signal is in the range defined by this object.
*/
bool IgnoreZerosThresholdRange::inRange(const signal_t &signal) {
  m_max = signal > m_max ? signal : m_max; // cache min and max values.
  if (signal < m_min && 0 != signal) {
    m_min = signal < m_min ? signal : m_min;
  }
  return signal != 0;
}
}
}
