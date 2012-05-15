//-------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------
#include "MantidKernel/PseudoRandomNumberGenerator.h"

namespace Mantid
{
  namespace Kernel
  {
    /**
     * Returns the next value in the 1D sequence as a point to be
     * compatible with the NDRandomNumberGenerator interface
     */
    std::vector<double> PseudoRandomNumberGenerator::nextPoint()
    {
      return std::vector<double>(1, this->nextValue());
    }
  }
}
