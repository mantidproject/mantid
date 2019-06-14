// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"

#include "MuonAnalysisHelper.h"
#include "MuonAnalysisResultTableCreator.h"

using Mantid::API::AnalysisDataService;
using Mantid::API::ExperimentInfo;
using Mantid::API::ITableWorkspace;
using Mantid::API::ITableWorkspace_sptr;
using Mantid::API::TableRow;
using Mantid::API::WorkspaceFactory;
using Mantid::API::WorkspaceGroup;
namespace {

/**
 * Converts QStringList -> std::vector<std::string>
 * @param qsl :: [input] QStringList to convert
 * @returns :: std::vector of std::strings
 */
std::vector<std::string> qStringListToVector(const QStringList &qsl) {
  std::vector<std::string> vec;
  vec.reserve(qsl.size());
  std::transform(qsl.begin(), qsl.end(), std::back_inserter(vec),
                 [](const QString &qs) { return qs.toStdString(); });
  return vec;
}

/**
 * Convenience function to add column to table
 * @param table :: [input, output] Table to which column should be added
 * @param dataType :: [input] Type of column e.g. "str" or "double"
 * @param name :: [input] Name of column
 * @param plotType :: [input] Plot type as integer
 */
void addColumnToTable(ITableWorkspace_sptr &table, const std::string &dataType,
                      const std::string &name, const int plotType) {

  auto column = table->addColumn(dataType, name);
  column->setPlotType(plotType);
  column->setReadOnly(false);
}

/// Plot types for table columns
constexpr static int PLOT_TYPE_X(1);
constexpr static int PLOT_TYPE_Y(2);
constexpr static int PLOT_TYPE_YERR(5);
constexpr static int PLOT_TYPE_LABEL(6);

/// The strings "Error" and "Cost function value"
const static std::string ERROR_STRING("Error");
constexpr static size_t ERROR_LENGTH(5);
const static QString ERROR_QSTRING{QString::fromStdString(ERROR_STRING)};
const static std::string COSTFN_STRING("Cost function value");
const static QString COSTFN_QSTRING{QString::fromStdString(COSTFN_STRING)};
} // namespace

namespace MantidQt {
namespace CustomInterfaces {

/**
 * Constructor
 * @param itemsSelected :: [input] List of selected items - individual
 * workspaces OR labels of simultaneous fits
 * @param logsSelected :: [input] List of selected log names
 * @param logValues :: [input] Pointer to data structure containing log values
 * (to avoid copying them all)
 * @param multipleLabels :: [input] Whether table is for individual workspaces
 * (false, default) or multiple labels (true)
 * @throws std::invalid_argument if pointer logValues is nullptr
 */
MuonAnalysisResultTableCreator::MuonAnalysisResultTableCreator(
    const QStringList &itemsSelected, const QStringList &logsSelected,
    const LogValuesMap *logValues, bool multipleLabels)
    : m_items(itemsSelected), m_logs(logsSelected), m_logValues(logValues),
      m_multiple(multipleLabels), m_firstStart_ns(0) {
  if (!m_logValues) {
    throw std::invalid_argument(
        "Log values passed in to result table creator are null!");
  }
}
/**
 * Create a results table with the given options
 * @returns :: results table workspace
 * @throws std::runtime_error if there was a problem creating the table
 */
ITableWorkspace_sptr MuonAnalysisResultTableCreator::createTable() const {
  if ((m_items.size() == 0) || m_logs.size() == 0) {
    throw std::runtime_error("Please select options from both tables.");
  }

  // Get the workspaces corresponding to the selected labels
  const auto &workspacesByLabel = getWorkspacesByLabel();

  // Check workspaces have the same parameters (and number of runs, if multiple)
  checkSameFitModel();
  checkSameNumberOfDatasets(workspacesByLabel);

  // Create the results table
  auto table = WorkspaceFactory::Instance().createTable("TableWorkspace");

  // Add columns for log values
  if (m_multiple) {
    addColumnToTable(table, "str", "Label", PLOT_TYPE_LABEL);
  } else {
    addColumnToTable(table, "str", "workspace_Name", PLOT_TYPE_LABEL);
  }

  // Cache the start time of the first run
  m_firstStart_ns = getFirstStartTimeNanosec(workspacesByLabel);

  // Get param information and add columns to table
  const auto &wsParamsByLabel = getParametersByLabel(workspacesByLabel);

  const auto valMap = m_logValues->value(m_logValues->keys()[0]);
  for (const auto &log : m_logs) {

    auto val = valMap[log];
    auto dashIndex = val.toString().indexOf("-");

    // multiple files use strings due to x-y format
    if (dashIndex != 0 && dashIndex != -1) {
      addColumnToTable(table, "str", log.toStdString(), PLOT_TYPE_X);
    } else if (MuonAnalysisHelper::isNumber(val.toString()) &&
               !log.endsWith(" (text)")) {
      addColumnToResultsTable(table, wsParamsByLabel, log); //

    } else {
      addColumnToTable(table, "str", log.toStdString(), PLOT_TYPE_X);
    }
  }
  const auto &paramsToDisplay = addParameterColumns(table, wsParamsByLabel);

  // Write log and parameter data to the table
  writeData(table, wsParamsByLabel, paramsToDisplay);

  // Remove error columns if all errors are zero
  // (because these correspond to fixed parameters)
  removeFixedParameterErrors(table);

  return table;
}

/**
 * Returns a map of labels to lists of workspace names.
 * If there are no labels, puts all workspaces under a "dummy" label.
 * @returns :: map of label to list of workspace names
 */
std::map<QString, std::vector<std::string>>
MuonAnalysisResultTableCreator::getWorkspacesByLabel() const {
  std::map<QString, std::vector<std::string>> wsByLabel;

  if (m_multiple) {
    // m_items is a list of labels
    for (const auto &label : m_items) {
      std::vector<std::string> names;
      const auto &group =
          AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
              "MuonSimulFit_" + label.toStdString());
      for (const auto &name : group->getNames()) {
        const size_t pos = name.find("_Workspace");
        if (pos != std::string::npos) {
          names.push_back(name.substr(0, pos));
        }
      }
      if (names.empty()) {
        // This guarantees the list of workspaces for each label will not be
        // empty
        throw std::runtime_error("No fitted workspaces found for label " +
                                 label.toStdString());
      }
      wsByLabel[label] = names;
    }
  } else {
    // There are no labels and m_items contains the workspace names
    wsByLabel["dummy"] = qStringListToVector(m_items);
  }
  return wsByLabel;
}

/**
 * Get parameters table out of ADS, given the simultaneous fit label or
 * workspace name
 * @param name :: [input] Label for simultaneous fit OR workspace name
 * @return :: pointer to parameters table
 * @throws std::runtime_error if workspace not found or wrong type
 */
ITableWorkspace_sptr MuonAnalysisResultTableCreator::getFitParametersTable(
    const QString &name) const {
  if (m_multiple) {
    return tableFromLabel(name.toStdString());
  } else {
    return tableFromWorkspace(name.toStdString());
  }
}

/**
 * Get parameters table out of ADS, given the workspace base name
 * @param wsName :: [input] Base name without "_Workspace" or "_Parameters"
 * @return :: pointer to parameters table
 * @throws std::runtime_error if workspace not found or wrong type
 */
ITableWorkspace_sptr MuonAnalysisResultTableCreator::tableFromWorkspace(
    const std::string &wsName) const {
  if (const auto &table =
          AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
              wsName + "_Parameters")) {
    return table;
  } else {
    throw std::runtime_error("Could not retrieve parameters table " + wsName);
  }
}

/**
 * Get parameters table out of ADS, given the simultaneous fit label
 * @param label :: [input] Label for simultaneous fit
 * @return :: pointer to parameters table
 * @throws std::runtime_error if workspace not found or wrong type
 */
ITableWorkspace_sptr
MuonAnalysisResultTableCreator::tableFromLabel(const std::string &label) const {
  if (const auto &wsGroup =
          AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
              "MuonSimulFit_" + label)) {
    const auto wsNames = wsGroup->getNames();
    const auto found =
        std::find_if(wsNames.cbegin(), wsNames.cend(), [](const auto &name) {
          return name.find("_Parameters") != std::string::npos;
        });
    if (found != wsNames.cend()) {
      return boost::dynamic_pointer_cast<ITableWorkspace>(
          wsGroup->getItem(*found));
    }
    throw std::runtime_error("Could not retrieve parameters table for label " +
                             label);
  } else {
    throw std::runtime_error("Could not retrieve fitted parameters for label " +
                             label);
  }
}

/**
 * Checks all fitted items (workspaces/labels) to make sure they have the same
 * fit model, i.e. same parameters.
 * @throws std::runtime_error if fit models do not match.
 */
void MuonAnalysisResultTableCreator::checkSameFitModel() const {
  std::vector<ITableWorkspace_sptr> paramTables;
  paramTables.reserve(static_cast<size_t>(m_items.size()));
  for (const auto &item : m_items) {
    paramTables.emplace_back(getFitParametersTable(item));
  }
  if (!haveSameParameters(paramTables)) {
    throw std::runtime_error(
        "Please pick workspaces with the same fitted parameters");
  }
}

/**
 * For multiple fits, checks each label to make sure it has the same number of
 * datasets.
 * @throws std::runtime_error if number of datasets differs between labels.
 */
void MuonAnalysisResultTableCreator::checkSameNumberOfDatasets(
    const std::map<QString, std::vector<std::string>> &workspacesByLabel)
    const {
  const size_t firstNumRuns = workspacesByLabel.begin()->second.size();
  if (std::any_of(workspacesByLabel.begin(), workspacesByLabel.end(),
                  [&firstNumRuns](
                      const std::pair<QString, std::vector<std::string>> fit) {
                    return fit.second.size() != firstNumRuns;
                  })) {
    throw std::runtime_error(
        "Please pick fit labels with the same number of workspaces");
  }
}

/**
 * Get the first start time in nanoseconds. Test all labels as we don't know
 * which was first; in each label the workspace names are assumed to be in
 * order.
 * @param workspacesByLabel :: [input] map of label to list of workspace names
 */
int64_t MuonAnalysisResultTableCreator::getFirstStartTimeNanosec(
    const std::map<QString, std::vector<std::string>> &workspacesByLabel)
    const {
  // Cache the start time of the first run. We don't know which label was first,
  // so test them all.
  int64_t firstStart_ns = std::numeric_limits<int64_t>::max();
  for (const auto &item : workspacesByLabel) {
    // item is a pair of <label name -> workspace names>
    const auto &wsNames = item.second;
    if (const auto &ws =
            Mantid::API::AnalysisDataService::Instance()
                .retrieveWS<ExperimentInfo>(wsNames.front() + "_Workspace")) {
      const int64_t start_ns = ws->run().startTime().totalNanoseconds();
      if (start_ns < firstStart_ns) {
        firstStart_ns = start_ns;
      }
    }
  }
  return firstStart_ns;
}

/**
 * Populate a map of <label name, <workspace name, <parameter, value>>>
 * from the given map of workspaces by label
 * @param workspacesByLabel :: [input] map of label to list of workspace names
 * @returns :: map of <label name, <workspace name, <parameter, value>>>
 */
QMap<QString, WSParameterList>
MuonAnalysisResultTableCreator::getParametersByLabel(
    const std::map<QString, std::vector<std::string>> &workspacesByLabel)
    const {
  QMap<QString, WSParameterList> wsParamsByLabel;
  for (const auto &labelToWsNames : workspacesByLabel) {
    WSParameterList wsParamsList;
    // labelToWsNames is a pair of <label name -> workspace names>
    for (const auto &wsName : labelToWsNames.second) {
      QMap<QString, double> paramsList;
      const auto &paramWS = tableFromWorkspace(wsName);
      TableRow paramRow = paramWS->getFirstRow();
      // Loop over all rows and get values and errors
      do {
        std::string key;
        double value;
        double error;
        paramRow >> key >> value >> error;
        paramsList[QString::fromStdString(key)] = value;
        paramsList[QString::fromStdString(key + "Error")] = error;
      } while (paramRow.next());
      wsParamsList[QString::fromStdString(wsName)] = paramsList;
    }
    wsParamsByLabel[labelToWsNames.first] = wsParamsList;
  }
  return wsParamsByLabel;
}

/**
 * Add columns to the table for parameter values and errors, and a column for
 * cost function at the end.
 * For multiple labels:
 *   - add a column + error for each global parameter
 *   - for each local param, add a column + error for each dataset in the label
 * For single labels (regular result tables), add a column and error for each
 * parameter.
 * @param table :: [input, output] Table to which column(s) will be added
 * @param paramsByLabel :: [input] Map of <label name, <workspace name,
 * <parameter, value>>>
 * @returns :: List of parameters to display
 */
QStringList MuonAnalysisResultTableCreator::addParameterColumns(
    ITableWorkspace_sptr &table,
    const QMap<QString, WSParameterList> &paramsByLabel) const {
  QStringList paramsToDisplay;
  auto paramNames = paramsByLabel.begin()->begin()->keys();
  // Remove the errors and cost function - just want the parameters
  paramNames.erase(std::remove_if(paramNames.begin(), paramNames.end(),
                                  [](const QString &qs) {
                                    return qs.endsWith("Error") ||
                                           qs.startsWith("Cost function");
                                  }),
                   paramNames.end());

  // Add columns to table and update list of parameters to display
  const auto addToTableAndList = [&table, &paramsToDisplay](
                                     const QString &paramName,
                                     const std::string &colName) {
    addColumnToTable(table, "double", colName, PLOT_TYPE_Y);
    addColumnToTable(table, "double", colName + ERROR_STRING, PLOT_TYPE_YERR);
    paramsToDisplay.append(paramName);
    paramsToDisplay.append(paramName + ERROR_QSTRING);
  };

  if (m_multiple) {
    // Global: add one column (+ error)
    // Local: add one per dataset (+ error)
    for (const auto &param : paramNames) {
      if (isGlobal(param, paramsByLabel)) {
        addToTableAndList(param, param.toStdString());
      } else {
        const int nDatasetsPerLabel = paramsByLabel.begin()->count();
        for (int i = 0; i < nDatasetsPerLabel; ++i) {
          const auto &columnName =
              'f' + std::to_string(i) + '.' + param.toStdString();
          addToTableAndList(param, columnName);
        }
      }
    }
  } else {
    // For each parameter, add a column and an error column
    for (const auto &param : paramNames) {
      addToTableAndList(param, param.toStdString());
    }
  }

  // Add cost function at the end of the table after params
  addColumnToTable(table, "double", COSTFN_STRING, PLOT_TYPE_Y);
  paramsToDisplay.append(COSTFN_QSTRING);

  return paramsToDisplay;
}

/**
 * In the supplied list of fit results, finds if the given parameter appears to
 * have been global. The global params have the same value for all workspaces in
 * a label.
 * @param param :: [input] Name of parameter
 * @param paramList :: [input] Map of label name to "WSParameterList" (itself a
 * map of workspace name to <name, value> map)
 * @returns :: Whether the parameter was global
 */
bool MuonAnalysisResultTableCreator::isGlobal(
    const QString &param,
    const QMap<QString, WSParameterList> &paramList) const {
  // It is safe to assume the same fit model was used for all labels.
  // So just test the first:
  return isGlobal(param, *paramList.begin());
}

/**
 * In the supplied list of fit results, finds if the given parameter appears to
 * have been global. The global params have the same value for all workspaces.
 * @param param :: [input] Name of parameter
 * @param paramList :: [input] Map of workspace name to <name, value> map
 * @returns :: Whether the parameter was global
 */
bool MuonAnalysisResultTableCreator::isGlobal(
    const QString &param, const WSParameterList &paramList) const {
  const double firstValue = paramList.begin()->value(param);
  if (paramList.size() > 1) {
    for (auto it = paramList.begin() + 1; it != paramList.end(); ++it) {
      // If any parameter differs from the first it cannot be global
      if (std::abs(it->value(param) - firstValue) >
          std::numeric_limits<double>::epsilon()) {
        return false;
      }
    }
    return true; // All values are the same so it is global
  } else {
    return false; // Only one workspace so no globals
  }
}

/**
 * Writes log and parameter values to the table. Calls the relevant method for
 * single fit or multiple fits as appropriate.
 * @param table :: [input, output] Table to write to
 * @param paramsByLabel :: [input] Map of <label name, <workspace name,
 * <parameter, value>>>
 * @param paramsToDisplay :: [input] List of parameters to display in table
 */
void MuonAnalysisResultTableCreator::writeData(
    ITableWorkspace_sptr &table,
    const QMap<QString, WSParameterList> &paramsByLabel,
    const QStringList &paramsToDisplay) const {
  if (m_multiple) {
    writeDataForMultipleFits(table, paramsByLabel, paramsToDisplay);
  } else {
    writeDataForSingleFit(table, paramsByLabel, paramsToDisplay);
  }
}

/**
 * Write log and parameter values to the table for the case of a single fit.
 * @param table :: [input, output] Table to write to
 * @param paramsByLabel :: [input] Map of <label name, <workspace name,
 * <parameter, value>>>
 * @param paramsToDisplay :: [input] List of parameters to display in table
 */
void MuonAnalysisResultTableCreator::writeDataForSingleFit(
    ITableWorkspace_sptr &table,
    const QMap<QString, WSParameterList> &paramsByLabel,
    const QStringList &paramsToDisplay) const {
  assert(!m_multiple);
  assert(m_logValues);

  for (const auto &wsName : m_items) {
    Mantid::API::TableRow row = table->appendRow();
    row << wsName.toStdString();

    // Get log values for this row
    const auto &logValues = m_logValues->value(wsName);

    // Write log values in each column
    for (int i = 0; i < m_logs.size(); ++i) {
      Mantid::API::Column_sptr col = table->getColumn(i);
      const auto &log = m_logs[i];
      const QVariant &val = logValues[log];
      QString valueToWrite;
      // Special case: if log is time in sec, subtract the first start time
      if (log.endsWith(" (s)")) {
        auto seconds =
            val.toDouble() - static_cast<double>(m_firstStart_ns) * 1.e-9;
        valueToWrite = QString::number(seconds);
      } else if (MuonAnalysisHelper::isNumber(val.toString()) &&
                 !log.endsWith(" (text)")) {
        valueToWrite = QString::number(val.toDouble());
      } else {
        valueToWrite = val.toString();
      }

      if (MuonAnalysisHelper::isNumber(val.toString()) &&
          !log.endsWith(" (text)")) {
        row << valueToWrite.toDouble();
      } else {
        row << valueToWrite.toStdString();
      }
    }

    // Add param values (params same for all workspaces)
    QMap<QString, double> paramsList = paramsByLabel.begin()->value(wsName);
    for (const auto &paramName : paramsToDisplay) {
      row << paramsList[paramName];
    }
  }
}
/**
 * Add column for a log to the table for the case of multiple fits.
 * Have to check multiple values are not returned
 * @param table :: [input, output] Table to write to
 * @param paramsByLabel :: [input] Map of <label name, <workspace name,
 * <parameter, value>>>
 * @param log :: [input] the log we are going to add to the table
 */
void MuonAnalysisResultTableCreator::addColumnToResultsTable(
    ITableWorkspace_sptr &table,
    const QMap<QString, WSParameterList> &paramsByLabel,
    const QString &log) const {
  // if its a single fit we know its a double
  if (!m_multiple) {
    addColumnToTable(table, "double", log.toStdString(), PLOT_TYPE_X);
    return;
  }
  const auto &labelName = m_items[0];

  QStringList valuesPerWorkspace;
  for (const auto &wsName : paramsByLabel[labelName].keys()) {
    const auto &logValues = m_logValues->value(wsName);
    const auto &val = logValues[log];
    // Special case: if log is time in sec, subtract the first start time
    if (log.endsWith(" (s)")) {
      auto seconds =
          val.toDouble() - static_cast<double>(m_firstStart_ns) * 1.e-9;
      valuesPerWorkspace.append(QString::number(seconds));
    } else if (MuonAnalysisHelper::isNumber(val.toString()) &&
               !log.endsWith(" (text)")) {

      valuesPerWorkspace.append(QString::number(val.toDouble()));

    } else {
      valuesPerWorkspace.append(logValues[log].toString());
    }
  }
  valuesPerWorkspace.sort();

  auto dashIndex = valuesPerWorkspace.front().toStdString().find_first_of("-");
  if (dashIndex != std::string::npos && dashIndex != 0) {

    addColumnToTable(table, "str", log.toStdString(), PLOT_TYPE_X);
    return;
  }
  const auto &min = valuesPerWorkspace.front().toDouble();
  const auto &max = valuesPerWorkspace.back().toDouble();
  if (min == max) {
    addColumnToTable(table, "double", log.toStdString(), PLOT_TYPE_X);
    return;
  }
  addColumnToTable(table, "str", log.toStdString(), PLOT_TYPE_X);
}

/**
 * Write log and parameter values to the table for the case of multiple fits.
 * @param table :: [input, output] Table to write to
 * @param paramsByLabel :: [input] Map of <label name, <workspace name,
 * <parameter, value>>>
 * @param paramsToDisplay :: [input] List of parameters to display in table
 */
void MuonAnalysisResultTableCreator::writeDataForMultipleFits(
    ITableWorkspace_sptr &table,
    const QMap<QString, WSParameterList> &paramsByLabel,
    const QStringList &paramsToDisplay) const {
  assert(m_multiple);
  assert(m_logValues);

  // Add data to table
  for (const auto &labelName : m_items) {
    Mantid::API::TableRow row = table->appendRow();
    size_t columnIndex(0); // Which column we are writing to

    row << labelName.toStdString();
    columnIndex++;

    // Get log values for this row and write in table
    for (const auto &log : m_logs) {
      QStringList valuesPerWorkspace;
      for (const auto &wsName : paramsByLabel[labelName].keys()) {
        const auto &logValues = m_logValues->value(wsName);
        const auto &val = logValues[log];

        auto dashIndex = val.toString().indexOf("-");
        // Special case: if log is time in sec, subtract the first start time
        if (log.endsWith(" (s)")) {
          auto seconds =
              val.toDouble() - static_cast<double>(m_firstStart_ns) * 1.e-9;
          valuesPerWorkspace.append(QString::number(seconds));
        } else if (dashIndex != 0 && dashIndex != -1) {
          valuesPerWorkspace.append(logValues[log].toString());
        } else if (MuonAnalysisHelper::isNumber(val.toString()) &&
                   !log.endsWith(" (text)")) {

          valuesPerWorkspace.append(QString::number(val.toDouble()));

        } else {
          valuesPerWorkspace.append(logValues[log].toString());
        }
      }

      // Range of values - use string comparison as works for numbers too
      // Why not use std::minmax_element? To avoid MSVC warning: QT bug 41092
      // (https://bugreports.qt.io/browse/QTBUG-41092)
      valuesPerWorkspace.sort();

      auto dashIndex =
          valuesPerWorkspace.front().toStdString().find_first_of("-");
      if (dashIndex != std::string::npos && dashIndex != 0) {
        std::ostringstream oss;
        auto dad = valuesPerWorkspace.front().toStdString();
        oss << valuesPerWorkspace.front().toStdString();
        row << oss.str();

      } else {
        if (MuonAnalysisHelper::isNumber(valuesPerWorkspace.front())) {
          const auto &min = valuesPerWorkspace.front().toDouble();
          const auto &max = valuesPerWorkspace.back().toDouble();
          if (min == max) {
            row << min;
          } else {
            std::ostringstream oss;
            oss << valuesPerWorkspace.front().toStdString() << "-"
                << valuesPerWorkspace.back().toStdString();
            row << oss.str();
          }
        } else {
          const auto &front = valuesPerWorkspace.front().toStdString();
          const auto &back = valuesPerWorkspace.back().toStdString();
          if (front == back) {
            row << front;
          } else {
            std::ostringstream oss;
            oss << valuesPerWorkspace[0].toStdString();

            for (int k = 1; k < valuesPerWorkspace.size(); k++) {
              oss << ", " << valuesPerWorkspace[k].toStdString();
              row << oss.str();
            }
          }
        }
      }
      columnIndex++;
    }

    // Parse column name - could be param name or f[n].param
    const auto parseColumnName =
        [&paramsToDisplay](
            const std::string &columnName) -> std::pair<int, std::string> {
      if (paramsToDisplay.contains(QString::fromStdString(columnName))) {
        return {0, columnName};
      } else {
        // column name is f[n].param
        size_t pos = columnName.find_first_of('.');
        if (pos != std::string::npos) {
          try {
            const auto &paramName = columnName.substr(pos + 1);
            const auto wsIndex = std::stoi(columnName.substr(1, pos));
            return {wsIndex, paramName};
          } catch (const std::exception &ex) {
            throw std::runtime_error("Failed to parse column name " +
                                     columnName + ": " + ex.what());
          }
        } else {
          throw std::runtime_error("Failed to parse column name " + columnName);
        }
      }
    };

    // Add param values
    const auto &params = paramsByLabel[labelName];
    while (columnIndex < table->columnCount()) {
      const auto &parsedColName =
          parseColumnName(table->getColumn(columnIndex)->name());
      const QString wsName = params.keys().at(parsedColName.first);
      const QString &paramName = QString::fromStdString(parsedColName.second);
      row << params[wsName].value(paramName);
      columnIndex++;
    }
  }
}

/**
 * Checks the given set of fit tables to see if all fits had same parameters.
 * @param tables :: [input] Fit tables
 * @returns :: True if all fits used same model, otherwise false.
 */
bool MuonAnalysisResultTableCreator::haveSameParameters(
    const std::vector<ITableWorkspace_sptr> &tables) const {
  bool sameParams = true;

  // lambda to pull keys out of table
  auto getKeysFromTable = [](const Mantid::API::ITableWorkspace_sptr &tab) {
    std::vector<std::string> keys;
    if (tab) {
      Mantid::API::TableRow row = tab->getFirstRow();
      do {
        std::string key;
        row >> key;
        keys.push_back(key);
      } while (row.next());
    }
    return keys;
  };

  if (tables.size() > 1) {
    const auto &firstKeys = getKeysFromTable(tables.front());
    for (size_t i = 1; i < tables.size(); ++i) {
      const auto &keys = getKeysFromTable(tables[i]);
      if (keys != firstKeys) {
        sameParams = false;
        break;
      }
    }
  }
  return sameParams;
}

/**
 * Removes error columns from the table if all errors are zero,
 * as these columns correspond to fixed parameters.
 * @param table :: [input, output] Pointer to TableWorkspace to edit
 */
void MuonAnalysisResultTableCreator::removeFixedParameterErrors(
    const ITableWorkspace_sptr table) const {
  assert(table);
  const size_t nRows = table->rowCount();
  const auto colNames = table->getColumnNames();
  std::vector<std::string> zeroErrorColumns;

  for (const auto &name : colNames) {
    // if name does not end with "Error", continue
    const size_t nameLength = name.length();
    if (nameLength < ERROR_LENGTH ||
        name.compare(nameLength - ERROR_LENGTH, ERROR_LENGTH, ERROR_STRING)) {
      continue;
    }

    auto col = table->getColumn(name);
    bool allZeros = true;
    // Check if all values in the column are zero
    for (size_t iRow = 0; iRow < nRows; ++iRow) {
      const double val = col->toDouble(iRow);
      if (std::abs(val) > std::numeric_limits<double>::epsilon()) {
        allZeros = false;
        break;
      }
    }
    if (allZeros) {
      zeroErrorColumns.push_back(name);
    }
  }

  for (const auto &name : zeroErrorColumns) {
    table->removeColumn(name);
  }
}
} // namespace CustomInterfaces
} // namespace MantidQt
