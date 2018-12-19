// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_SOBOLSEQUENCE_H_
#define MANTID_KERNEL_SOBOLSEQUENCE_H_

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
} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_SOBOLSEQUENCE_H_ */
