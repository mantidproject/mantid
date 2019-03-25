// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ReflTableSchema.h"
#include <utility>

namespace MantidQt {
namespace CustomInterfaces {
namespace ReflTableSchema {

ColumnIndexNameMap makeColumnIndexMap() {
  return {{COL_RUNS, RUNS},
          {COL_ANGLE, ANGLE},
          {COL_TRANSMISSION, TRANSMISSION},
          {COL_QMIN, QMINDEF},
          {COL_QMAX, QMAXDEF},
          {COL_DQQ, DQQ},
          {COL_SCALE, SCALE},
          {COL_GROUP, GROUP},
          {COL_OPTIONS, OPTIONS}};
}

ColumnNameIndexMap makeColumnNameMap() {
  auto indexMap = makeColumnIndexMap();
  ColumnNameIndexMap columnMap;
  for (auto & it : indexMap) {
    columnMap.emplace(it.second, it.first);
  }
  return columnMap;
}
} // namespace ReflTableSchema
} // namespace CustomInterfaces
} // namespace MantidQt
