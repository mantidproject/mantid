#ifndef MANTID_CUSTOMINTERFACES_REDUCTIONWORKSPACES_H_
#define MANTID_CUSTOMINTERFACES_REDUCTIONWORKSPACES_H_

#include <vector>
#include <boost/optional.hpp>
#include <string>
#include <unordered_map>
#include "../DllConfig.h"

namespace MantidQt {
namespace CustomInterfaces {

class ReductionWorkspaces {
public:
  ReductionWorkspaces(std::vector<std::string> timeOfFlight,
                      std::pair<std::string, std::string> transmissionRuns,
                      std::string iVsLambda, std::string iVsQ,
                      std::string iVsQBinned);

  std::vector<std::string> const& timeOfFlight() const;
  std::pair<std::string, std::string> const& transmissionRuns() const;
  std::string const& iVsLambda() const;
  std::string const& iVsQ() const;
  std::string const& iVsQBinned() const;

private:
  std::vector<std::string> m_timeOfFlight;
  std::pair<std::string, std::string> m_transmissionRuns;
  std::string m_iVsLambda;
  std::string m_iVsQ;
  std::string m_iVsQBinned;
};

MANTIDQT_ISISREFLECTOMETRY_DLL bool operator==(ReductionWorkspaces const &lhs,
                                               ReductionWorkspaces const &rhs);
MANTIDQT_ISISREFLECTOMETRY_DLL bool operator!=(ReductionWorkspaces const &lhs,
                                               ReductionWorkspaces const &rhs);
}
}
#endif // MANTID_CUSTOMINTERFACES_REDUCTIONWORKSPACES_H_
