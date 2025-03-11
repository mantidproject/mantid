// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidKernel/PseudoRandomNumberGenerator.h"

#include <memory>
#include <random>

namespace Mantid {
namespace Kernel {
/**
  This implements the Mersenne Twister 19937 pseudo-random number
  generator algorithm as a specialzation of the PseudoRandomNumberGenerator
  interface.
*/
class MANTID_KERNEL_DLL MersenneTwister final : public PseudoRandomNumberGenerator {

public:
  /// Construct the generator using time stamp for the initial seed.
  MersenneTwister();
  /// Construct the generator with an initial range and default seed.
  MersenneTwister(const double start, const double end);
  /// Construct the generator with an initial seed. It can be reseeded using
  /// setSeed.
  explicit MersenneTwister(const size_t seedValue);
  /// Construct the generator with an initial seed and range.
  MersenneTwister(const size_t seedValue, const double start, const double end);

  MersenneTwister(const MersenneTwister &) = delete;
  MersenneTwister &operator=(const MersenneTwister &) = delete;

  /// Set the random number seed
  void setSeed(const size_t seedValue);
  /// Sets the range of the subsequent calls to next
  void setRange(const double start, const double end) override;
  /// Generate the next random number in the sequence within the given range
  /// default range
  inline double nextValue() override { return uniformRealDistribution(m_engine); }
  /// Generate the next random number in the sequence within the given range.
  inline double nextValue(double start, double end) override {
    return std::uniform_real_distribution<double>(start, end)(m_engine);
  }
  /// Return the next integer in the sequence within the given range
  int nextInt(int start, int end) override;
  /// Resets the generator
  void restart() override;
  /// Saves the current state of the generator
  void save() override;
  /// Restores the generator to the last saved point, or the beginning if
  /// nothing has been saved
  void restore() override;
  /// Return the minimum value of the range
  double min() const override { return m_start; }
  /// Return the maximum value of the range
  double max() const override { return m_end; }

private:
  /// The engine
  std::mt19937 m_engine;
  /// Uniform Real distribution
  std::uniform_real_distribution<double> uniformRealDistribution;
  /// Minimum in range
  double m_start;
  /// Maximum in range
  double m_end;
  /// The current seed
  std::mt19937::result_type m_seed;
  /// A generator that will take the value when save is requested. Pointer so
  /// that it is only instantiated when required
  std::unique_ptr<std::mt19937> m_savedEngine;
};
} // namespace Kernel
} // namespace Mantid
