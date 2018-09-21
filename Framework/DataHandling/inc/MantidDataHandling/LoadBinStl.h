#include <MantidKernel/V3D.h>
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace DataHandling {

class DLLExport LoadBinStl  : public Mantid::API::Algorithm{
public:
private:
  void readStl(std::string filename);
  std::vector<uint16_t> triangle;
  std::vector<Kernel::V3D> verticies;
};
}
}