#include "MantidQtCustomInterfaces/Muon/MuonAnalysisFitDataHelper.h"

using MantidQt::MantidWidgets::IMuonFitDataSelector;
using MantidQt::MantidWidgets::IWorkspaceFitControl;

namespace MantidQt {
namespace CustomInterfaces {

/**
 * Constructor
 * @param fitBrowser :: [input] Pointer to fit browser to update
 * @param dataSelector :: [input] Pointer to data selector to get input from
 */
MuonAnalysisFitDataHelper::MuonAnalysisFitDataHelper(
    IWorkspaceFitControl *fitBrowser, IMuonFitDataSelector *dataSelector)
    : m_fitBrowser(fitBrowser), m_dataSelector(dataSelector) {}

/**
 * Called when data selector reports "workspace properties changed"
 * Updates WS name, WS index, startX, endX
 */
void MuonAnalysisFitDataHelper::handleWorkspacePropertiesChanged() {
  const auto runs = m_dataSelector->getRuns();
  // Get workspace from these runs (TODO)
  // m_fitBrowser->setWorkspaceName()

  // update workspace index
  const unsigned int wsIndex = m_dataSelector->getWorkspaceIndex();
  m_fitBrowser->setWorkspaceIndex(static_cast<int>(wsIndex));

  // update start and end times
  const double start = m_dataSelector->getStartTime();
  const double end = m_dataSelector->getEndTime();
  m_fitBrowser->setStartX(start);
  m_fitBrowser->setEndX(end);
}

/**
 * Called when data selector reports "selected groups changed"
 * Updates WS name
 */
void MuonAnalysisFitDataHelper::handleSelectedGroupsChanged() {
  // TODO: implement this
}

/**
 * Called when data selector reports "selected periods changed"
 * Updates WS name
 */
void MuonAnalysisFitDataHelper::handleSelectedPeriodsChanged() {
  // TODO: implement this
}
} // namespace CustomInterfaces
} // namespace MantidQt
