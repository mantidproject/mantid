#include "MantidQtWidgets/Common/DataProcessorUI/TreeData.h"
namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {

// Constructors
RowData::RowData() {}

RowData::RowData(QStringList data) : m_data(std::move(data)) {}

RowData::RowData(const RowData *src)
    : m_data(src->m_data), m_options(src->m_options),
      m_preprocessedOptions(src->m_preprocessedOptions) {}

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
 */
void RowData::setValue(const int i, const QString &value) {
  if (m_data.size() > i)
    m_data[i] = value;
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

/** Get the value for the given property
 * @param name [in] : the property name to get
 * @return : the value, or an empty string if the property
 * doesn't exist
 */
QString RowData::optionValue(const QString &name) const {
  return hasOption(name) ? m_options.at(name) : "";
}

/** Set the value for the given property
 * @param name [in] : the property to set
 * @param value [in] : the value
 */
void RowData::setOptionValue(const QString &name, const QString &value) {
  m_options[name] = value;
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
RowData_sptr RowData::addSlice(const QString &sliceSuffix,
                               std::vector<QString> &workspaceProperties) {
  // Create a copy
  auto sliceData = std::make_shared<RowData>(this);
  // Override the workspace name in options
  for (auto const &propertyName : workspaceProperties) {
    if (hasPreprocessedOption(propertyName)) {
      auto const sliceName = m_preprocessedOptions[propertyName] + sliceSuffix;
      sliceData->m_preprocessedOptions[propertyName] = sliceName;
    }
  }
  // Add to list of slices
  m_slices.push_back(sliceData);

  return sliceData;
}
}
}
}
