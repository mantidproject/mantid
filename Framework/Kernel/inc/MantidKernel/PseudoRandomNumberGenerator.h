#ifndef MANTID_KERNEL_PSEUDORANDOMNUMBERGENERATOR_H_
#define MANTID_KERNEL_PSEUDORANDOMNUMBERGENERATOR_H_
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
#include "MantidKernel/NDRandomNumberGenerator.h"

namespace Mantid {
namespace Kernel {
/**
 *
 * Defines a 1D pseudo-random number generator, i.e. a generator
 * that takes an initial seed and produces a set of numbers. It specialises
 * the interface for a general random number generator.
 */
class MANTID_KERNEL_DLL PseudoRandomNumberGenerator
    : public NDRandomNumberGenerator {
public:
  /// Default constructor setting the dimension to 1
  PseudoRandomNumberGenerator();

  /// Disable copy operator
  PseudoRandomNumberGenerator(const PseudoRandomNumberGenerator &) = delete;

  /// Disable assignment operator
  PseudoRandomNumberGenerator &
  operator=(const PseudoRandomNumberGenerator &) = delete;

  /// Set the random number seed
  virtual void setSeed(const size_t seedValue) = 0;
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

#endif /* MANTID_KERNEL_PSEUDORANDOMNUMBERGENERATOR_H_ */
