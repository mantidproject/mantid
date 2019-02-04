// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_MAGNETICFORMFACTORTABLE_H_
#define MANTID_KERNEL_MAGNETICFORMFACTORTABLE_H_

#include "MantidKernel/DllConfig.h"

#include <vector>

namespace Mantid {
namespace PhysicalConstants {

// Forward declare
struct MagneticIon;

/**
  Implements a cached lookup table for a magnetic form factor. The table is
  created for a given MagneticIon.
 */
class MANTID_KERNEL_DLL MagneticFormFactorTable {
public:
  /// Construct the table around an ion
  MagneticFormFactorTable(const size_t length, const MagneticIon &ion);

  /// Disable default constructor
  MagneticFormFactorTable() = delete;

  /// Disable copy operator
  MagneticFormFactorTable(const MagneticFormFactorTable &) = delete;

  /// Disable assignment operator
  MagneticFormFactorTable &operator=(const MagneticFormFactorTable &) = delete;

  /// Returns an interpolated form factor for the given \f$Q^2\f$ value
  double value(const double qsqr) const;

private:
  /// Setup the table with the values
  void setup(const MagneticIon &ion);

  /// Cache the size
  size_t m_length;
  /// The actual table of values
  std::vector<double> m_lookup;
  /// Point spacing
  double m_delta;
};

} // namespace PhysicalConstants
} // namespace Mantid

#endif /* MANTID_KERNEL_MAGNETICFORMFACTORTABLE_H_ */
