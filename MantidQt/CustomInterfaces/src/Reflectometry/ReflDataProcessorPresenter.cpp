#include "MantidQtCustomInterfaces/Reflectometry/ReflDataProcessorPresenter.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorTreeManager.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorView.h"
#include "MantidQtMantidWidgets/ProgressPresenter.h"

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
    const std::map<std::string, DataProcessorPreprocessingAlgorithm>
        &preprocessMap,
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

  // if uniform slicing is empty process normally, delegating to
  // GenericDataProcessorPresenter
  std::string timeSlicingOptions =
      m_mainPresenter->getTimeSlicingOptions();
  if (timeSlicingOptions.empty()) {
    GenericDataProcessorPresenter::process();
    return;
  }

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

  std::map<std::string, std::string> globalTransmissionOptions;
  if (!m_preprocessMap.empty())
    globalTransmissionOptions = m_mainPresenter->getPreprocessingOptions();

  // If transmission runs specified in the table
  // Load transmission runs
  // Run CreateTransmissionWorkspaceAuto, taking into account global options
  // from settings, if any

  // Get selected runs
  const auto items = m_manager->selectedData(true);

  // Progress: each group and each row within count as a progress step.
  int progress = 0;
  int maxProgress = (int)(items.size());
  for (const auto subitem : items) {
    maxProgress += (int)(subitem.second.size());
  }
  ProgressPresenter progressReporter(progress, maxProgress, maxProgress,
                                     m_progressView);

  for (const auto &item : items) {

    // Reduce rows sequentially

    for (const auto &data : item.second) {
      // item.second -> set of vectors containing data

      try {
        auto newData = reduceRow(data.second);
        m_manager->update(item.first, data.first, newData);
        progressReporter.report();

      }
      catch (std::exception &ex) {
        m_mainPresenter->giveUserCritical(ex.what(), "Error");
        progressReporter.clear();
        return;
      }

      progressReporter.report();
    }

    // Post-process (if needed)
    if (item.second.size() > 1) {
      try {
        postProcessGroup(item.second);
        progressReporter.report();
      }
      catch (std::exception &ex) {
        m_mainPresenter->giveUserCritical(ex.what(), "Error");
        progressReporter.clear();
        return;
      }
    }
  }

  // Perform slicing, see Max's script
  // When running ReflectometryReductionOneAuto apply global options and options
  // specified in the 'Options' column

  // For each group
  // Stitch slices (if needed), applying global options

  // Finally, notebook: Don't implement this for the moment
  // If selected, print message saying that notebook will not be saved
  if (m_view->getEnableNotebook()) {
    GenericDataProcessorPresenter::giveUserWarning(
        "Notebook will not be generated for sliced data",
        "Notebook will not be created");
  }
  // If not selected, do nothing
}
}
}
