// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IMDIterator.h"

namespace Mantid {
namespace API {

/** Default constructor */
IMDIterator::IMDIterator() : m_normalization(Mantid::API::VolumeNormalization) {}

/** Set how the signal will be normalized when calling getNormalizedSignal()
 *
 * @param normalization :: method to use
 */
void IMDIterator::setNormalization(Mantid::API::MDNormalization normalization) { m_normalization = normalization; }

/** @return how the signal will be normalized when calling getNormalizedSignal()
 */
Mantid::API::MDNormalization IMDIterator::getNormalization() const { return m_normalization; }

} // namespace API
} // namespace Mantid
