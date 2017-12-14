#include "MantidQtWidgets/Common/DataProcessorUI/WorkspaceNameUtils.h"

#include "MantidQtWidgets/Common/DataProcessorUI/WhiteList.h"
#include "MantidQtWidgets/Common/ParseKeyValueString.h"

#include <QString>
#include <QStringList>

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {

/** Parses individual values from a string containing a list of
 * values that should be preprocesed
 * @param inputStr : the input string. Multiple runs may be separated by '+' or
 * ','
 * @return : a list of strings for the individual values
 */
QStringList preprocessingStringToList(const QString &inputStr) {
  auto values = inputStr.split(QRegExp("[+,]"), QString::SkipEmptyParts);
  trimWhitespaceQuotesAndEmptyValues(values);
  return values;
}

/** Join a list of preprocessing inputs into a single string
 * separated by '+' with an optional prefix
 */
QString preprocessingListToString(const QStringList &values,
                                  const QString prefix) {
  return prefix + values.join("+");
}

/**
Returns the name of the reduced workspace for a given row
@param data :: [input] The data for this row
@param prefix : A prefix to be appended to the generated ws name
@throws std::runtime_error if the workspace could not be prepared
@returns : The name of the workspace
*/
QString getReducedWorkspaceName(const QStringList &data,
                                const WhiteList &whitelist,
                                const QString prefix) {
  if (data.size() != static_cast<int>(whitelist.size()))
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
  auto runNumbersIt = data.constBegin();
  for (; columnIt != whitelist.cend(); ++columnIt, ++runNumbersIt) {
    auto column = *columnIt;
    // Do we want to use this column to generate the name of the output ws?
    if (column.isShown()) {
      auto const runNumbers = *runNumbersIt;

      if (!runNumbers.isEmpty()) {
        auto values = preprocessingStringToList(runNumbers);
        if (!values.isEmpty())
          names.append(preprocessingListToString(values, column.prefix()));
      }
    }
  } // Columns

  auto wsname = prefix;
  wsname += names.join("_");
  return wsname;
}
}
}
}
