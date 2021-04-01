// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

/** Utilities for finding the output name of reduced workspaces based
on the reduction algorithm's input values and preprocessing settings
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
QString preprocessingListToString(const QStringList &values, const QString &prefix, const QString &separator);
// Returns the name of the reduced workspace for a given row
QString DLLExport getReducedWorkspaceName(const RowData_sptr &data, const WhiteList &whitelist,
                                          const std::map<QString, PreprocessingAlgorithm> &preprocessor);
// Consolidate global options with row values
OptionsMap DLLExport getCanonicalOptions(const RowData_sptr &data, const OptionsMap &globalOptions,
                                         const WhiteList &whitelist, const bool allowInsertions,
                                         const std::vector<QString> &outputProperties = std::vector<QString>(),
                                         const std::vector<QString> &prefixes = std::vector<QString>());
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
