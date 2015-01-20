//-------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------
#include "MantidKernel/PseudoRandomNumberGenerator.h"

namespace Mantid {
namespace Kernel {

/// Default constructor setting the dimension to 1
PseudoRandomNumberGenerator::PseudoRandomNumberGenerator()
    : NDRandomNumberGenerator(1) {}

/**
 * Returns the next value in the 1D sequence as a point to be
 * compatible with the NDRandomNumberGenerator interface
 */
void PseudoRandomNumberGenerator::generateNextPoint() {
  cacheGeneratedValue(0, this->nextValue());
}
}
}
