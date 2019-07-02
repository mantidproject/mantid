// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_FLOODCORRECTIONS_H_
#define MANTID_CUSTOMINTERFACES_FLOODCORRECTIONS_H_
#include "Common/DllConfig.h"
#include <boost/optional.hpp>
#include <string>
namespace MantidQt {
namespace CustomInterfaces {
enum class FloodCorrectionType { Workspace, ParameterFile };

inline FloodCorrectionType
floodCorrectionTypeFromString(std::string const &correctionType) {
  if (correctionType == "Workspace")
    return FloodCorrectionType::Workspace;
  else if (correctionType == "ParameterFile")
    return FloodCorrectionType::ParameterFile;
  else
    throw std::invalid_argument("Unexpected flood correction type.");
}

inline std::string
floodCorrectionTypeToString(FloodCorrectionType correctionType) {
  switch (correctionType) {
  case FloodCorrectionType::Workspace:
    return "Workspace";
  case FloodCorrectionType::ParameterFile:
    return "ParameterFile";
  }
  throw std::invalid_argument("Unexpected flood correction type.");
}

inline bool floodCorrectionRequiresInputs(FloodCorrectionType correctionType) {
  return (correctionType == FloodCorrectionType::Workspace);
}

/** @class FloodCorrections

    The FloodCorrections model holds information about what type of flood
    corrections should be performed in the reduction
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL FloodCorrections {
public:
  FloodCorrections(FloodCorrectionType correctionType,
                   boost::optional<std::string> workspace = boost::none);

  FloodCorrectionType correctionType() const;
  boost::optional<std::string> workspace() const;

private:
  FloodCorrectionType m_correctionType;
  boost::optional<std::string> m_workspace;
};

MANTIDQT_ISISREFLECTOMETRY_DLL bool operator==(FloodCorrections const &lhs,
                                               FloodCorrections const &rhs);
MANTIDQT_ISISREFLECTOMETRY_DLL bool operator!=(FloodCorrections const &lhs,
                                               FloodCorrections const &rhs);
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACES_FLOODCORRECTIONS_H_
