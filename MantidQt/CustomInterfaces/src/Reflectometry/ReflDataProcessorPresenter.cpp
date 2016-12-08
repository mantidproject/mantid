#include "MantidQtCustomInterfaces/Reflectometry/ReflDataProcessorPresenter.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorTreeManager.h"

using namespace MantidQt::MantidWidgets;

namespace MantidQt {
namespace CustomInterfaces {

/**
* Constructor
* @param whitelist : The set of properties we want to show as columns
* @param preprocessMap : A map containing instructions for pre-processing
* @param processor : A DataProcessorProcessingAlgorithm
* @param postprocessor : A DataProcessorPostprocessingAlgorithm
* workspaces
* @param postprocessMap : A map containing instructions for post-processing.
* This map links column name to properties of the post-processing algorithm
* @param loader : The algorithm responsible for loading data
*/
ReflDataProcessorPresenter::ReflDataProcessorPresenter(
    const DataProcessorWhiteList &whitelist,
    const std::map<std::string, DataProcessorPreprocessingAlgorithm> &
        preprocessMap,
    const DataProcessorProcessingAlgorithm &processor,
    const DataProcessorPostprocessingAlgorithm &postprocessor,
    const std::map<std::string, std::string> &postprocessMap,
    const std::string &loader)
    : GenericDataProcessorPresenter(whitelist, preprocessMap, processor,
                                    postprocessor, postprocessMap, loader) {}

/**
* Destructor
*/
ReflDataProcessorPresenter::~ReflDataProcessorPresenter() {}

/**
 Process selected data
*/
void ReflDataProcessorPresenter::process() {

  // throw std::runtime_error("Not implemented");

  // if uniform slicing is empty process normally, delegating to
  // GenericDataProcessorPresenter
  GenericDataProcessorPresenter::process();
  // return here, nothing else to do

  // Uniform slicing:

  // Things we should take into account:
  // 1. We need to report progress, see GenericDataProcessorPresenter::process()
  // 2. We need to pay attention to prefixes. See how the interface names the output workspaces
  // 3. For slices, we probably want to add a suffix: <output_ws_name>_start_stop
  // (check that this is the suffix they're using in Max's script)
  // 4. There may be some methods defined in GenericDataProcessorPreseter that
  // we may find useful here, for instance
  // GenericDataProcessorPresenter::getReducedWorkspaceName() or
  // GenericDataProcessorPresenter::getPostprocessedWorkspaceName(). If there is
  // a private method you want to use, don't hesitate to make it protected in
  // the base class.

  // If transmission runs specified in the table
  // Load transmission runs
  // Run CreateTransmissionWorkspaceAuto, taking into account global options
  // from settings, if any

  // Get selected runs
  const auto items = m_manager->selectedData(true);

  // For each selected run

  // Perform slicing, see Max's script
  // When running ReflectometryReductionOneAuto apply global options and options specified in the 'Options' column

  // For each group
  // Stitch slices (if needed), applying global options

  // Finally, notebook: Don't implement this for the moment
  // If selected, print message saying that notebook will not be saved
  GenericDataProcessorPresenter::giveUserWarning(
      "Notebook will not be generated for sliced data",
      "Notebook will not be created");
  // If not selected, do nothing

  return;
}
}
}
