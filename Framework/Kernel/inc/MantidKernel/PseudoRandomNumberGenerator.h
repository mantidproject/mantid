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
 * Defines a 1D pseudo-random number generator, i.e. a generator
 * that takes an initial seed and produces a set of numbers. It specialises
 * the interface for a general random number generator.
 */
class MANTID_KERNEL_DLL PseudoRandomNumberGenerator : public NDRandomNumberGenerator {
public:
  /// Default constructor setting the dimension to 1
  PseudoRandomNumberGenerator();

  /// Disable copy operator
  PseudoRandomNumberGenerator(const PseudoRandomNumberGenerator &) = delete;

  /// Disable assignment operator
  PseudoRandomNumberGenerator &operator=(const PseudoRandomNumberGenerator &) = delete;

  /// Sets the range of the subsequent calls to nextValue;
  virtual void setRange(const double start, const double end) = 0;
  /// Return the next double in the sequence
  virtual double nextValue() = 0;
  /// Return the next double in the sequence overriding the default range
  virtual double nextValue(double start, double end) = 0;
  /// Return the next integer in the sequence
  virtual int nextInt(int start, int end) = 0;
  /// Generates the next point
  void generateNextPoint() override;
  // Interface to boost distribution generators
  /// Result (output) value type.
  using result_type = double;
  /// Return the minimum value of the range
  virtual double min() const = 0;
  /// Return the maximum value of the range
  virtual double max() const = 0;
  /// Return next random value
  double operator()() { return nextValue(); }
};
} // namespace Kernel
} // namespace Mantid
