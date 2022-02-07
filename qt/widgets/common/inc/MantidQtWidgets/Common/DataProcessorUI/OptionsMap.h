// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
/** This file defines utilities for handling option maps used by
    the DataProcessor widget.
    */

#include "MantidKernel/System.h"
#include "MantidQtWidgets/Common/DataProcessorUI/OptionsQMap.h"

#include <QString>
#include <map>

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
/// A map where the key is a string containing the property name and
/// the value is a string continaing the property value
using OptionsMap = std::map<QString, QString>;
/// A map where the key is a string containing the column name and the
/// value is an OptionsMap containing the properties applicable to that
/// column
using ColumnOptionsMap = std::map<QString, OptionsMap>;
/// Convert a QMap of options to a std::map of options
OptionsMap DLLExport convertOptionsFromQMap(const OptionsQMap &src);
/// Convert a QMap of column options to a std::map of column options
ColumnOptionsMap DLLExport convertColumnOptionsFromQMap(const ColumnOptionsQMap &src);
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt