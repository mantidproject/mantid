// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidVatesAPI/CompositePeaksPresenterVsi.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include "MantidVatesAPI/PeaksPresenterVsi.h"
#include "MantidVatesAPI/ViewFrustum.h"
#include <map>
#include <vector>
namespace Mantid {
namespace VATES {

/**
 * Update the view frustum
 * @param frustum The view frustum
 */
void CompositePeaksPresenterVsi::updateViewFrustum(
    ViewFrustum_const_sptr frustum) {
  for (const auto &presenter : m_peaksPresenters) {
    presenter->updateViewFrustum(frustum);
  }
}

/**
 *Get the viewable peaks. Essentially copied from the slice viewer.
 *@returns A vector indicating which of the peaks are viewable.
 */
std::vector<bool> CompositePeaksPresenterVsi::getViewablePeaks() const {
  return std::vector<bool>();
}

/**
 * Get the name of all peaks workspaces as a vector
 * @returns A vector of all peaks workspace names.
 */
std::vector<std::string>
CompositePeaksPresenterVsi::getPeaksWorkspaceNames() const {
  std::vector<std::string> peaksWorkspaceNames;
  peaksWorkspaceNames.reserve(m_peaksPresenters.size());
  for (const auto &presenter : m_peaksPresenters) {
    peaksWorkspaceNames.emplace_back(presenter->getPeaksWorkspaceName());
  }
  return peaksWorkspaceNames;
}

/**
 * Extract the peak information regarding position and radius of the peak.
 * @param peaksWorkspace A pointer to the peaks workspace
 * @param row The selected row.
 * @param position A reference to extract the position.
 * @param radius A reference to extract the radius.
 * @param specialCoordinateSystem The coordinate system
 */
void CompositePeaksPresenterVsi::getPeaksInfo(
    Mantid::API::IPeaksWorkspace_sptr peaksWorkspace, int row,
    Mantid::Kernel::V3D &position, double &radius,
    Mantid::Kernel::SpecialCoordinateSystem specialCoordinateSystem) const {
  for (const auto &presenter : m_peaksPresenters) {
    if (presenter->getPeaksWorkspace() == peaksWorkspace) {
      presenter->getPeaksInfo(peaksWorkspace, row, position, radius,
                              specialCoordinateSystem);
    }
  }
}

/**
 * Get the frame in which the peak workspaces are evaluated. Note that all will
 * have the same frame, so only the first
 * workspace needs to be probed.
 * @returns The coordinate frame.
 */
std::string CompositePeaksPresenterVsi::getFrame() const {
  std::string frame;
  if (!m_peaksPresenters.empty())
    frame = m_peaksPresenters[0]->getFrame();
  return frame;
}

/**
 * Add a new peaks workspace presenter
 * @param presenter Add a new presenter to the composite.
 */
void CompositePeaksPresenterVsi::addPresenter(
    PeaksPresenterVsi_sptr presenter) {
  m_peaksPresenters.push_back(std::move(presenter));
}

/**
 * Get a vector with peak workspace pointers for which presenters exist.
 * @returns A vector with peaks workspace pointers
 */
std::vector<Mantid::API::IPeaksWorkspace_sptr>
CompositePeaksPresenterVsi::getPeaksWorkspaces() const {
  std::vector<Mantid::API::IPeaksWorkspace_sptr> peaksWorkspaces;
  peaksWorkspaces.reserve(m_peaksPresenters.size());
  for (const auto &presenter : m_peaksPresenters) {
    peaksWorkspaces.emplace_back(presenter->getPeaksWorkspace());
  }
  return peaksWorkspaces;
}

/**
 * Get the initialized viewable peaks. For each presenter return a vector with
 * true for each peak
 * @returns A vector of bool-vectors for each peaks presenter.
 */
std::map<std::string, std::vector<bool>>
CompositePeaksPresenterVsi::getInitializedViewablePeaks() const {
  std::map<std::string, std::vector<bool>> viewablePeaks;
  for (const auto &presenter : m_peaksPresenters) {
    viewablePeaks.emplace(
        presenter->getPeaksWorkspace()->getName(),
        std::vector<bool>(presenter->getPeaksWorkspace()->getNumberPeaks(),
                          true));
  }
  return viewablePeaks;
}

/**
 * Remove the presenters which are based on a certain peaks workspace.
 * @param peaksWorkspaceName
 */
void CompositePeaksPresenterVsi::removePresenter(
    const std::string &peaksWorkspaceName) {
  m_peaksPresenters.erase(
      std::remove_if(
          m_peaksPresenters.begin(), m_peaksPresenters.end(),
          [&peaksWorkspaceName](const PeaksPresenterVsi_sptr &presenter) {
            return presenter->getPeaksWorkspaceName() == peaksWorkspaceName;
          }),
      m_peaksPresenters.end());
}

/**
 * Update the presenters by checking if a presenter is still present, which is
 * not needed any longer.
 * @param peaksWorkspaceNames The names of all currently active peak sources.
 */
void CompositePeaksPresenterVsi::updateWorkspaces(
    const std::vector<std::string> &peaksWorkspaceNames) {
  std::vector<std::string> storedPeaksWorkspaces = getPeaksWorkspaceNames();
  for (const auto &ws : storedPeaksWorkspaces) {
    size_t count =
        std::count(peaksWorkspaceNames.begin(), peaksWorkspaceNames.end(), ws);
    if (count == 0) {
      removePresenter(ws);
    }
  }
}

/**
 * Check if there are any peaks availble.
 * @returns If there are any peaks availbale.
 */
bool CompositePeaksPresenterVsi::hasPeaks() {
  return !m_peaksPresenters.empty();
}

/**
 * Sort the peak workpace by the specified column
 * @param columnToSortBy The column by which the workspace is to be sorted.
 * @param sortedAscending If the column is to be sorted in ascending or
 * descending
 * order.
 * @param peaksWS The peak workspace which is being sorted.
 */
void CompositePeaksPresenterVsi::sortPeaksWorkspace(
    const std::string &columnToSortBy, const bool sortedAscending,
    const Mantid::API::IPeaksWorkspace_sptr peaksWS) {
  for (const auto &presenter : m_peaksPresenters) {
    if (presenter->getPeaksWorkspace() == peaksWS) {
      presenter->sortPeaksWorkspace(columnToSortBy, sortedAscending);
    }
  }
}
} // namespace VATES
} // namespace Mantid
