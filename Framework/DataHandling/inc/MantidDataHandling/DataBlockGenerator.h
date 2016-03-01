#ifndef MANTID_DATAHANDLING_DATABLOCKGENERATOR_H_
#define MANTID_DATAHANDLING_DATABLOCKGENERATOR_H_

#include "MantidDataHandling/DllConfig.h"
#include <memory>

namespace Mantid {
namespace DataHandling {

class DataBlock;

class DLLExport DataBlockGenerator {
public:
  DataBlockGenerator(DataBlock* datablock, int64_t startIndex);
  void operator++();
  bool isDone();
  int64_t getCurrent();
public:
  int64_t m_currentIndex;
  DataBlock* m_dataBlock;
};

} // namespace DataHandling
} // namespace Mantid

#endif