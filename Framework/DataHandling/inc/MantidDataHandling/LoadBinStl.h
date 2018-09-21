#ifndef MANTID_DATAHANDLING_LOADBINSTL_H_
#define MANTID_DATAHANDLING_LOADBINSTL_H_
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/BinaryStreamReader.h"
#include <MantidKernel/V3D.h>
namespace Mantid {
namespace DataHandling {

class DLLExport LoadBinStl : public Mantid::API::Algorithm {
public:
private:
  void readStl(std::string filename);
  void readTriangle(Kernel::BinaryStreamReader);
  std::vector<uint16_t> triangle;
  std::vector<Kernel::V3D> verticies;
};
} // namespace DataHandling
} // namespace Mantid
#endif /* MANTID_DATAHANDLING_LOADBINSTL_H_ */