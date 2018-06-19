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

MANTIDQT_ISISREFLECTOMETRY_DLL std::string postprocessedWorkspaceNameForUnsliced(
    std::vector<std::vector<std::string> const*> const &summedRunNumbers);

}
}
#endif // MANTID_CUSTOMINTERFACES_REDUCTIONWORKSPACES_H_
