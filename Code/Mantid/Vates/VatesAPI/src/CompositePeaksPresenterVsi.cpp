#include "MantidVatesAPI/CompositePeaksPresenterVsi.h"
#include "MantidVatesAPI/PeaksPresenterVsi.h"
#include "MantidVatesAPI/ViewFrustum.h"
#include "MantidAPI/IPeaksWorkspace.h"

#include <vector>
#include <map>
namespace Mantid
{
namespace VATES
{
  /// Constructor
  CompositePeaksPresenterVsi::CompositePeaksPresenterVsi(){}                                                                                                  

  /// Destructor
  CompositePeaksPresenterVsi::~CompositePeaksPresenterVsi(){}

  /**
   * Update the view frustum
   * @param frustum The view frustum
   */
  void CompositePeaksPresenterVsi::updateViewFrustum(ViewFrustum frustum)
  {
    for (std::vector<PeaksPresenterVsi_sptr>::iterator it = m_peaksPresenters.begin(); it != m_peaksPresenters.end(); ++it) {
      (*it)->updateViewFrustum(frustum);
    }
  }

    /**
   * Get the viewable peaks. Essentially copied from the slice viewer.
   * @retruns A vector indicating which of the peaks are viewable.
   */
  std::vector<bool> CompositePeaksPresenterVsi::getViewablePeaks()
  {
    return std::vector<bool>();
  }

  /**
   * Get the name of all peaks workspaces as a vector
   * @returns A vector of all peaks workspace names.
   */
  std::vector<std::string> CompositePeaksPresenterVsi::getPeaksWorkspaceNames() {
    std::vector<std::string> peaksWorkspaceNames;
    for (std::vector<PeaksPresenterVsi_sptr>::iterator it = m_peaksPresenters.begin(); it != m_peaksPresenters.end(); ++it) {
      peaksWorkspaceNames.push_back((*it)->getPeaksWorkspaceName());
    }
    return peaksWorkspaceNames;
  }

  /**
   * Extract the peak information regarding position and radius of the peak.
   * @param peaksWorkspace A pointer to the peaks workspace
   * @param position A reference to extract the position.
   * @param radius A reference to extract the radius.
   */
  void CompositePeaksPresenterVsi::getPeaksInfo(Mantid::API::IPeaksWorkspace_sptr peaksWorkspace, int row, Mantid::Kernel::V3D& position, double& radius) {
    for (std::vector<PeaksPresenterVsi_sptr>::iterator it = m_peaksPresenters.begin(); it != m_peaksPresenters.end(); ++it) {
      if ((*it)->getPeaksWorkspace() == peaksWorkspace) {
        (*it)->getPeaksInfo(peaksWorkspace, row, position, radius);
      }
    }
  }

  /**
   * Get the frame in which the peak workspaces are evaluated. Note that all will have the same frame, so only the first
   * workspace needs to be probed.
   * @returns The coordinate frame.
   */
  std::string CompositePeaksPresenterVsi::getFrame() {
    if (m_peaksPresenters.empty())
      return std::string;
    else
      return m_peaksPresenters[0]->getFrame();
  }

  /**
   * Add a new peaks workspace presenter
   * @param presenter Add a new presenter to the composite.
   */
  void CompositePeaksPresenterVsi::addPresenter(PeaksPresenterVsi_sptr presenter) {
    m_peaksPresenters.push_back(presenter);
  }

  /**
   * Get a vector with peak workspace pointers for which presenters exist.
   * @returns A vector with peaks workspace pointers
   */
  std::vector<Mantid::API::IPeaksWorkspace_sptr> CompositePeaksPresenterVsi::getPeaksWorkspaces() {
    std::vector<Mantid::API::IPeaksWorkspace_sptr> peaksWorkspaces;
    for (std::vector<PeaksPresenterVsi_sptr>::iterator it = m_peaksPresenters.begin(); it != m_peaksPresenters.end(); ++it) {
      peaksWorkspaces.push_back((*it)->getPeaksWorkspace());
    }
    return peaksWorkspaces;
  }

  /**
   * Get the initialized viewable peaks. For each presenter return a vector with true for each peak
   * @returns A vector of bool-vectors for each peaks presenter.
   */
  std::map<std::string, std::vector<bool>> CompositePeaksPresenterVsi::getInitializedViewablePeaks(){
    std::map<std::string, std::vector<bool>> viewablePeaks;
    for (std::vector<PeaksPresenterVsi_sptr>::iterator it = m_peaksPresenters.begin(); it != m_peaksPresenters.end(); ++it) {
      viewablePeaks.insert(std::pair<std::string, std::vector<bool>>((*it)->getPeaksWorkspace()->getName(), std::vector<bool>((*it)->getPeaksWorkspace()->getNumberPeaks(), true)));
    }
    return viewablePeaks;
  }

  /**
   * Remove the presenters which are based on a certain peaks workspace.
   * @param peaksWorkspaceName
   */
  void CompositePeaksPresenterVsi::removePresenter(std::string peaksWorkspaceName) {
    std::vector<PeaksPresenterVsi_sptr>::iterator it = m_peaksPresenters.begin();
    for (; it != m_peaksPresenters.end();) {
      if ((*it)->getPeaksWorkspaceName() == peaksWorkspaceName) {
        it = m_peaksPresenters.erase(it);
      } else {
        ++it;
      }
    }
  }

  /**
   * Update the presenters by checking if a presenter is still present, which is not needed any longer.
   * @param peaksWorkspaceNames The names of all currently active peak sources.
   */
  void CompositePeaksPresenterVsi::updateWorkspaces(std::vector<std::string>peaksWorkspaceNames) {
    std::vector<std::string> storedPeaksWorkspaces = getPeaksWorkspaceNames();
    for (std::vector<std::string>::iterator it = storedPeaksWorkspaces.begin(); it != storedPeaksWorkspaces.end(); ++it) {
      size_t count = std::count(peaksWorkspaceNames.begin(), peaksWorkspaceNames.end(), *it);
      if (count == 0) {
        removePresenter(*it);
      }
    }
  }

  /**
   * Check if there are any peaks availble.
   * @returns If there are any peaks availbale.
   */
  bool CompositePeaksPresenterVsi::hasPeaks(){
    if (m_peaksPresenters.size() > 0) {
      return true;
    } else {
      return false;
    }
  }
}
}
