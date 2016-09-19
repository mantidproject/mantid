#include "MantidVatesAPI/NoThresholdRange.h"
#include <stdexcept>

namespace Mantid {
namespace VATES {

/**
Constructor.
*/
NoThresholdRange::NoThresholdRange() : m_min(0), m_max(0) {}

/**
Constructor.
*/
NoThresholdRange::NoThresholdRange(signal_t min, signal_t max)
    : m_min(min), m_max(max) {}

/**
Indicates wheter execution has occured or not.
return : true always.
*/
bool NoThresholdRange::hasCalculated() const { return true; }

/**
Destructor.
*/
NoThresholdRange::~NoThresholdRange() {}

/**
Do nothing calculate method.
*/
void NoThresholdRange::calculate() {}

/**
Minimum value getter.
@return The minimum value.
*/
double NoThresholdRange::getMinimum() const { return m_min; }

/**
Maximum value getter.
@return The maximum value.
*/
double NoThresholdRange::getMaximum() const { return m_max; }

/**
Virtual constructor clone method.
@return clone of original.
*/
NoThresholdRange *NoThresholdRange::clone() const {
  return new NoThresholdRange(this->m_min, this->m_max);
}

/**
Determine whether the signal is withing range.
@param signal value
@return true if the signal is in the range defined by this object.
*/
bool NoThresholdRange::inRange(const signal_t &signal) {
  m_max = signal > m_max ? signal : m_max; // cache min and max values.
  m_min = signal < m_min ? signal : m_min;
  return true;
}
}
}
