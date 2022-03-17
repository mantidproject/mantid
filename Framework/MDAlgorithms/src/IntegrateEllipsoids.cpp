#include "MantidMDAlgorithms/IntegrateEllipsoids.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/Property.h"
#include "MantidMDAlgorithms/IntegrateEllipsoidsV1.h"
#include "MantidMDAlgorithms/IntegrateEllipsoidsV2.h"

#include <boost/algorithm/string.hpp>

using namespace Mantid::API;
using namespace Mantid::HistogramData;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

namespace Mantid::MDAlgorithms {

DECLARE_ALGORITHM(IntegrateEllipsoids)
//---------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string IntegrateEllipsoids::name() const { return "IntegrateEllipsoids"; }

/// Algorithm's version for identification. @see Algorithm::version
int IntegrateEllipsoids::version() const { return 3; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string IntegrateEllipsoids::category() const { return "Crystal\\Integration"; }
//---------------------------------------------------------------------

void IntegrateEllipsoids::init() {
  IntegrateEllipsoidsV1::initInstance(*this);
  IntegrateEllipsoidsV2::initInstance(*this);
}

int getIndexCount(PeaksWorkspace_sptr peakWorkspace) {
  int indexCount = 0;
  const int numPeaks = peakWorkspace->getNumberPeaks();
  for (int i = 0; i < numPeaks; ++i) {
    const auto peak = peakWorkspace->getPeak(i);
    if (peak.getHKL().norm2() > 0)
      indexCount += 1;
  }
  return indexCount;
}

void IntegrateEllipsoids::exec() {
  const bool isIntegrateInHKL = getProperty("IntegrateInHKL");
  const bool isGetUBFromPeaksWorkspace = getProperty("GetUBFromPeaksWorkspace");
  const bool shareBackground = getProperty("ShareBackground");

  const PeaksWorkspace_sptr peakWorkspace = getProperty("PeaksWorkspace");

  const int indexCount = getIndexCount(peakWorkspace);

  Algorithm_sptr alg;

  // detect which algo to run
  if ((isIntegrateInHKL || isGetUBFromPeaksWorkspace) && indexCount > 0 && !shareBackground) {
    // v1
    alg = std::dynamic_pointer_cast<Algorithm>(createChildAlgorithm("IntegrateEllipsoids", -1., -1., true, 1));
  } else {
    // v2
    alg = std::dynamic_pointer_cast<Algorithm>(createChildAlgorithm("IntegrateEllipsoids", -1., -1., true, 2));
  }
  // forward properties to algo
  alg->copyPropertiesFrom(*this);
  // run correct algo and return results
  alg->execute();
  if (!alg->isExecuted())
    throw std::runtime_error("IntegrateEllipsoids Algorithm has not executed successfully");

  PeaksWorkspace_sptr outputWorkspace = alg->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace", outputWorkspace);
}
} // namespace Mantid::MDAlgorithms
