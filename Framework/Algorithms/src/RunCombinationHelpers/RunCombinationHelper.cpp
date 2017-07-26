#include "MantidAlgorithms/RunCombinationHelpers/RunCombinationHelper.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Unit.h"

namespace Mantid {
namespace Algorithms {

using namespace API;
using namespace Geometry;
using namespace Kernel;

//----------------------------------------------------------------------------------------------
/** Flattens the list of group workspaces (if any) into list of workspaces
* @param inputs : input workspaces vector [including] group workspaces (all must
* be on ADS)
* @return : the flat vector of the input workspaces
*/
std::vector<std::string>
RunCombinationHelper::unWrapGroups(const std::vector<std::string> &inputs) {
  std::vector<std::string> outputs;
  for (const auto &input : inputs) {
    WorkspaceGroup_sptr wsgroup =
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(input);
    if (wsgroup) {
      // workspace group
      std::vector<std::string> group = wsgroup->getNames();
      outputs.insert(outputs.end(), group.begin(), group.end());
    } else {
      // single workspace
      outputs.push_back(input);
    }
  }
  return outputs;
}

//----------------------------------------------------------------------------------------------
/** Sets the properties of the reference (usually first) workspace,
 * to later check the compatibility of the others with the reference
 * @param ref : the reference workspace
 */
void RunCombinationHelper::setReferenceProperties(MatrixWorkspace_sptr ref) {
  m_numberSpectra = ref->getNumberHistograms();
  m_xUnit = ref->getAxis(0)->unit()->unitID();
  m_spectrumAxisUnit = ref->getAxis(1)->unit()->unitID();
  m_yUnit = ref->YUnit();
  m_isHistogramData = ref->isHistogramData();
  m_instrumentName = ref->getInstrument()->getName();
}

//----------------------------------------------------------------------------------------------
/** Compares the properties of the input workspace with the reference
 * @param ws : the testee workspace
 * @param checkNumberHistograms : whether to check also the number of histograms
 * @return : empty if compatible, error message otherwises
 */
std::string
RunCombinationHelper::checkCompatibility(MatrixWorkspace_sptr ws,
                                         bool checkNumberHistograms) {
  std::string errors;
  if (ws->getNumberHistograms() != m_numberSpectra && checkNumberHistograms)
    errors += "different number of histograms; ";
  if (ws->getAxis(0)->unit()->unitID() != m_xUnit)
    errors += "different X units; ";
  if (ws->getAxis(1)->unit()->unitID() != m_spectrumAxisUnit)
    errors += "different spectrum axis units; ";
  if (ws->YUnit() != m_yUnit)
    errors += "different Y units; ";
  if (ws->isHistogramData() != m_isHistogramData)
    errors += "different distribution or histogram type; ";
  if (ws->getInstrument()->getName() != m_instrumentName)
    errors += "different instrument names; ";
  return errors;
}

} // namespace Algorithms
} // namespace Mantid
