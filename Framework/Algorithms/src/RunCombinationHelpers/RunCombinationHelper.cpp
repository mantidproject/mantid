#include "MantidAlgorithms/RunCombinationHelpers/RunCombinationHelper.h"

#include "MantidKernel/ListValidator.h"

namespace Mantid {
namespace Algorithms {

using namespace MergeRunsOptions;
using namespace Kernel;

bool RunCombinationHelper::checkCompatibility(API::Workspace_sptr,
                                              API::Workspace_sptr) {
  return true;
}

void RunCombinationHelper::declareSampleLogOverrideProperties(API::Algorithm* alg) {
    alg->declareProperty("SampleLogsTimeSeries", "",
                    "A comma separated list of the sample logs to merge into a "
                    "time series. The initial times are taken as the start times "
                    "for the run. Sample logs must be numeric.");
    alg->declareProperty("SampleLogsList", "",
                    "A comma separated list of the sample logs to merge into a "
                    "list. ");
    alg->declareProperty("SampleLogsWarn", "", "A comma separated list of the sample "
                                          "logs to generate a warning if "
                                          "different when merging.");
    alg->declareProperty("SampleLogsWarnTolerances", "",
                    "The tolerances for warning if sample logs are different. "
                    "Can either be empty for a comparison of the strings, a "
                    "single value for all warn sample logs, or a comma "
                    "separated list of values (must be the same length as "
                    "SampleLogsWarn).");
    alg->declareProperty("SampleLogsFail", "", "The sample logs to fail if different "
                                          "when merging. If there is a "
                                          "difference the run is skipped.");
    alg->declareProperty("SampleLogsFailTolerances", "",
                    "The tolerances for failing if sample logs are different. "
                    "Can either be empty for a comparison of the strings, a "
                    "single value for all fail sample logs, or a comma "
                    "separated list of values (must be the same length as "
                    "SampleLogsFail).");
    alg->declareProperty("SampleLogsSum", "", "A comma separated list of the sample "
                                         "logs to sum into a single entry.  "
                                         "Sample logs must be numeric.");
    const std::vector<std::string> rebinOptions = {REBIN_BEHAVIOUR,
                                                   FAIL_BEHAVIOUR};
    alg->declareProperty("RebinBehaviour", REBIN_BEHAVIOUR,
                    boost::make_shared<StringListValidator>(rebinOptions),
                    "Choose whether to rebin when bins are different, or fail "
                    "(fail behaviour defined in FailBehaviour option).");
    const std::vector<std::string> failBehaviourOptions = {SKIP_BEHAVIOUR,
                                                           STOP_BEHAVIOUR};
    alg->declareProperty("FailBehaviour", SKIP_BEHAVIOUR,
                    boost::make_shared<StringListValidator>(failBehaviourOptions),
                    "Choose whether to skip the file and continue, or stop and "
                    "throw and error, when encountering a failure.");
}

} // namespace Algorithms
} // namespace Mantid
