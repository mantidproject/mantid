/**
Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
#ifndef MANTID_CUSTOMINTERFACES_PERTHETADEFAUTSVALIDATOR_H_
#define MANTID_CUSTOMINTERFACES_PERTHETADEFAUTSVALIDATOR_H_
#include "PerThetaDefaults.h"
#include <boost/optional.hpp>
#include "ValidationResult.h"
#include "ParseReflectometryStrings.h"
#include "TransmissionRunPair.h"
#include "DllConfig.h"
namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL PerThetaDefaultsValidator {
public:
  static auto constexpr INPUT_FIELD_COUNT = 8;

  ValidationResult<PerThetaDefaults>
  operator()(std::array<std::string, INPUT_FIELD_COUNT> const &cellText);

private:
  boost::optional<double> parseTheta(std::array<std::string, INPUT_FIELD_COUNT> const &cellText);
  boost::optional<TransmissionRunPair>
  parseTransmissionRuns(std::array<std::string, INPUT_FIELD_COUNT> const &cellText);
  boost::optional<boost::optional<RangeInQ>>
  parseQRange(std::array<std::string, INPUT_FIELD_COUNT> const &cellText);
  boost::optional<boost::optional<double>>
  parseScaleFactor(std::array<std::string, INPUT_FIELD_COUNT> const &cellText);
  boost::optional<std::map<std::string, std::string>>
  parseOptions(std::array<std::string, INPUT_FIELD_COUNT> const &cellText);

  std::vector<int> m_invalidColumns;
};

ValidationResult<PerThetaDefaults>
validatePerThetaDefaults(std::array<std::string, PerThetaDefaultsValidator::INPUT_FIELD_COUNT> const &cellText);

}
}
#endif // MANTID_CUSTOMINTERFACES_PERTHETADEFAUTSVALIDATOR_H_
