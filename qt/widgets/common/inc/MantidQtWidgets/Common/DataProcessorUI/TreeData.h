#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORTREEDATA_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORTREEDATA_H
/** This file defines the RowData, GroupData and TreeData type aliases used by
   the
    DataProcessor widget.

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
    National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */

#include <QStringList>
#include "MantidKernel/System.h"
#include "MantidQtWidgets/Common/DataProcessorUI/OptionsMap.h"
#include "MantidQtWidgets/Common/DataProcessorUI/PostprocessingAlgorithm.h"
#include "MantidQtWidgets/Common/DataProcessorUI/PreprocessingAlgorithm.h"
#include "MantidQtWidgets/Common/DataProcessorUI/ProcessingAlgorithm.h"
#include "MantidQtWidgets/Common/DataProcessorUI/TreeData.h"
#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidQtWidgets/Common/DataProcessorUI/WhiteList.h"

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
class RowData;
using RowData_sptr = std::shared_ptr<RowData>;

/**
* A class representing the data and properties for a row in the data processor
* table. Historically this was just a QStringList and currently this class just
* wraps the QStringList and adds some metadata.
*/
class DLLExport RowData {
public:
  // Constructors
  explicit RowData(const int columnCount);
  explicit RowData(QStringList data);
  explicit RowData(const std::vector<std::string> &data);
  explicit RowData(const RowData &src);

  // Iterators
  QList<QString>::iterator begin();
  QList<QString>::iterator end();
  QList<QString>::const_iterator constBegin() const;
  QList<QString>::const_iterator constEnd() const;
  QString back() const;
  QString operator[](int i) const;

  /// Return all of the data values
  QStringList data() const;
  /// Return the data value at the given index
  QString value(const int i);
  /// Set the data value at the given index
  void setValue(const int i, const QString &value);

  /// Get the algorithm input properties
  OptionsMap options() const;
  /// Get the preprocessed algorithm input properties
  OptionsMap preprocessedOptions() const;
  /// Set the algorithm input properties
  void setOptions(OptionsMap options);
  /// Set the preprocessed algorithm properties
  void setPreprocessedOptions(OptionsMap options);

  // Get the number of fields in the data
  int size() const;

  /// Check if a property exists
  bool hasOption(const QString &name) const;
  /// Return a property value
  QString optionValue(const QString &name) const;
  /// Return a property value
  QString optionValue(const QString &name, const size_t sliceIndex) const;
  /// Return a preprocessed property value
  QString preprocessedOptionValue(const QString &name) const;
  /// Set a property value
  void setOptionValue(const QString &name, const QString &value);
  /// Set a property value
  void setOptionValue(const std::string &name, const std::string &value);
  /// Set a preprocessed property value
  void setPreprocessedOptionValue(const QString &name, const QString &value);
  /// Set a preprocessed property value
  void setPreprocessedOptionValue(const std::string &name,
                                  const std::string &value);
  /// Get the number of slices for this row
  size_t numberOfSlices() const;
  /// Check whether a slice exists by index
  bool hasSlice(const size_t sliceIndex);
  /// Get a child slice
  RowData_sptr getSlice(const size_t sliceIndex);
  /// Add a child slice
  RowData_sptr addSlice(const QString &sliceSuffix,
                        const std::vector<QString> &workspaceProperties);
  /// Clear all slices from the row
  void clearSlices();

  /// Whether the row has been processed
  bool isProcessed() const { return m_isProcessed; }
  /// Set whether the row has been processed
  void setProcessed(const bool isProcessed) { m_isProcessed = isProcessed; }
  /// Set an error message for this row
  void setError(const std::string &error) { m_error = error; }
  /// Whether reduction failed for this row
  bool reductionFailed() const;

  /// Get the reduced workspace name, optionally adding a prefix
  QString reducedName(const QString prefix = QString());
  /// Set the reduced workspace name
  void setReducedName(const QString &name) { m_reducedName = name; }

private:
  /// Check if a preprocessed property exists
  bool hasPreprocessedOption(const QString &name) const;

  /// The row data as a list of string values
  QStringList m_data;
  /// Original input options for the main reduction algorithm
  OptionsMap m_options;
  /// Input options for the main reduction after they have been
  /// preprocessed
  OptionsMap m_preprocessedOptions;
  /// For sliced event data the original row gets split into multiple
  /// slices
  std::vector<RowData_sptr> m_slices;
  /// Whether the row has been processed
  bool m_isProcessed;
  /// The error message, if reduction failed for this row
  std::string m_error;
  /// The canonical reduced workspace name for this row i.e. prior to any
  /// prefixes being added for specific output workspace properties
  QString m_reducedName;
};

using GroupData = std::map<int, RowData_sptr>;
using TreeData = std::map<int, GroupData>;
EXPORT_OPT_MANTIDQT_COMMON bool canPostprocess(GroupData const &group);
}
}
}
#endif // MANTIDQTMANTIDWIDGETS_DATAPROCESSORTREEDATA_H
