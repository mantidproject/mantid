#include "MantidQtWidgets/Common/DataProcessorUI/PostprocessingStep.h"

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
PostprocessingStep::PostprocessingStep(const QString &options)
    : m_options(options) {}
PostprocessingStep::PostprocessingStep(const QString &options,
                                       PostprocessingAlgorithm algorithm,
                                       std::map<QString, QString> map)
    : m_options(options), m_algorithm(algorithm), m_map(map) {}

bool PostprocessingStep::workspaceExists(QString const &workspaceName) {
  return Mantid::API::AnalysisDataService::Instance().doesExist(
      workspaceName.toStdString());
}

void PostprocessingStep::removeWorkspace(QString const &workspaceName) {
  Mantid::API::AnalysisDataService::Instance().remove(
      workspaceName.toStdString());
}

void PostprocessingStep::removeIfExists(QString const &workspaceName) {
  if (workspaceExists(workspaceName)) {
    removeWorkspace(workspaceName);
  }
}

void PostprocessingStep::ensureRowSizeMatchesColumnCount(
    const WhiteList &columns, const QStringList &row) {
  if (row.size() != static_cast<int>(columns.size()))
    throw std::invalid_argument("Can't find reduced workspace name");
}

QString PostprocessingStep::getReducedWorkspaceName(const WhiteList &whitelist,
                                                    const QStringList &data,
                                                    const QString &prefix) {
  ensureRowSizeMatchesColumnCount(whitelist, data);

  /* This method calculates, for a given row, the name of the output
  * (processed)
  * workspace. This is done using the white list, which contains information
  * about the columns that should be included to create the ws name. In
  * Reflectometry for example, we want to include values in the 'Run(s)' and
  * 'Transmission Run(s)' columns. We may also use a prefix associated with
  * the column when specified. Finally, to construct the ws name we may also
  * use a 'global' prefix associated with the processing algorithm (for
  * instance 'IvsQ_' in Reflectometry) this is given by the second argument to
  * this method */

  // Temporary vector of strings to construct the name
  QStringList names;

  auto columnIt = whitelist.cbegin();
  auto runNumbersIt = data.constBegin();
  for (; columnIt != whitelist.cend(); ++columnIt, ++runNumbersIt) {
    auto column = *columnIt;
    // Do we want to use this column to generate the name of the output ws?
    if (column.isShown()) {
      auto const runNumbers = *runNumbersIt;

      if (!runNumbers.isEmpty()) {
        // But we may have things like '1+2' which we want to replace with
        // '1_2'
        auto value = runNumbers.split("+", QString::SkipEmptyParts);
        names.append(column.prefix() + value.join("_"));
      }
    }
  } // Columns

  auto wsname = prefix;
  wsname += names.join("_");
  return wsname;
}

QString
PostprocessingStep::getPostprocessedWorkspaceName(const WhiteList &whitelist,
                                                  const GroupData &groupData,
                                                  const QString &prefix) {
  /* This method calculates, for a given set of rows, the name of the output
  * (post-processed) workspace */

  QStringList outputNames;

  for (const auto &data : groupData) {
    outputNames.append(getReducedWorkspaceName(whitelist, data.second));
  }
  return prefix + outputNames.join("_");
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
  auto const outputWSName =
      getPostprocessedWorkspaceName(whitelist, groupData, m_algorithm.prefix());

  // Go through each row and get the input ws names
  for (auto const &row : groupData) {
    // The name of the reduced workspace for this row
    auto const inputWSName =
        getReducedWorkspaceName(whitelist, row.second, processorPrefix);

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
  for (auto kvp : optionsMap) {
    try {
      alg->setProperty(kvp.first, kvp.second);
    } catch (Mantid::Kernel::Exception::NotFoundError &) {
      throw std::runtime_error("Invalid property in options column: " +
                               kvp.first);
    }
  }

  // Options specified via post-process map
  for (auto const &prop : m_map) {
    auto const propName = prop.second;
    auto const propValueStr =
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
