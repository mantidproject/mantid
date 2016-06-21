//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IMDIterator.h"

namespace Mantid {
namespace API {

/** Default constructor */
IMDIterator::IMDIterator()
    : m_normalization(Mantid::API::VolumeNormalization) {}

/** Set how the signal will be normalized when calling getNormalizedSignal()
 *
 * @param normalization :: method to use
 */
void IMDIterator::setNormalization(Mantid::API::MDNormalization normalization) {
  m_normalization = normalization;
}

/** @return how the signal will be normalized when calling getNormalizedSignal()
 */
Mantid::API::MDNormalization IMDIterator::getNormalization() const {
  return m_normalization;
}

} // namespace API
} // namespace Mantid
