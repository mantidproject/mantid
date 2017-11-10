#ifndef MANTID_KERNEL_MERSENNE_TWISTER_H_
#define MANTID_KERNEL_MERSENNE_TWISTER_H_

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidKernel/PseudoRandomNumberGenerator.h"

#ifndef Q_MOC_RUN
#include <boost/random/mersenne_twister.hpp>
#endif

namespace Mantid {
namespace Kernel {
/**
  This implements the the Mersenne Twister 19937 pseudo-random number
  generator algorithm as a specialzation of the PseudoRandomNumberGenerator
  interface.

  Further documentation can be found here:

  http://www.boost.org/doc/libs/1_42_0/libs/random/random-generators.html

  @author Martyn Gigg, Tessella plc
  @date 19/11/2007

  Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_KERNEL_DLL MersenneTwister final
    : public PseudoRandomNumberGenerator {

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
  /// Destructor
  ~MersenneTwister() override;

  MersenneTwister(const MersenneTwister &) = delete;
  MersenneTwister &operator=(const MersenneTwister &) = delete;

  /// Set the random number seed
  void setSeed(const size_t seedValue) override;
  /// Sets the range of the subsequent calls to next
  void setRange(const double start, const double end) override;
  /// Generate the next random number in the sequence within the given range
  /// default range
  inline double nextValue() override { return nextValue(m_start, m_end); }
  /// Generate the next random number in the sequence within the given range.
  double nextValue(double start, double end) override;
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
  /// The boost Mersenne Twister generator
  boost::mt19937 m_generator;
  /// Minimum in range
  double m_start;
  /// Maximum in range
  double m_end;
  /// The current seed
  boost::mt19937::result_type m_currentSeed;
  /// A generator that will take the value when save is requested. Pointer so
  /// that it is only instantiated when required
  boost::mt19937 *m_savedStateGenerator;
};
}
}

#endif
