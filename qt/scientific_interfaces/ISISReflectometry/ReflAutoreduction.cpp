#include "ReflAutoreduction.h"
#include "IReflMainWindowPresenter.h"
#include "IReflRunsTabView.h"
#include "ReflRunsTabPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {

ReflAutoreduction::ReflAutoreduction()
    : m_running(false), m_searchResultsExist(false) {}

/** Check whether autoreduction is currently running
 */
bool ReflAutoreduction::running() const { return m_running; }

/** Return true if the given search string is different from when
 * autoreduction was started
 */
bool ReflAutoreduction::searchStringChanged(
    const std::string &newSearchString) const {
  return m_searchString != newSearchString;
}

/** Check whether search results list has been created yet
 */
bool ReflAutoreduction::searchResultsExist() const {
  return m_searchResultsExist;
}

/** Set the flag to indicate search results list has been created for the first
 * run through for this autoreduction process. On subsequent runs, the existing
 * search results will be updated, rather than being re-populated
 */
void ReflAutoreduction::setSearchResultsExist() { m_searchResultsExist = true; }

/** Initialise a new autoreduction on the given group
 *
 * @param group : the index of which group to start the reduction on
 * @param searchString : the search string to use for finding runs
 * @return : true if started
 */
bool ReflAutoreduction::setupNewAutoreduction(const std::string &searchString) {
  m_searchString = searchString;
  m_running = true;
  m_searchResultsExist = false;
  return true;
}

/** Stop an autoreduction for a given group
 * @param group : the group to stop autoreduction for
 * @return : true if stopped
 */
bool ReflAutoreduction::pause() {
  // If autoreduction is already stopped then return success
  if (!m_running)
    return true;

  m_running = false;
  return true;
}

/** Stop autoreduction on any group for which it is running
 */
void ReflAutoreduction::stop() { m_running = false; }
} // namespace CustomInterfaces
} // namespace MantidQt
