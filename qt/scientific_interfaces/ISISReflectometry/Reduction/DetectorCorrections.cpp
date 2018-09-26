#include "DetectorCorrections.h"
namespace MantidQt {
namespace CustomInterfaces {

DetectorCorrections::DetectorCorrections(bool correctPositions,
                                         DetectorCorrectionType correctionType)
    : m_correctPositions(correctPositions), m_correctionType(correctionType) {}

bool DetectorCorrections::correctPositions() const {
  return m_correctPositions;
}

DetectorCorrectionType DetectorCorrections::correctionType() const {
  return m_correctionType;
}
} // namespace CustomInterfaces
} // namespace MantidQt
