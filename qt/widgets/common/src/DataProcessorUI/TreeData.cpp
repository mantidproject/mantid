// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Common/DataProcessorUI/TreeData.h"

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
bool canPostprocess(GroupData const &group) { return group.size() > 1; }

// Constructors
RowData::RowData(const int columnCount) : m_isProcessed{false} {
  // Create a list of empty column values of the required length
  for (int i = 0; i < columnCount; ++i)
    m_data.append("");
}

RowData::RowData(QStringList data)
    : m_data(std::move(data)), m_isProcessed{false} {}

RowData::RowData(const std::vector<std::string> &data) : m_isProcessed{false} {
  for (auto &value : data)
    m_data.append(QString::fromStdString(value));
}

RowData::RowData(const RowData &src)
    : m_data(src.m_data), m_options(src.m_options),
      m_preprocessedOptions(src.m_preprocessedOptions), m_isProcessed{false} {}

// Iterators

QList<QString>::iterator RowData::begin() { return m_data.begin(); }

QList<QString>::iterator RowData::end() { return m_data.end(); }

QList<QString>::const_iterator RowData::constBegin() const {
  return m_data.constBegin();
}

QList<QString>::const_iterator RowData::constEnd() const {
  return m_data.constEnd();
}

QString RowData::back() const { return m_data.back(); }

QString RowData::operator[](int i) const { return m_data[i]; }

/** Return all of the data values in the row
 * @return : the values as a string list
 */
QStringList RowData::data() const { return m_data; }

/** Return a data value
 * @param i [in] : the column index of the value
 * @return : the value in that column
 */
QString RowData::value(const int i) {
  return m_data.size() > i ? m_data.at(i) : "";
}

/** Set a data value
 * @param i [in] : the index of the value to set
 * @param value [in] : the new value
 * @param isGenerated [in] : indicates whether the value is
 * auto-generated or user-entered
 */
void RowData::setValue(const int i, const QString &value,
                       const bool isGenerated) {
  // Set the row value
  if (m_data.size() > i) {
    m_data[i] = value;
  }

  if (isGenerated)
    m_generatedColumns.insert(i);
  else
    m_generatedColumns.erase(i);

  // Also update the value in any child slices
  if (m_slices.size() > 0) {
    for (auto &slice : m_slices)
      slice->setValue(i, value);
  }
}

/** Get the original input options for the main reduction
 * algorithm before preprocessing i.e. as entered by the
 * user
 * @return : the options as a map of property name to value
 */
OptionsMap RowData::options() const { return m_options; }

/** Get the input options for the main reduction algorithm
 * after preprocessing has been performed on them
 * @return : the options as a map of property name to value
 */
OptionsMap RowData::preprocessedOptions() const {
  return m_preprocessedOptions;
}

/** Set the options
 * @param options [in] : the options to set as a map of property
 * name to value
 */
void RowData::setOptions(OptionsMap options) { m_options = std::move(options); }

/** Set the preprocessed options
 * @param options [in] : the options to set as a map of property
 * name to value
 */
void RowData::setPreprocessedOptions(OptionsMap options) {
  m_preprocessedOptions = std::move(options);
}

/** Get the number of fields in the row data
 * @return : the number of fields
 */
int RowData::size() const { return m_data.size(); }

/** Check whether a cell value was auto-generated (i.e. has been populated with
 * a result of the algorithm rather than being entered by the user)
 * @param i : the column index of the cell to check
 */
bool RowData::isGenerated(const int i) const {
  return (m_generatedColumns.count(i) > 0);
}

/** Check whether the given property exists in the options
 * @return : true if the property exists
 */
bool RowData::hasOption(const QString &name) const {
  return m_options.find(name) != m_options.end();
}

/** Check whether the given property exists in the options
 * @return : true if the property exists
 */
bool RowData::hasPreprocessedOption(const QString &name) const {
  return m_preprocessedOptions.find(name) != m_preprocessedOptions.end();
}

/** Get the value for the given property as it was entered by the user
 * i.e. before any preprocessing
 * @param name [in] : the property name to get
 * @return : the value, or an empty string if the property
 * doesn't exist
 */
QString RowData::optionValue(const QString &name) const {
  return hasOption(name) ? m_options.at(name) : "";
}

/** Get the value for the given property for the given slice
 * @param name [in] : the property name to get
 * @param sliceIndex [in] : the index of the slice
 * @return : the value, or an empty string if the property
 * doesn't exist
 */
QString RowData::optionValue(const QString &name,
                             const size_t sliceIndex) const {
  if (sliceIndex >= m_slices.size())
    throw std::runtime_error("Attempted to access an invalid slice");

  return m_slices[sliceIndex]->optionValue(name);
}

/** Get the value for the given property after any preprocessing of
 * user input
 * @param name [in] : the property name to get
 * @return : the value, or an empty string if the property
 * doesn't exist
 */
QString RowData::preprocessedOptionValue(const QString &name) const {
  return hasPreprocessedOption(name) ? m_preprocessedOptions.at(name) : "";
}

/** Set the value for the given property
 * @param name [in] : the property to set
 * @param value [in] : the value
 */
void RowData::setOptionValue(const QString &name, const QString &value) {
  m_options[name] = value;
}

/** Set the value for the given property
 * @param name [in] : the property to set
 * @param value [in] : the value
 */
void RowData::setOptionValue(const std::string &name,
                             const std::string &value) {
  setOptionValue(QString::fromStdString(name), QString::fromStdString(value));
}

/** Set the preprocessed value for the given property
 * @param name [in] : the property to set
 * @param value [in] : the value
 */
void RowData::setPreprocessedOptionValue(const QString &name,
                                         const QString &value) {
  m_preprocessedOptions[name] = value;
}

/** Set the preprocessed value for the given property
 * @param name [in] : the property to set
 * @param value [in] : the value
 */
void RowData::setPreprocessedOptionValue(const std::string &name,
                                         const std::string &value) {
  setPreprocessedOptionValue(QString::fromStdString(name),
                             QString::fromStdString(value));
}

/** Get the number of slices for this row
 * @return : the number of slices
 */
size_t RowData::numberOfSlices() const { return m_slices.size(); }

/** Check whether a slice exists in this row
 * @param sliceIndex [in] : the index of the slice
 * @return : true if the slice exists
 */
bool RowData::hasSlice(const size_t sliceIndex) {
  return (sliceIndex < m_slices.size());
}

/** Get a child slice
 * @param sliceIndex [in] : the index of the slice
 * @return : the slice's row data
 */
RowData_sptr RowData::getSlice(const size_t sliceIndex) {
  if (sliceIndex >= m_slices.size())
    throw std::runtime_error("Attempted to access an invalid slice");

  return m_slices[sliceIndex];
}

/** Add a child slice for this row with the given name.
 * The child slice takes the same options and data values
 * as its parent row except for the input workspace name.
 * @param sliceSuffix [in] : the suffix to use for the slice workspace name
 * @param workspaceProperties [in] : a list of algorithm properties that are
 * workspace names. The property values for these will be updated to include
 * the slice suffix in the options for the new slice.
 * @return : the row data for the new slice
 */
RowData_sptr
RowData::addSlice(const QString &sliceSuffix,
                  const std::vector<QString> &workspaceProperties) {
  // Create a copy
  auto sliceData = std::make_shared<RowData>(*this);
  for (auto const &propertyName : workspaceProperties) {
    // Add the slice suffix to the reduced workspace name
    sliceData->setReducedName(reducedName() + sliceSuffix);
    // Override the workspace names in the preprocessed options with the slice
    // suffix
    if (hasPreprocessedOption(propertyName)) {
      auto const sliceName = m_preprocessedOptions[propertyName] + sliceSuffix;
      sliceData->m_preprocessedOptions[propertyName] = sliceName;
    }
    // Same for the un-preprocessed options
    if (hasOption(propertyName)) {
      auto const sliceName = m_options[propertyName] + sliceSuffix;
      sliceData->m_options[propertyName] = sliceName;
    }
  }
  // Add to list of slices
  m_slices.push_back(sliceData);

  return sliceData;
}

/** Reset the row to how it was before it was processed. This clears the
 * processed state, errors, generated values etc. It doesn't change the row
 * data other than clearing generated values i.e. it leaves user-entered inputs
 * unchanged
 */
void RowData::reset() {
  // Clear processed state and error
  setProcessed(false);
  setError("");

  // Clear the cache of algorithm properties used
  setOptions(OptionsMap());
  setPreprocessedOptions(OptionsMap());

  // Clear generated values
  for (auto columnIndex : m_generatedColumns) {
    setValue(columnIndex, "");
  }
  m_generatedColumns.clear();
}

/** Clear all child slices for this row
 */
void RowData::clearSlices() { m_slices.clear(); }

/** Check whether reduction failed for this row (or any of its slices)
 */
bool RowData::reductionFailed() const {
  if (!m_error.empty())
    return true;

  return std::any_of(m_slices.cbegin(), m_slices.cend(), [](const auto &slice) {
    return slice->reductionFailed();
  });
}

/** Return the canonical reduced workspace name i.e. before any
 * prefixes have been applied for specific output properties.
 * @param prefix [in] : if not empty, apply this prefix to the name
 */
QString RowData::reducedName(const QString prefix) const {
  if (prefix.isEmpty())
    return m_reducedName;
  else
    return prefix + m_reducedName;
}

/** Check if this row has an output workspace with the given workspace name
 * and prefix (including any slices)
 */
bool RowData::hasOutputWorkspaceWithNameAndPrefix(const QString &workspaceName,
                                                  const QString &prefix) const {
  if (reducedName(prefix) == workspaceName) {
    return true;
  }
  for (auto slice : m_slices) {
    if (slice->hasOutputWorkspaceWithNameAndPrefix(workspaceName, prefix))
      return true;
  }
  return false;
}
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
