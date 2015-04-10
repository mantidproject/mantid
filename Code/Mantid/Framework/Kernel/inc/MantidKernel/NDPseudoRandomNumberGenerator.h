#ifndef MANTID_KERNEL_NDPSEUDORANDOMNUMBERGENERATOR_H_
#define MANTID_KERNEL_NDPSEUDORANDOMNUMBERGENERATOR_H_
/**
  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>.
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidKernel/ClassMacros.h"
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/NDRandomNumberGenerator.h"
#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
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
class DLLExport NDPseudoRandomNumberGenerator : public NDRandomNumberGenerator {
public:
  /// Constructor
  NDPseudoRandomNumberGenerator(const unsigned int ndims,
                                const size_t seedValue);
  /// Constructor taking a range to limit the each evaluation
  NDPseudoRandomNumberGenerator(const unsigned int ndims,
                                const size_t seedValue, const double start,
                                const double end);
  /// Set the random number seed
  void setSeed(const size_t seedValue);
  /// Generates the next ND point
  void generateNextPoint();
  /// Resets the generator
  void restart();
  /// Saves the current state of the generator
  void save();
  /// Restores the generator to the last saved point, or the beginning if
  /// nothing has been saved
  void restore();

private:
  DISABLE_DEFAULT_CONSTRUCT(NDPseudoRandomNumberGenerator)
  DISABLE_COPY_AND_ASSIGN(NDPseudoRandomNumberGenerator)

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
NDPseudoRandomNumberGenerator<
    SingleValueGenerator>::NDPseudoRandomNumberGenerator(const unsigned int
                                                             ndims,
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
NDPseudoRandomNumberGenerator<SingleValueGenerator>::
    NDPseudoRandomNumberGenerator(const unsigned int ndims,
                                  const size_t seedValue, const double start,
                                  const double end)
    : NDRandomNumberGenerator(ndims), m_singleValueGen(seedValue, start, end) {}

/**
 * Set the random number seed
 * @param seedValue :: (Re-)seed the generator
 */
template <typename SingleValueGenerator>
void NDPseudoRandomNumberGenerator<SingleValueGenerator>::setSeed(
    const size_t seedValue) {
  m_singleValueGen.setSeed(seedValue);
}

/// Generates the next point
template <typename SingleValueGenerator>
void NDPseudoRandomNumberGenerator<SingleValueGenerator>::generateNextPoint() {
  for (unsigned int i = 0; i < numberOfDimensions(); ++i) {
    this->cacheGeneratedValue(i, m_singleValueGen.nextValue());
  }
}

/**
 * Resets the underlying generator
 */
template <typename SingleValueGenerator>
void NDPseudoRandomNumberGenerator<SingleValueGenerator>::restart() {
  m_singleValueGen.restart();
}

/// Saves the current state of the generator
template <typename SingleValueGenerator>
void NDPseudoRandomNumberGenerator<SingleValueGenerator>::save() {
  m_singleValueGen.save();
}

/// Restores the generator to the last saved point, or the beginning if nothing
/// has been saved
template <typename SingleValueGenerator>
void NDPseudoRandomNumberGenerator<SingleValueGenerator>::restore() {
  m_singleValueGen.restore();
}
}
}

#endif /* MANTID_KERNEL_NDPSEUDORANDOMNUMBERGENERATOR_H_ */
