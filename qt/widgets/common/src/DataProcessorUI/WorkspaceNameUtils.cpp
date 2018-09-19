#include "MantidQtWidgets/Common/DataProcessorUI/WorkspaceNameUtils.h"

#include "MantidQtWidgets/Common/DataProcessorUI/WhiteList.h"
#include "MantidQtWidgets/Common/ParseKeyValueString.h"

#include <QString>
#include <QStringList>
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {

namespace { // unnamed namespace
            /** Update the given options with user-specified options from the
             * given row. Options are pre-processed where applicable.
             * If values already exist in the map they are overwritten.
             * @param options : a map of property name to option value to update
             * @param data : the row data to get option values from
             * @param allowInsertions : if true, allow new keys to be inserted;
             * otherwise, only allow updating of keys that already exist
             */
void updateRowOptions(OptionsMap &options, const RowData_sptr data,
                      const WhiteList &whitelist, const bool allowInsertions) {
  // Loop through all columns (excluding the Options and Hidden options
  // columns)
  auto columnIt = whitelist.cbegin();
  auto columnValueIt = data->constBegin();
  for (; columnIt != whitelist.cend() - 2; ++columnIt, ++columnValueIt) {
    auto column = *columnIt;
    auto &propertyName = column.algorithmProperty();

    if (allowInsertions || options.find(propertyName) != options.end()) {
      // Get the value from this column
      auto columnValue = *columnValueIt;

      // If no value, nothing to do
      if (!columnValue.isEmpty())
        options[propertyName] = columnValue;
    }
  }
}

/** Update the given options with user-specified options from the
 * Options column. If values already exist in the map they are overwritten.
 * @param options : a map of property name to option value to update
 * @param data : the data for this row
 * @param allowInsertions : if true, allow new keys to be inserted;
 * otherwise, only allow updating of keys that already exist
 */
void updateUserOptions(OptionsMap &options, const RowData_sptr data,
                       const WhiteList &whitelist, const bool allowInsertions) {
  auto userOptions =
      parseKeyValueQString(data->value(static_cast<int>(whitelist.size()) - 2));
  for (auto &kvp : userOptions) {
    if (allowInsertions || options.find(kvp.first) != options.end())
      options[kvp.first] = kvp.second;
  }
}

/** Update the given options with options from the Hidden Options
 * column. If values already exist in the map they are overwritten.
 * @param options : a map of property name to option value to update
 * @param data : the data for this row
 * @param allowInsertions : if true, allow new keys to be inserted;
 * otherwise, only allow updating of keys that already exist
 */
void updateHiddenOptions(OptionsMap &options, const RowData_sptr data,
                         const bool allowInsertions) {
  const auto hiddenOptions = parseKeyValueQString(data->back());
  for (auto &kvp : hiddenOptions) {
    if (allowInsertions || options.find(kvp.first) != options.end())
      options[kvp.first] = kvp.second;
  }
}

/** Update the given options with the output properties.
 * If values already exist in the map they are overwritten.
 * @param options : a map of property name to option value to update
 * @param data : the data for this row
 */
void updateOutputOptions(OptionsMap &options, const RowData_sptr data,
                         const bool allowInsertions,
                         const std::vector<QString> &outputPropertyNames,
                         const std::vector<QString> &outputNamePrefixes) {
  // Set the properties for the output workspace names
  for (auto i = 0u; i < outputPropertyNames.size(); i++) {
    const auto propertyName = outputPropertyNames[i];
    if (allowInsertions || options.find(propertyName) != options.end()) {
      options[propertyName] = data->reducedName(outputNamePrefixes[i]);
    }
  }
}
} // unnamed namespace

/** Parses individual values from a string containing a list of
 * values that should be preprocesed
 * @param inputStr : the input string. Multiple runs may be separated by '+' or
 * ','
 * @return : a list of strings for the individual values
 */
QStringList preprocessingStringToList(const QString &inputStr) {
  QStringList values;

  // split on comma or plus symbol
  auto str = inputStr.toStdString();
  boost::tokenizer<boost::escaped_list_separator<char>> tok(
      str, boost::escaped_list_separator<char>("\\", ",+", "\"'"));

  for (const auto &it : tok) {
    if (!it.empty())
      values.append(QString::fromStdString(it));
  }

  return values;
}

/** Join a list of preprocessing inputs into a single string
 * separated by '+' with an optional prefix
 */
QString preprocessingListToString(const QStringList &values,
                                  const QString &prefix,
                                  const QString &separator) {
  return prefix + values.join(separator);
}

/**
Returns the name of the reduced workspace for a given row
@param data :: [input] The data for this row
@param whitelist :: [input] The list of columns
@param preprocessMap :: [input] a map of column names to the preprocessing
algorithm for that column
@throws std::runtime_error if the workspace could not be prepared
@returns : The name of the workspace
*/
QString getReducedWorkspaceName(
    const RowData_sptr data, const WhiteList &whitelist,
    const std::map<QString, PreprocessingAlgorithm> &preprocessMap) {
  if (data->size() != static_cast<int>(whitelist.size()))
    throw std::invalid_argument("Can't find reduced workspace name");

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
  auto runNumbersIt = data->constBegin();
  for (; columnIt != whitelist.cend(); ++columnIt, ++runNumbersIt) {
    // Check the column is relevant to the generation of the output name
    auto column = *columnIt;
    if (!column.isShown())
      continue;

    // Check there is a value in the column
    auto const runNumbers = *runNumbersIt;
    if (runNumbers.isEmpty())
      continue;

    // Convert the string value to a list
    auto values = preprocessingStringToList(runNumbers);
    if (values.isEmpty())
      continue;

    // Get the separator to use if preprocessing multiple input values
    QString separator = "+";
    if (preprocessMap.count(column.name()))
      separator = preprocessMap.at(column.name()).separator();

    // Convert the value list to a correctly-formatted output name
    names.append(preprocessingListToString(values, column.prefix(), separator));
  } // Columns

  QString wsname;
  wsname += names.join("_");
  return wsname;
}

/** Get the algorithm property values for the main processing algorithm.  This
 * consolidates values from the global options as well as the data processor
 * table columns and the Options/HiddenOptions columns.
 *
 * @param data [in] : the row data to get option values for
 * @param globalOptions [in] : default property values from the global settings
 * @param whitelist [in] : the list of columns
 * @param allowInsertions [in] : if true, allow values to be inserted into the
 * map if they do not exist; otherwise, only update existing values
 * @param outputProperties [in] : the list of output property names
 * @param prefixes [in] : the list of prefixes to apply to output workspace
 * names
 * @return : a map of property names to value
 */
OptionsMap getCanonicalOptions(const RowData_sptr data,
                               const OptionsMap &globalOptions,
                               const WhiteList &whitelist,
                               const bool allowInsertions,
                               const std::vector<QString> &outputProperties,
                               const std::vector<QString> &prefixes) {
  // Compile all of the options into a single map - add them in reverse
  // order of precedence. Latter items are overwritten, or added if they
  // do not yet exist in the map if allowInsertions is true.
  OptionsMap options = globalOptions;
  updateHiddenOptions(options, data, allowInsertions);
  updateUserOptions(options, data, whitelist, allowInsertions);
  updateRowOptions(options, data, whitelist, allowInsertions);
  updateOutputOptions(options, data, allowInsertions, outputProperties,
                      prefixes);
  return options;
}
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
