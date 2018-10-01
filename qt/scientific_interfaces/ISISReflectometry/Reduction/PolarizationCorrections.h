#ifndef MANTID_CUSTOMINTERFACES_POLARIZATIONCORRECTIONS_H_
#define MANTID_CUSTOMINTERFACES_POLARIZATIONCORRECTIONS_H_
#include "../DllConfig.h"
#include <stdexcept>
#include <string>
namespace MantidQt {
namespace CustomInterfaces {
enum class PolarizationCorrectionType { None, PA, PNR, ParameterFile };

inline PolarizationCorrectionType
polarizationCorrectionTypeFromString(std::string const &correctionType) {
  if (correctionType == "None")
    return PolarizationCorrectionType::None;
  else if (correctionType == "PA")
    return PolarizationCorrectionType::PA;
  else if (correctionType == "PNR")
    return PolarizationCorrectionType::PNR;
  else if (correctionType == "ParameterFile")
    return PolarizationCorrectionType::ParameterFile;
  else
    throw std::runtime_error("Unexpected polarization correction type.");
}

class MANTIDQT_ISISREFLECTOMETRY_DLL PolarizationCorrections {
public:
  PolarizationCorrections(PolarizationCorrectionType correctionType,
                          double CRho, double CAlpha, double CAp, double CPp);

  PolarizationCorrectionType correctionType();
  double cRho() const;
  double cAlpha() const;
  double cAp() const;
  double cPp() const;
  bool enableInputs() const;

private:
  PolarizationCorrectionType m_correctionType;
  double m_cRho;
  double m_cAlpha;
  double m_cAp;
  double m_cPp;
};

MANTIDQT_ISISREFLECTOMETRY_DLL bool
operator==(PolarizationCorrections const &lhs,
           PolarizationCorrections const &rhs);
MANTIDQT_ISISREFLECTOMETRY_DLL bool
operator!=(PolarizationCorrections const &lhs,
           PolarizationCorrections const &rhs);
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACES_POLARIZATIONCORRECTIONS_H_
