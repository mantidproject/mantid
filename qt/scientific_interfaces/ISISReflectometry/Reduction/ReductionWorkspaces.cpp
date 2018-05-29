#include "ReductionWorkspaces.h"
namespace MantidQt {
namespace CustomInterfaces {
ReductionWorkspaces::ReductionWorkspaces(
    std::vector<std::string> timeOfFlight,
    std::pair<std::string, std::string> transmissionRuns, std::string iVsLambda,
    std::string iVsQ, std::string iVsQBinned)
    : m_timeOfFlight(std::move(timeOfFlight)),
      m_transmissionRuns(std::move(transmissionRuns)),
      m_iVsLambda(std::move(iVsLambda)), m_iVsQ(std::move(iVsQ)),
      m_iVsQBinned(std::move(iVsQBinned)) {}

std::vector<std::string> const& ReductionWorkspaces::timeOfFlight() const {
  return m_timeOfFlight;
}

std::pair<std::string, std::string> const& ReductionWorkspaces::transmissionRuns() const {
  return m_transmissionRuns;
}

std::string const& ReductionWorkspaces::iVsLambda() const {
  return m_iVsLambda;
}

std::string const& ReductionWorkspaces::iVsQ() const {
  return m_iVsQ;
}

std::string const& ReductionWorkspaces::iVsQBinned() const {
  return m_iVsQBinned;
}

bool operator==(ReductionWorkspaces const &lhs,
                ReductionWorkspaces const &rhs) {
  return lhs.timeOfFlight() == rhs.timeOfFlight() &&
         lhs.transmissionRuns() == rhs.transmissionRuns() &&
         lhs.iVsLambda() == rhs.iVsLambda() && lhs.iVsQ() == rhs.iVsQ() &&
         lhs.iVsQBinned() == rhs.iVsQBinned();
}

bool operator!=(ReductionWorkspaces const &lhs,
                ReductionWorkspaces const &rhs) {
  return !(lhs == rhs);
}
}
}
