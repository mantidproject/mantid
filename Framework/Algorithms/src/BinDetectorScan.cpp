#include "MantidAlgorithms/BinDetectorScan.h"

#include "MantidAPI/ADSValidator.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/make_unique.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidAlgorithms/RunCombinationHelpers/RunCombinationHelper.h"

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(BinDetectorScan)

using namespace API;
using namespace Kernel;

void BinDetectorScan::init() {
  // declare arbitrary number of input workspaces as a list of strings at the
  // moment
  declareProperty(
      make_unique<ArrayProperty<std::string>>(
          "InputWorkspaces", boost::make_shared<ADSValidator>()),
      "The names of the input workspaces as a list. You may "
      "also group workspaces using the GUI or [[GroupWorkspaces]], and specify "
      "the name of the group instead.");
  declareProperty(make_unique<WorkspaceProperty<Workspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Name of the output workspace");
  declareProperty(
      make_unique<ArrayProperty<double>>(
          "Params", boost::make_shared<RebinParamsValidator>()),
      "A comma separated list of first bin boundary, width, last bin boundary. "
      "Optionally "
      "this can be followed by a comma and more widths and last boundary "
      "pairs. "
      "Optionally this can also be a single number, which is the bin width. "
      "In this case, the boundary of binning will be determined by minimum and "
      "maximum TOF "
      "values among all events, or previous binning boundary, in case of event "
      "Workspace, or "
      "non-event Workspace, respectively. Negative width values indicate "
      "logarithmic binning. ");
}

void BinDetectorScan::exec() {
  std::vector<double> params = getProperty("Params");
  if (params.size() == 0) {
    m_stepSize = params[0];
  } else if (params.size() == 3) {
    m_startAngle = params[0];
    m_stepSize = params[1];
    m_endAngle = params[2];
  }
  const std::vector<std::string> inputWorkspaces =
      getProperty("InputWorkspaces");
  auto workspaces = RunCombinationHelper::unWrapGroups(inputWorkspaces);
  RunCombinationHelper combHelper;
  auto workspaceList = combHelper.validateInputWorkspaces(workspaces, g_log);

  //  for (auto ws : workspaces) {
  //    g_log.error() << ws->name();
  //  }
}

} // namespace Algorithms
} // namespace Mantid
