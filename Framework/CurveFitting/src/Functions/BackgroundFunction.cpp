//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/BackgroundFunction.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

using namespace CurveFitting;

using namespace Kernel;

using namespace API;

void BackgroundFunction::fit(const std::vector<double> &X,
                             const std::vector<double> &Y) {
  (void)X; // Avoid compiler warning
  (void)Y; // Avoid compiler warning
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
