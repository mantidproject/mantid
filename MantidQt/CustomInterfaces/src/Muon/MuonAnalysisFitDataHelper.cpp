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

/**
 * Called when user drags lines to set fit range
 * Update the text boxes silently (no event)
 * @param start :: [input] start of fit range
 * @param end :: [input] end of fit range
 */
void MuonAnalysisFitDataHelper::handleXRangeChangedGraphically(double start,
                                                               double end) {
  m_dataSelector->setStartTimeQuietly(start);
  m_dataSelector->setEndTimeQuietly(end);
}

/**
 * Called by selectMultiPeak: fit browser has been reassigned to a new
 * workspace.
 * Sets run number, instrument and workspace index in the data selector.
 * @param wsName :: [input] Name of new workspace
 */
void MuonAnalysisFitDataHelper::peakPickerReassigned(const QString &wsName) {
  // Parse workspace name here for run number and instrument name
  const QString instRun = wsName.section(';', 0, 0).trimmed();
  const int firstZero = instRun.indexOf("0");
  const QString instName = instRun.left(firstZero);
  const QString numberString = instRun.right(instRun.size() - firstZero);
  m_dataSelector->setWorkspaceDetails(numberString, instName);
  m_dataSelector->setWorkspaceIndex(0u); // always has only one spectrum
}

} // namespace CustomInterfaces
} // namespace MantidQt
