#include "MantidQtAPI/MdConstants.h"

namespace MantidQt
{
  namespace API
  {
    MdConstants::MdConstants() : m_colorScaleStandardMax(0.1)
    {
    };

    MdConstants::~MdConstants(){};

    const double MdConstants::getColorScaleStandardMax() const
    {
      return m_colorScaleStandardMax;
    }
  }
}
