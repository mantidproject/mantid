#include <MantidKernel/V3D.h>
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace DataHandling {

class DLLExport LoadBinStl  : public Mantid::API::Algorithm{
public:
private:
  Kernel::V3D makeV3(char *facet);
  void readStl(std::string filename);
  std::vector<uint16_t> triangle;
  std::vector<Kernel::V3D> verticies;
};
}
}