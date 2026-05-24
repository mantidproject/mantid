// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX-License-Identifier: GPL-3.0+
#include "MantidAPI/LiveListener.h"
#include "MantidAPI/IAlgorithm.h"

namespace Mantid::API {

/** Template-method body. See header for the full contract.
 *  `onBeforeExtract()` runs first; if it throws, later phases are skipped.
 *  `onAfterExtract()` runs only after `doExtractData()` returns normally.
 */
std::shared_ptr<Workspace> LiveListener::extractData() {
  onBeforeExtract();
  auto workspace = doExtractData();
  onAfterExtract();
  return workspace;
}

/// Default no-op hook; subclasses override to perform per-extract work.
void LiveListener::onBeforeExtract() {}

/// Default no-op hook; subclasses override for success-only post-extract work.
void LiveListener::onAfterExtract() {}

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

} // namespace Mantid::API
