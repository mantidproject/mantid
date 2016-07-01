#include "MantidVatesAPI/UserDefinedThresholdRange.h"
#include <stdexcept>

namespace Mantid {
namespace VATES {

/**
Constructor.
@param min : range min.
@param max : range max.
*/
UserDefinedThresholdRange::UserDefinedThresholdRange(signal_t min, signal_t max)
    : m_min(min), m_max(max) {
  if (max < min) {
    throw std::invalid_argument(
        "Cannot have max < min in a UserDefinedThresholdRange.");
  }
}

/**
Indicates wheter execution has occured or not.
return : true always.
*/
bool UserDefinedThresholdRange::hasCalculated() const { return true; }

/**
Destructor.
*/
UserDefinedThresholdRange::~UserDefinedThresholdRange() {}

/**
Do nothing calculate method.
*/
void UserDefinedThresholdRange::calculate() {}

/**
Minimum value getter.
@return The minimum value.
*/
double UserDefinedThresholdRange::getMinimum() const { return m_min; }

/**
Maximum value getter.
@return The maximum value.
*/
double UserDefinedThresholdRange::getMaximum() const { return m_max; }

/**
Virtual constructor clone method.
@return clone of original.
*/
UserDefinedThresholdRange *UserDefinedThresholdRange::clone() const {
  return new UserDefinedThresholdRange(this->m_min, this->m_max);
}

/**
Determine whether the signal is withing range.
@param signal value
@return true if the signal is in the range defined by this object.
*/
bool UserDefinedThresholdRange::inRange(const signal_t &signal) {
  return signal >= m_min && signal <= m_max;
}
}
}
