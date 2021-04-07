// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/LiveListener.h"
#include "MantidAPI/IAlgorithm.h"

namespace Mantid {
namespace API {

/// @copydoc ILiveListener::dataReset
bool LiveListener::dataReset() {
  const bool retval = m_dataReset;
  // Should this be done here or should extractData do it?
  m_dataReset = false;
  return retval;
}

/**
 * Default behaviour reads all spectrum numbers
 * @param specList :: A vector with spectra numbers (ignored)
 */
void LiveListener::setSpectra(const std::vector<specnum_t> &specList) { UNUSED_ARG(specList); }

/**
 * Default behaviour updates property values on Listener
 * using those on calling algorithm.
 */
void LiveListener::setAlgorithm(const IAlgorithm &callingAlgorithm) { this->updatePropertyValues(callingAlgorithm); }

} // namespace API
} // namespace Mantid
