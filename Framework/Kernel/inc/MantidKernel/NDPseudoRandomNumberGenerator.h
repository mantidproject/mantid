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
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/NDRandomNumberGenerator.h"
#ifndef Q_MOC_RUN
#include <memory>
#endif

namespace Mantid {
namespace Kernel {
/**
 *
 * Defines an ND pseudo-random number generator. This uses a single
 * 1D pseudo-random number generator, given by the template type, to produce ND
 *random values. It
 * supports settings a seed value plus a range for each generated value.
 *
 */
template <typename SingleValueGenerator>
class MANTID_KERNEL_DLL NDPseudoRandomNumberGenerator : public NDRandomNumberGenerator {
public:
  /// Constructor
  NDPseudoRandomNumberGenerator(const unsigned int ndims, const size_t seedValue);
  /// Constructor taking a range to limit the each evaluation
  NDPseudoRandomNumberGenerator(const unsigned int ndims, const size_t seedValue, const double start, const double end);

  /// Disable default constructor
  NDPseudoRandomNumberGenerator() = delete;

  /// Disable copy operator
  NDPseudoRandomNumberGenerator(const NDPseudoRandomNumberGenerator &) = delete;

  /// Disable assignment operator
  NDPseudoRandomNumberGenerator &operator=(const NDPseudoRandomNumberGenerator &) = delete;

  /// Set the random number seed
  void setSeed(const size_t seedValue);
  /// Generates the next ND point
  void generateNextPoint() override;
  /// Resets the generator
  void restart() override;
  /// Saves the current state of the generator
  void save() override;
  /// Restores the generator to the last saved point, or the beginning if
  /// nothing has been saved
  void restore() override;

private:
  /// The single value generator
  SingleValueGenerator m_singleValueGen;
};

//-------------------------------------------------------------------------------------------------------
// Implementation
//-------------------------------------------------------------------------------------------------------

/**
 * Constructor taking the number of dimensions and seed The template
 * argument should be the type of a 1DPseudoRandomNumberGenerator and have a
 * constructor that takes a single value as the seed
 * The single value generator is called ndims times for each call to nextPoint
 * @param ndims :: The number of dimensions the point should return
 * @param seedValue :: A seed value
 */
template <typename SingleValueGenerator>
NDPseudoRandomNumberGenerator<SingleValueGenerator>::NDPseudoRandomNumberGenerator(const unsigned int ndims,
                                                                                   const size_t seedValue)
    : NDRandomNumberGenerator(ndims), m_singleValueGen(seedValue) {}

/**
 * Constructor taking the number of dimensions, seed and a range. The template
 * argument should be the type of a 1DPseudoRandomNumberGenerator and have a
 * constructor that takes a three values. A seed & two doubles for the start &
 * end range
 * The single value generator is called ndims times for each call to nextPoint
 * @param ndims :: The number of dimensions the point should return
 * @param seedValue :: A seed value
 * @param start :: The lower value for the range of generated numbers
 * @param end :: The upper value for the range of generated numbers
 */
template <typename SingleValueGenerator>
NDPseudoRandomNumberGenerator<SingleValueGenerator>::NDPseudoRandomNumberGenerator(const unsigned int ndims,
                                                                                   const size_t seedValue,
                                                                                   const double start, const double end)
    : NDRandomNumberGenerator(ndims), m_singleValueGen(seedValue, start, end) {}

/**
 * Set the random number seed
 * @param seedValue :: (Re-)seed the generator
 */
template <typename SingleValueGenerator>
void NDPseudoRandomNumberGenerator<SingleValueGenerator>::setSeed(const size_t seedValue) {
  m_singleValueGen.setSeed(seedValue);
}

/// Generates the next point
template <typename SingleValueGenerator> void NDPseudoRandomNumberGenerator<SingleValueGenerator>::generateNextPoint() {
  for (unsigned int i = 0; i < numberOfDimensions(); ++i) {
    this->cacheGeneratedValue(i, m_singleValueGen.nextValue());
  }
}

/**
 * Resets the underlying generator
 */
template <typename SingleValueGenerator> void NDPseudoRandomNumberGenerator<SingleValueGenerator>::restart() {
  m_singleValueGen.restart();
}

/// Saves the current state of the generator
template <typename SingleValueGenerator> void NDPseudoRandomNumberGenerator<SingleValueGenerator>::save() {
  m_singleValueGen.save();
}

/// Restores the generator to the last saved point, or the beginning if nothing
/// has been saved
template <typename SingleValueGenerator> void NDPseudoRandomNumberGenerator<SingleValueGenerator>::restore() {
  m_singleValueGen.restore();
}
} // namespace Kernel
} // namespace Mantid
