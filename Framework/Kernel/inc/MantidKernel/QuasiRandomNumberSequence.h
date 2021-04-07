// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidKernel/NDRandomNumberGenerator.h"

namespace Mantid {
namespace Kernel {
/**
 *
 * Defines an interface to a quasi-random number sequence. A quasi-random
 *sequence
 * progressively covers a d-dimensional space with a set of points that are
 * uniformly distributed.
 */
class MANTID_KERNEL_DLL QuasiRandomNumberSequence : public NDRandomNumberGenerator {
public:
  QuasiRandomNumberSequence(const unsigned int ndims) : NDRandomNumberGenerator(ndims) {}
};
} // namespace Kernel
} // namespace Mantid
