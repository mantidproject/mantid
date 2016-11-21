#ifndef MANTID_KERNEL_NORMAL_DISTRIBUTION_H_
#define MANTID_KERNEL_NORMAL_DISTRIBUTION_H_

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/MersenneTwister.h"

#include <boost/random/normal_distribution.hpp>

namespace Mantid {
namespace Kernel {

/**
  This implements a generator of normally distributed pseudo-random numbers.

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
class MANTID_KERNEL_DLL NormalDistribution {

public:
  /// Construct the generator using time stamp for the initial seed.
  NormalDistribution();
  /// Construct the generator with initial distribution parameters
  /// and default seed.
  NormalDistribution(const double mean, const double sigma);
  /// Construct the generator with initial distribution parameters and
  /// a seed value.
  NormalDistribution(const size_t seedValue, const double mean,
                     const double sigma);

  NormalDistribution(const NormalDistribution &) = delete;
  NormalDistribution &operator=(const NormalDistribution &) = delete;

  /// Set the random number seed
  void setSeed(const size_t seedValue);
  /// Generate the next random number in the sequence
  double nextValue();
  /// Get the mean of the distribution
  double mean() const { return m_generator.mean(); }
  /// Get the sigma of the distribution
  double sigma() const { return m_generator.sigma(); }
  /// Generate a random number from a distribution with given mean and sigma
  double randomValue(double argMean, double argSigma);

private:
  /// The boost Mersenne Twister generator
  /// (In the future when we have other uniform generators this can be a ref
  /// to a PseudoRandomNumberGenerator base class and the user can initialize it
  /// with an implementation of choise)
  MersenneTwister m_uniform_generator;
  /// The boost normal distribution generator
  boost::normal_distribution<double> m_generator;
};
}
}

#endif // MANTID_KERNEL_NORMAL_DISTRIBUTION_H_
