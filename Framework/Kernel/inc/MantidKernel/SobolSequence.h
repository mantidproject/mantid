#ifndef MANTID_KERNEL_SOBOLSEQUENCE_H_
#define MANTID_KERNEL_SOBOLSEQUENCE_H_
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
#include "MantidKernel/QuasiRandomNumberSequence.h"
#include <gsl/gsl_qrng.h>

//------------------------------------------------------------------------------
// Forward declarations
//------------------------------------------------------------------------------

namespace Mantid {
namespace Kernel {
/**
 *
 * Defines a generator that produces quasi-random numbers according
 * to a Sobol sequence http://en.wikipedia.org/wiki/Sobol_sequence
 */
class MANTID_KERNEL_DLL SobolSequence : public QuasiRandomNumberSequence {
public:
  /// Constructor taking the number of dimensions
  explicit SobolSequence(const unsigned int ndims);
  /// Destructor
  ~SobolSequence() override;

  /// Disable default constructor
  SobolSequence() = delete;

  /// Disable copy operator
  SobolSequence(const SobolSequence &) = delete;

  /// Disable assignment operator
  SobolSequence &operator=(const SobolSequence &) = delete;

  /// Generates the next value in the sequence
  void generateNextPoint() override;
  /// Reset the sequence
  void restart() override;
  /// Saves the current state of the generator
  void save() override;
  /// Restores the generator to the last saved point, or the beginning if
  /// nothing has been saved
  void restore() override;

private:
  /// Set the number of dimensions
  void setNumberOfDimensions(const unsigned int ndims);
  /// Frees resources allocated by current generator
  void deleteCurrentGenerator();

  /// GSL quasi-random number state generator
  gsl_qrng *m_gslGenerator;
  /// Allocated object for save state calls
  gsl_qrng *m_savedGenerator;
};
}
}

#endif /* MANTID_KERNEL_SOBOLSEQUENCE_H_ */
