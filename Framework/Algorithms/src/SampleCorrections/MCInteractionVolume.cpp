#include "MantidAlgorithms/SampleCorrections/MCInteractionVolume.h"
#include "MantidAPI/Sample.h"
#include "MantidGeometry/Objects/Object.h"

namespace Mantid {
namespace Algorithms {

/**
 * Construct the volume with only a sample object
 * @param sample A reference to a sample object that defines a valid shape
 * & material
 */
MCInteractionVolume::MCInteractionVolume(const API::Sample &sample)
    : m_sample(sample.getShape()) {
  if (!m_sample.hasValidShape()) {
    throw std::invalid_argument(
        "MCInteractionVolume() - Sample shape does not have a valid shape.");
  }
}

} // namespace Algorithms
} // namespace Mantid
