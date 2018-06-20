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
#ifndef MANTID_CUSTOMINTERFACES_REDUCTIONWORKSPACES_H_
#define MANTID_CUSTOMINTERFACES_REDUCTIONWORKSPACES_H_

#include <vector>
#include <boost/optional.hpp>
#include <boost/algorithm/string/join.hpp>
#include <string>
#include <unordered_map>
#include "../DllConfig.h"

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL ReductionWorkspaces {
public:
  ReductionWorkspaces(std::vector<std::string> timeOfFlight,
                      std::pair<std::string, std::string> transmissionRuns,
                      std::string combinedTransmissionRuns,
                      std::string iVsLambda, std::string iVsQ,
                      std::string iVsQBinned);

  std::vector<std::string> const &timeOfFlight() const;
  std::pair<std::string, std::string> const &transmissionRuns() const;
  std::string const &combinedTransmissionRuns() const;
  std::string const &iVsLambda() const;
  std::string const &iVsQ() const;
  std::string const &iVsQBinned() const;

private:
  std::vector<std::string> m_timeOfFlight;
  std::pair<std::string, std::string> m_transmissionRuns;
  std::string m_combinedTransmissionRuns;
  std::string m_iVsLambda;
  std::string m_iVsQ;
  std::string m_iVsQBinned;
};

MANTIDQT_ISISREFLECTOMETRY_DLL bool operator==(ReductionWorkspaces const &lhs,
                                               ReductionWorkspaces const &rhs);
MANTIDQT_ISISREFLECTOMETRY_DLL bool operator!=(ReductionWorkspaces const &lhs,
                                               ReductionWorkspaces const &rhs);

std::pair<std::string, std::string> transmissionWorkspaceNames(
    std::pair<std::string, std::string> const &transmissionRuns);

std::string transmissionWorkspacesCombined(
    std::pair<std::string, std::string> const &transmissionRuns);

MANTIDQT_ISISREFLECTOMETRY_DLL ReductionWorkspaces workspaceNamesForUnsliced(
    std::vector<std::string> const &summedRunNumbers,
    std::pair<std::string, std::string> const &transmissionRuns);

MANTIDQT_ISISREFLECTOMETRY_DLL std::string
postprocessedWorkspaceNameForUnsliced(
    std::vector<std::vector<std::string> const *> const &summedRunNumbers);
}
}
#endif // MANTID_CUSTOMINTERFACES_REDUCTIONWORKSPACES_H_
