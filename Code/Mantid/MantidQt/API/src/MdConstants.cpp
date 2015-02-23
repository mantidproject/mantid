#include "MantidQtAPI/MdConstants.h"

namespace MantidQt
{
  namespace API
  {
    MdConstants::MdConstants() : m_colorScaleStandardMax(0.1), m_logScaleDefaultValue(0.1)
    {
    };

    MdConstants::~MdConstants(){};

    double MdConstants::getColorScaleStandardMax()
    {
      return m_colorScaleStandardMax;
    }

    double MdConstants::getLogScaleDefaultValue()

    {
      return m_logScaleDefaultValue;
    }
  }
}
