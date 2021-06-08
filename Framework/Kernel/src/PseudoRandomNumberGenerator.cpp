// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//-------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------
#include "MantidKernel/PseudoRandomNumberGenerator.h"

namespace Mantid {
namespace Kernel {

/// Default constructor setting the dimension to 1
PseudoRandomNumberGenerator::PseudoRandomNumberGenerator() : NDRandomNumberGenerator(1) {}

/**
 * Returns the next value in the 1D sequence as a point to be
 * compatible with the NDRandomNumberGenerator interface
 */
void PseudoRandomNumberGenerator::generateNextPoint() { cacheGeneratedValue(0, this->nextValue()); }
} // namespace Kernel
} // namespace Mantid
