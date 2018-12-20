// Includes
#include "MantidKernel/MagneticFormFactorTable.h"

#include "MantidKernel/MagneticIon.h" // for MagneticIon

namespace Mantid {
namespace PhysicalConstants {

/**
 * Constructs the table for the given ion, J & L
 * @param length :: scattering length
 * @param ion :: A reference to the required ion
 */
MagneticFormFactorTable::MagneticFormFactorTable(const size_t length,
                                                 const MagneticIon &ion)
    : m_length(length), m_lookup(length, 0.0), m_delta(0.0) {
  setup(ion);
}

/**
 * Returns an interpolated form factor for the given \f$Q^2\f$ value
 * @param qsqr :: \f$Q^2\f$ in \f$\mbox{\AA}^{-2}\f$
 * @return The interpolated value
 */
double MagneticFormFactorTable::value(const double qsqr) const {
  const double nintervals = qsqr / m_delta;
  const size_t index = static_cast<size_t>(nintervals);
  if (index < m_length) {
    const double fraction = nintervals - static_cast<double>(index);
    return (1.0 - fraction) * m_lookup[index] + fraction * m_lookup[index + 1];
  } else
    return 0.0;
}

//-------------------------------------------------------------------------------------------------------
// Private methods
//-------------------------------------------------------------------------------------------------------

/**
 * Setup the (dipole approximation) form factor table with the
 * values for the given ion.
 * @param ion :: A reference to the required ion
 */
void MagneticFormFactorTable::setup(const MagneticIon &ion) {
  const size_t length = m_length;
  const double qsqrMax = MagneticIon::formFactorCutOff();
  m_delta = qsqrMax / static_cast<double>(length);

  for (size_t i = 0; i < length; ++i) {
    const double qsqr = m_delta * static_cast<double>(i);
    m_lookup[i] = ion.analyticalFormFactor(qsqr);
  }
}

} // namespace PhysicalConstants
} // namespace Mantid
