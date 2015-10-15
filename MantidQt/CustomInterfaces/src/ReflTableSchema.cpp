#include "MantidQtCustomInterfaces/ReflTableSchema.h"
#include <utility>

namespace MantidQt {
namespace CustomInterfaces {
namespace ReflTableSchema {

ColumnIndexNameMap makeColumnIndexMap(){
    ColumnIndexNameMap columnMap;
    columnMap.insert(std::make_pair(COL_RUNS, RUNS));
    columnMap.insert(std::make_pair(COL_ANGLE, ANGLE));
    columnMap.insert(std::make_pair(COL_TRANSMISSION, TRANSMISSION));
    columnMap.insert(std::make_pair(COL_QMIN, QMINDEF));
    columnMap.insert(std::make_pair(COL_QMAX, QMAXDEF));
    columnMap.insert(std::make_pair(COL_DQQ, DQQ));
    columnMap.insert(std::make_pair(COL_SCALE, SCALE));
    columnMap.insert(std::make_pair(COL_GROUP, GROUP));
    columnMap.insert(std::make_pair(COL_OPTIONS, OPTIONS));
    return columnMap;
}

}
} // namespace CustomInterfaces
} // namespace Mantid
