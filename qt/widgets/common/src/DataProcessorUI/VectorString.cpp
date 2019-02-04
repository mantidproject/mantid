// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/DataProcessorUI/VectorString.h"
namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
/** Create a comma separated list of items from a QStringList.
@param items : The strings in the list.
@return The comma separated list of items.
*/
QString vectorString(const QStringList &items) { return items.join(", "); }
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
