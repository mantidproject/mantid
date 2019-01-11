// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSOROPTIONSQMAP_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSOROPTIONSQMAP_H
/** This file defines utilities for handling option maps used by the data
    processor widget. These are QVariantMap equivalents of the types in
    OptionsMap.h. The QVariantMap types are required in the sip conversions for
    Python and should only be used where required for functions that are
    exposed to Python; otherwise, you should use the types in OptionsMap.h
    instead.
    */

#include <QVariantMap>

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
/// A QMap where the key is a string containing the property name and
/// the value is a string continaing the property value
using OptionsQMap = QVariantMap;
/// A QMap where the key is a string containing the column name and the
/// value is an OptionsQMap containing the properties applicable to that
/// column
using ColumnOptionsQMap = QVariantMap;
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
#endif // MANTIDQTMANTIDWIDGETS_DATAPROCESSOROPTIONSQMAP_H
