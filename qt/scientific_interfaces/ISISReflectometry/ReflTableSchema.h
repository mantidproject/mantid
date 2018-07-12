#ifndef MANTID_ISISREFLECTOMETRY_REFLTABLESCHEMA_H_
#define MANTID_ISISREFLECTOMETRY_REFLTABLESCHEMA_H_

#include "DllConfig.h"
#include "string"
#include "map"

namespace MantidQt {
namespace CustomInterfaces {
namespace ReflTableSchema {

using ColumnNameType = std::string;
using ColumnValueType = std::string;
using ColumnIndexNameMap = std::map<int, ColumnNameType>;
using ColumnNameIndexMap = std::map<ColumnNameType, int>;

/// Label for run number column
static const std::string RUNS("Run(s)");
/// Label for angle column
static const std::string ANGLE("Angle");
/// Label for transmission column
static const std::string TRANSMISSION("Transmission Run(s)");
/// Label for qmin column
static const std::string QMINDEF("Q min");
/// Label for qmax column
static const std::string QMAXDEF("Q max");
/// Label for dq/q column
static const std::string DQQ("dQ/Q");
/// Label for scale column
static const std::string SCALE("Scale");
/// Label for group column
static const std::string GROUP("Group");
/// Label for options column
static const std::string OPTIONS("Options");

/// Index for run number column
const int COL_RUNS(0);
/// Index for angle column
const int COL_ANGLE(1);
/// Index for transmission column
const int COL_TRANSMISSION(2);
/// Index for qmin column
const int COL_QMIN(3);
/// Index for qmax column
const int COL_QMAX(4);
/// Index for dq/q column
const int COL_DQQ(5);
/// Index for scale column
const int COL_SCALE(6);
/// Index for group column
const int COL_GROUP(7);
/// Index for options column
const int COL_OPTIONS(8);

/// Make the column index map.
ColumnIndexNameMap makeColumnIndexMap();
/// Make the column name map.
ColumnNameIndexMap makeColumnNameMap();

} // ReflTableSchema
} // namespace CustomInterfaces
} // namespace Mantid

#endif /* MANTID_ISISREFLECTOMETRY_REFLTABLESCHEMA_H_ */
