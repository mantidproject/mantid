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

QString
PostprocessingStep::getPostprocessedWorkspaceName(const WhiteList &whitelist,
                                                  const GroupData &groupData) {
  /* This method calculates, for a given set of rows, the name of the output
  * (post-processed) workspace */

  QStringList outputNames;

  for (const auto &data : groupData) {
    outputNames.append(getReducedWorkspaceName(data.second, whitelist));
  }
  return m_algorithm.prefix() + outputNames.join("_");
}

/**
  Post-processes the workspaces created by the given rows together.
  @param processorPrefix : The prefix of the processor algorithm.
  @param whitelist : The list of columns in the table.
  @param groupData : the data in a given group as received from the tree
  manager
 */
void PostprocessingStep::postProcessGroup(const QString &processorPrefix,
                                          const WhiteList &whitelist,
                                          const GroupData &groupData) {
  // The input workspace names
  QStringList inputNames;

  // The name to call the post-processed ws
  auto const outputWSName = getPostprocessedWorkspaceName(whitelist, groupData);

  // Go through each row and get the input ws names
  for (auto const &row : groupData) {
    // The name of the reduced workspace for this row
    auto const inputWSName =
        getReducedWorkspaceName(row.second, whitelist, processorPrefix);

    if (workspaceExists(inputWSName)) {
      inputNames.append(inputWSName);
    }
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
        groupData.begin()->second[whitelist.indexFromName(prop.first)];
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

  if (!alg->isExecuted())
    throw std::runtime_error("Failed to post-process workspaces.");
}
}
}
}
