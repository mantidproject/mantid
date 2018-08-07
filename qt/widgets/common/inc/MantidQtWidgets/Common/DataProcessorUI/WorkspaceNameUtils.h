#ifndef MANTIDQTMANTIDWIDGETSDATAPROCESSOR_WORKSPACENAMEUTILS_H
#define MANTIDQTMANTIDWIDGETSDATAPROCESSOR_WORKSPACENAMEUTILS_H

/** Utilities for finding the output name of reduced workspaces based
on the reduction algorithm's input values and preprocessing settings

Copyright &copy; 2011-14 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

#include "MantidKernel/System.h"
#include "MantidQtWidgets/Common/DataProcessorUI/OptionsMap.h"
#include "MantidQtWidgets/Common/DataProcessorUI/TreeData.h"

#include <QString>
#include <QStringList>

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {

class WhiteList;

// Create list of trimmed values from a string
QStringList preprocessingStringToList(const QString &inputStr);
// Create string of trimmed values from a list of values
QString preprocessingListToString(const QStringList &values,
                                  const QString &prefix,
                                  const QString &separator);
// Returns the name of the reduced workspace for a given row
QString DLLExport getReducedWorkspaceName(
    const RowData_sptr data, const WhiteList &whitelist,
    const std::map<QString, PreprocessingAlgorithm> &preprocessor);
// Consolidate global options with row values
OptionsMap DLLExport getCanonicalOptions(
    const RowData_sptr data, const OptionsMap &globalOptions,
    const WhiteList &whitelist, const bool allowInsertions,
    const std::vector<QString> &outputProperties = std::vector<QString>(),
    const std::vector<QString> &prefixes = std::vector<QString>());
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDQTMANTIDWIDGETSDATAPROCESSOR_WORKSPACENAMEUTILS_H
