// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "Autoreduction.h"
#include "GUI/Runs/IRunsView.h"
#include "GUI/Runs/RunsPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {

Autoreduction::Autoreduction()
    : m_running(false), m_searchResultsExist(false) {}

/** Check whether autoreduction is currently running
 */
bool Autoreduction::running() const { return m_running; }

/** Return true if the given search string is different from when
 * autoreduction was started
 */
bool Autoreduction::searchStringChanged(
    const std::string &newSearchString) const {
  return m_searchString != newSearchString;
}

/** Check whether search results list has been created yet
 */
bool Autoreduction::searchResultsExist() const { return m_searchResultsExist; }

/** Set the flag to indicate search results list has been created for the first
 * run through for this autoreduction process. On subsequent runs, the existing
 * search results will be updated, rather than being re-populated
 */
void Autoreduction::setSearchResultsExist() { m_searchResultsExist = true; }

/** Initialise a new autoreduction
 *
 * @param searchString : the search string to use for finding runs
 * @return : true if started
 */
void Autoreduction::setupNewAutoreduction(const std::string &searchString) {
  m_searchString = searchString;
  m_running = true;
  m_searchResultsExist = false;
}

/** Stop an autoreduction
 * @return : true if stopped
 */
bool Autoreduction::pause() {
  // If autoreduction is already stopped then return success
  if (!m_running)
    return true;

  m_running = false;
  return true;
}

/** Stop autoreduction
 */
void Autoreduction::stop() { m_running = false; }
} // namespace CustomInterfaces
} // namespace MantidQt
