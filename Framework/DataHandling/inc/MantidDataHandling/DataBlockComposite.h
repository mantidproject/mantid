#ifndef MANTID_DATAHANDLING_DATABLOCK_COMPOSITE_H_
#define MANTID_DATAHANDLING_DATABLOCK_COMPOSITE_H_

#include "MantidDataHandling/DataBlock.h"
#include "MantidDataHandling/DllConfig.h"

namespace Mantid {
namespace DataHandling {

class DLLExport DataBlockComposite : public DataBlock {
public:
  static const int64_t end;

  int64_t getMinSpectrumID() const override;
  void setMinSpectrumID(int64_t) override;

  int64_t getMaxSpectrumID() const override;
  void setMaxSpectrumID(int64_t) override;

  std::unique_ptr<DataBlockGenerator> getGenerator();
  int64_t getNextSpectrumID(int64_t spectrumID) const;

  void addDataBlock(DataBlock dataBlock);

private:
  std::vector<DataBlock> m_dataBlocks;
};
}
}

#endif
