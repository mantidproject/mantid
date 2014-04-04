#include "MantidKernel/UnitLabel.h"

namespace Mantid
{
  namespace Kernel
  {

    /**
     */
    UnitLabel::~UnitLabel()
    {
    }

    /**
     * Returns the results of the ascii() method
     */
    UnitLabel::operator std::string()
    {
      return this->ascii();
    }

  } // namespace Kernel
} // namespace Mantid
