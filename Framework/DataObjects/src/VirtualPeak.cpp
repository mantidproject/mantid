
#include "MantidDataObjects/VirtualPeak.h"
#include "MantidKernel/V3D.h"

using namespace Mantid::Kernel;

namespace Mantid {
namespace DataObjects {

    V3D VirtualPeak::getDetPos() const {
        return V3D(0, 0, 0);
    }
}
}
