#include "ReflAutoreduction.h"
#include "IReflMainWindowPresenter.h"
#include "IReflRunsTabView.h"
#include "ReflRunsTabPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {

ReflAutoreduction::ReflAutoreduction() : m_running(false), m_group(0) {}

/** Check whether autoreduction is currently running
 */
bool ReflAutoreduction::running() const { return m_running; }

/** Get the group that autoreduction is running for
 */
int ReflAutoreduction::group() const { return m_group; }

/** Return true if the given search string is different from when
 * autoreduction was started
 */
bool ReflAutoreduction::searchStringChanged(
    const std::string &newSearchString) const {
  return m_searchString != newSearchString;
}

/** Start an autoreduction on the given group
 *
 * @param group : the index of which group to start the reduction on
 * @param searchString : the search string to use for finding runs
 * @return : true if started
 */
bool ReflAutoreduction::start(const int group,
                              const std::string &searchString) {
  m_group = group;
  m_searchString = searchString;
  m_running = true;
  return true;
}

/** Stop an autoreduction for a given group
 * @param group : the group to stop autoreduction for
 * @return : true if stopped
 */
bool ReflAutoreduction::stop(int group) {
  // Currently there can only be one autoreduction running so do nothing if
  // the group doesn't match
  if (group != m_group)
    return false;

  m_running = false;
  return true;
}

/** Stop an autoreduction if there is one in progress
 * @return : true if stopped
 */
bool ReflAutoreduction::stop() {
  m_running = false;
  return true;
}
}
}
