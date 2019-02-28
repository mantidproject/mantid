// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_POLARIZATIONCORRECTIONS_H_
#define MANTID_CUSTOMINTERFACES_POLARIZATIONCORRECTIONS_H_
#include "Common/DllConfig.h"
#include <boost/optional.hpp>
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

inline std::string
polarizationCorrectionTypeToString(PolarizationCorrectionType correctionType) {
  switch (correctionType) {
  case PolarizationCorrectionType::None:
    return "None";
  case PolarizationCorrectionType::PA:
    return "PA";
  case PolarizationCorrectionType::PNR:
    return "PNR";
  case PolarizationCorrectionType::ParameterFile:
    return "ParameterFile";
  }
  throw std::runtime_error("Unexpected polarization correction type.");
}

inline bool polarizationCorrectionRequiresInputs(
    PolarizationCorrectionType correctionType) {
  return (correctionType == PolarizationCorrectionType::PA ||
          correctionType == PolarizationCorrectionType::PNR);
}

/** @class PoliarizationCorrections

    The PoliarizationCorrections model holds information about what polarization
    corrections should be done during reduction
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL PolarizationCorrections {
public:
  PolarizationCorrections(PolarizationCorrectionType correctionType,
                          boost::optional<double> CRho = boost::none,
                          boost::optional<double> CAlpha = boost::none,
                          boost::optional<double> CAp = boost::none,
                          boost::optional<double> CPp = boost::none);

  PolarizationCorrectionType correctionType() const;
  boost::optional<double> cRho() const;
  boost::optional<double> cAlpha() const;
  boost::optional<double> cAp() const;
  boost::optional<double> cPp() const;

private:
  PolarizationCorrectionType m_correctionType;
  boost::optional<double> m_cRho;
  boost::optional<double> m_cAlpha;
  boost::optional<double> m_cAp;
  boost::optional<double> m_cPp;
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
