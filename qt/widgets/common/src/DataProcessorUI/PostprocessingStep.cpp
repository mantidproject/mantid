#include "MantidQtWidgets/Common/DataProcessorUI/PostprocessingStep.h"
#include "MantidQtWidgets/Common/DataProcessorUI/WorkspaceNameUtils.h"

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
PostprocessingStep::PostprocessingStep(QString options)
    : m_options(std::move(options)) {}
PostprocessingStep::PostprocessingStep(QString options,
                                       PostprocessingAlgorithm algorithm,
                                       std::map<QString, QString> map)
    : m_options(std::move(options)), m_algorithm(std::move(algorithm)),
      m_map(std::move(map)) {}

bool PostprocessingStep::workspaceExists(QString const &workspaceName) {
  return Mantid::API::AnalysisDataService::Instance().doesExist(
      workspaceName.toStdString());
}

void PostprocessingStep::removeWorkspace(QString const &workspaceName) {
  Mantid::API::AnalysisDataService::Instance().remove(
      workspaceName.toStdString());
}

void PostprocessingStep::removeIfExists(QString const &workspaceName) {
  if (workspaceExists(workspaceName))
    removeWorkspace(workspaceName);
}

void PostprocessingStep::ensureRowSizeMatchesColumnCount(
    const WhiteList &columns, const QStringList &row) {
  if (row.size() != static_cast<int>(columns.size()))
    throw std::invalid_argument("Can't find reduced workspace name");
}

QString PostprocessingStep::getPostprocessedWorkspaceName(
    const GroupData &groupData, boost::optional<size_t> sliceIndex) const {
  /* This method calculates, for a given set of rows, the name of the output
   * (post-processed) workspace for a given slice */

  QStringList outputNames;

  for (const auto &row : groupData) {
    auto rowData = row.second;
    // If given a slice, check if it exists (nothing to do for slices otherwise)
    if (sliceIndex && rowData->hasSlice(*sliceIndex)) {
      outputNames.append(rowData->getSlice(*sliceIndex)->reducedName());
    } else if (!sliceIndex) {
      // A slice index was not provided, so just use the row's workspace name
      outputNames.append(rowData->reducedName());
    }
  }
  return m_algorithm.prefix() + outputNames.join("_");
}

/**
  Post-processes the workspaces created by the given rows together.
  @param outputWSName : The property name for the input workspace
  used in the row reductions
  @param rowOutputWSPropertyName : The property name for the output workspace
  used in the row reductions
  @param whitelist : The list of columns in the table.
  @param groupData : the data in a given group as received from the tree
  manager
 */
void PostprocessingStep::postProcessGroup(
    const QString &outputWSName, const QString &rowOutputWSPropertyName,
    const WhiteList &whitelist, const GroupData &groupData) {
  // Go through each row and get the input ws names for postprocessing
  // (i.e. the output workspace of each row)
  QStringList inputNames;
  for (auto const &row : groupData) {
    // The name of the reduced workspace for this row from the given property
    // value. Note that we need the preprocessed names as these correspond to
    // the real output workspace names.
    auto const inputWSName =
        row.second->preprocessedOptionValue(rowOutputWSPropertyName);

    // Only postprocess if all workspaces exist
    if (!workspaceExists(inputWSName))
      throw std::runtime_error(
          "Some workspaces in the group could not be found");

    inputNames.append(inputWSName);
  }

  auto const inputWSNames = inputNames.join(", ");

  // If the previous result is in the ADS already, we'll need to remove it.
  // If it's a group, we'll get an error for trying to group into a used group
  // name
  removeIfExists(outputWSName);

  auto alg = Mantid::API::AlgorithmManager::Instance().create(
      m_algorithm.name().toStdString());

  alg->initialize();
  alg->setProperty(m_algorithm.inputProperty().toStdString(),
                   inputWSNames.toStdString());
  alg->setProperty(m_algorithm.outputProperty().toStdString(),
                   outputWSName.toStdString());

  auto optionsMap = parseKeyValueString(m_options.toStdString());
  for (auto const &kvp : optionsMap) {
    try {
      alg->setProperty(kvp.first, kvp.second);
    } catch (Mantid::Kernel::Exception::NotFoundError &) {
      throw std::runtime_error("Invalid property in options column: " +
                               kvp.first);
    }
  }

  // Options specified via post-process map
  for (auto const &prop : m_map) {
    auto const &propName = prop.second;
    auto const &propValueStr =
        (*groupData.begin()->second)[whitelist.indexFromName(prop.first)];
    if (!propValueStr.isEmpty()) {
      // Warning: we take minus the value of the properties because in
      // Reflectometry this property refers to the rebin step, and they want a
      // logarithmic binning. If other technique areas need to use a
      // post-process map we'll need to re-think how to do this.
      alg->setPropertyValue(propName.toStdString(),
                            ("-" + propValueStr).toStdString());
    }
  }

  alg->execute();

  if (!alg->isExecuted()) {
    throw std::runtime_error("Failed to execute algorithm " +
                             m_algorithm.name().toStdString());
  }
}
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
