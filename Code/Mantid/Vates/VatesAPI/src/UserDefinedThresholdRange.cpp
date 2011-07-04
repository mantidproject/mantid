#include "MantidVatesAPI/UserDefinedThresholdRange.h"
#include <stdexcept>

namespace Mantid
{
  namespace VATES
  {

    /**
    Constructor.
    @param min : range min.
    @param max : range max.
    */
    UserDefinedThresholdRange::UserDefinedThresholdRange(signal_t min, signal_t max) : m_min(min), m_max(max)
    {
      if(max < min)
      {
        throw std::invalid_argument("Cannot have max < min in a UserDefinedThresholdRange.");
      }
    }

    /**
    Destructor.
    */
    UserDefinedThresholdRange::~UserDefinedThresholdRange()
    {
    }

    /**
    Do nothing calculate method.
    */
    void UserDefinedThresholdRange::calculate()
    {
      //DO NOTHING!
    }

    /**
    Minimum value getter.
    @return The minimum value.
    */
    double UserDefinedThresholdRange::getMinimum() const
    {
      return m_min;
    }

    /**
    Maximum value getter.
    @return The maximum value.
    */
    double UserDefinedThresholdRange::getMaximum() const
    {
      return m_max;
    }
  }
}