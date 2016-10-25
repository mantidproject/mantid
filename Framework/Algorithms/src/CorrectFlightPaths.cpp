#include "MantidAlgorithms/CorrectFlightPaths.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument/ComponentHelper.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/UnitFactory.h"

#include <cmath>

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace API;
using namespace Geometry;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(CorrectFlightPaths)

} // namespace Algorithm
} // namespace Mantid
