#include "MantidQtAPI/MdConstants.h"

namespace MantidQt
{
  namespace API
  {
    MdConstants::MdConstants() : m_colorScaleStandardMax(0.1)
    {
    };

    MdConstants::~MdConstants(){};

    double MdConstants::getColorScaleStandardMax()
    {
      return m_colorScaleStandardMax;
    }
  }
}
