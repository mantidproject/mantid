#include "MantidAPI/ScopedWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"

namespace Mantid {
namespace API {

const size_t ScopedWorkspace::NAME_LENGTH = 16;

//----------------------------------------------------------------------------------------------
/**
 * Empty constructor
 */
ScopedWorkspace::ScopedWorkspace() : m_name(generateUniqueName()) {}

/**
 * Workspace constructor
 */
ScopedWorkspace::ScopedWorkspace(Workspace_sptr ws)
    : m_name(generateUniqueName()) {
  set(ws);
}

//----------------------------------------------------------------------------------------------
/**
 * Destructor
 */
ScopedWorkspace::~ScopedWorkspace() { remove(); }

/**
 * Operator for conversion to boolean. Returns true if workspace was created for
 * the
 * name and it is not null workspace.
 */
ScopedWorkspace::operator bool() const {
  return AnalysisDataService::Instance().doesExist(m_name);
}

/**
 * Retrieve workspace from the ADS. Null pointer returned if nothing was added
 * under the name.
 */
Workspace_sptr ScopedWorkspace::retrieve() const {
  AnalysisDataServiceImpl &ads = AnalysisDataService::Instance();

  if (ads.doesExist(m_name)) {
    return ads.retrieveWS<Workspace>(m_name);
  }

  return Workspace_sptr();
}

/**
 * Removes the workspace entry from the ADS.
 */
void ScopedWorkspace::remove() {
  AnalysisDataServiceImpl &ads = AnalysisDataService::Instance();

  // When destructed, remove workspace from the ADS if was added and still
  // exists
  if (ads.doesExist(m_name)) {
    if (ads.retrieveWS<WorkspaceGroup>(m_name)) {
      // If is a group, need to remove all the members as well
      ads.deepRemoveGroup(m_name);
    } else {
      ads.remove(m_name);
    }
  }
}

/**
 * Make ADS entry to point to the given workspace.
 */
void ScopedWorkspace::set(Workspace_sptr newWS) {
  AnalysisDataServiceImpl &ads = AnalysisDataService::Instance();

  if (!newWS->name().empty() && ads.doesExist(newWS->name()))
    throw std::invalid_argument(
        "Workspace is already in the ADS under the name " + newWS->name());

  // Remove previous workspace entry
  remove();

  ads.add(m_name, newWS);
}

/**
 * Generates a tricky name which is unique within ADS.
 */
std::string ScopedWorkspace::generateUniqueName() {
  std::string newName;

  do {
    // __ makes it hidden in the MantidPlot
    newName = "__ScopedWorkspace_" + randomString(NAME_LENGTH);
  } while (AnalysisDataService::Instance().doesExist(newName));

  return newName;
}

/**
 * Generates random alpha-numeric string.
 * @param len :: Length of the string
 * @return Random string of the given length
 */
std::string ScopedWorkspace::randomString(size_t len) {
  static const std::string alphabet = "0123456789abcdefghijklmnopqrstuvwxyz";

  std::string result;
  result.reserve(len);

  while (result.size() != len) {
    size_t randPos = ((rand() % (alphabet.size() - 1)));
    result.push_back(alphabet[randPos]);
  }

  return result;
}

} // namespace API
} // namespace Mantid
