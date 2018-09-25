#ifndef MANTID_CUSTOMINTERFACES_DETECTORCORRECTIONS_H_
#define MANTID_CUSTOMINTERFACES_DETECTORCORRECTIONS_H_
#include "../DllConfig.h"
namespace MantidQt {
namespace CustomInterfaces {

enum class DetectorCorrectionType { VerticalShift, RotateAroundSample };
  
class MANTIDQT_ISISREFLECTOMETRY_DLL DetectorCorrections {
public:
  DetectorCorrections(bool correctPositions,
                      DetectorCorrectionType correctionType);
  bool correctPositions() const;
  DetectorCorrectionType correctionType() const;
  
private:
  bool m_correctPositions;
  DetectorCorrectionType m_correctionType;
};
}
}
#endif // MANTID_CUSTOMINTERFACES_DETECTORCORRECTIONS_H_
