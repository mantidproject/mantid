#include "MantidTestHelpers/DataProcessorTestHelper.h"
#include "MantidQtWidgets/Common/DataProcessorUI/TreeData.h"

using namespace MantidQt::MantidWidgets::DataProcessor;

namespace DataProcessorTestHelper {

/* Add a property value from a list to the given row data with an optional
 * prefix
 */
void addPropertyValue(RowData_sptr rowData,
                      const std::vector<std::string> &list, const size_t index,
                      const std::string &property, const std::string &prefix) {
  if (index >= list.size() || list[index].empty())
    return;

  // Set the option based on the row value
  rowData->setOptionValue(property, list[index]);
  // Set the preprocessed option based on the value and prefix
  rowData->setPreprocessedOptionValue(property, prefix + list[index]);
}

/* Add a property value to the given row data
 */
void addPropertyValue(RowData_sptr rowData, const std::string &property,
                      const std::string &value) {
  // Set the value and preprocessed value to the given value
  rowData->setOptionValue(property, value);
  rowData->setPreprocessedOptionValue(property, value);
}

/* Append a value from a list to a string. Optionally add a prefix
 * to the value from a corrsponding list of prefixes before appending
 * it to the string
 *
 * @param stringToEdit [inout] : the string to append to
 * @param list [in] : the list of values to append
 * @param prefixes [in] : the list of prefixes
 * @param i [in] : the index of the value/prefix to use
 * @separator [in] : optional separator to use when appending the
 * value
 */
void appendStringWithPrefixedValue(std::string &stringToEdit,
                                   const std::vector<std::string> &list,
                                   const std::vector<std::string> &prefixes,
                                   const size_t i,
                                   const std::string &separator = "") {
  // do nothing if string to add is empty
  if (i >= list.size() || list[i].empty())
    return;

  // add separator and string with/without prefix
  if (i >= prefixes.size() || prefixes[i].empty())
    stringToEdit += separator + list[i];
  else
    stringToEdit += separator + prefixes[i] + list[i];
}

// Utility to create a row data class from a string list of simple inputs
// (does not support multiple input runs or transmission runs, or entries
// in the options/hidden columns). Assumes input workspaces are prefixed
// with TOF_ and transmission runs with TRANS_
RowData_sptr makeRowData(const std::vector<std::string> &list,
                         const std::vector<std::string> &prefixes,
                         const size_t numSlices) {
  // Create the data and add default options
  auto rowData = std::make_shared<RowData>(list);

  if (list.size() < 1)
    return rowData;

  std::string reducedName;
  appendStringWithPrefixedValue(reducedName, list, prefixes, 0);
  appendStringWithPrefixedValue(reducedName, list, prefixes, 2, "_");

  rowData->setReducedName(QString::fromStdString(reducedName));
  addPropertyValue(rowData, "OutputWorkspace", "IvsQ_" + reducedName);
  addPropertyValue(rowData, "OutputWorkspaceBinned",
                   "IvsQ_binned_" + reducedName);
  addPropertyValue(rowData, "OutputWorkspaceWavelength",
                   "IvsLam_" + reducedName);

  // Set other options from the row data values
  addPropertyValue(rowData, list, 0, "InputWorkspace", "TOF_");
  addPropertyValue(rowData, list, 1, "ThetaIn");
  addPropertyValue(rowData, list, 2, "FirstTransmissionRun", "TRANS_");
  addPropertyValue(rowData, list, 3, "MomentumTransferMin");
  addPropertyValue(rowData, list, 4, "MomentumTransferMax");
  addPropertyValue(rowData, list, 5, "MomentumTransferStep");
  addPropertyValue(rowData, list, 6, "ScaleFactor");

  // Add some slices if requested
  std::vector<QString> workspaceProperties = {
      "InputWorkspace", "OutputWorkspace", "OutputWorkspaceBinned",
      "OutputWorkspaceWavelength"};
  for (size_t i = 0; i < numSlices; ++i) {
    QString sliceName =
        QString("_slice_") + QString::fromStdString(std::to_string(i));
    rowData->addSlice(sliceName, workspaceProperties);
  }

  return rowData;
}
}
