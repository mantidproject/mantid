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
int IntegrateEllipsoids::version() const { return 1; }

<<<<<<< HEAD
/// Only convert to Q-vector.
const std::string Q3D("Q3D");

/// Q-vector is always three dimensional.
const std::size_t DIMS(3);

void IntegrateEllipsoids::qListFromEventWS(IntegrateQLabEvents &integrator, Progress &prog, EventWorkspace_sptr &wksp) {
  auto numSpectra = static_cast<int>(wksp->getNumberHistograms());
  PARALLEL_FOR_IF(Kernel::threadSafe(*wksp))
  for (int i = 0; i < numSpectra; ++i) {
    PARALLEL_START_INTERRUPT_REGION

    // units conversion helper
    UnitsConversionHelper unitConverter;
    unitConverter.initialize(m_targWSDescr, "Momentum");

    // initialize the MD coordinates conversion class
    MDTransfQ3D qConverter;
    qConverter.initialize(m_targWSDescr);

    std::vector<double> buffer(DIMS);
    // get a reference to the event list
    EventList &events = wksp->getSpectrum(i);

    events.switchTo(WEIGHTED_NOTIME);
    events.compressEvents(1e-5, &events);

    // check to see if the event list is empty
    if (events.empty()) {
      prog.report();
      continue; // nothing to do
    }

    // update which pixel is being converted
    std::vector<Mantid::coord_t> locCoord(DIMS, 0.);
    unitConverter.updateConversion(i);
    qConverter.calcYDepCoordinates(locCoord, i);

    // loop over the events
    double signal(1.);  // ignorable garbage
    double errorSq(1.); // ignorable garbage
    const std::vector<WeightedEventNoTime> &raw_events = events.getWeightedEventsNoTime();
    std::vector<std::pair<std::pair<double, double>, V3D>> qList;
    for (const auto &raw_event : raw_events) {
      double val = unitConverter.convertUnits(raw_event.tof());
      qConverter.calcMatrixCoord(val, locCoord, signal, errorSq);
      for (size_t dim = 0; dim < DIMS; ++dim) {
        buffer[dim] = locCoord[dim];
      }
      V3D qVec(buffer[0], buffer[1], buffer[2]);
      qList.emplace_back(std::pair<double, double>(raw_event.m_weight, raw_event.m_errorSquared), qVec);
    } // end of loop over events in list
    PARALLEL_CRITICAL(addEvents) { integrator.addEvents(qList); }

    prog.report();
    PARALLEL_END_INTERRUPT_REGION
  } // end of loop over spectra
  PARALLEL_CHECK_INTERRUPT_REGION
  integrator.populateCellsWithPeaks();
}

void IntegrateEllipsoids::qListFromHistoWS(IntegrateQLabEvents &integrator, Progress &prog, Workspace2D_sptr &wksp) {
  auto numSpectra = static_cast<int>(wksp->getNumberHistograms());
  PARALLEL_FOR_IF(Kernel::threadSafe(*wksp))
  for (int i = 0; i < numSpectra; ++i) {
    PARALLEL_START_INTERRUPT_REGION

    // units conversion helper
    UnitsConversionHelper unitConverter;
    unitConverter.initialize(m_targWSDescr, "Momentum");

    // initialize the MD coordinates conversion class
    MDTransfQ3D qConverter;
    qConverter.initialize(m_targWSDescr);

    std::vector<double> buffer(DIMS);
    // get tof and y values
    const auto &xVals = wksp->points(i);
    const auto &yVals = wksp->y(i);
    const auto &eVals = wksp->e(i);

    // update which pixel is being converted
    std::vector<Mantid::coord_t> locCoord(DIMS, 0.);
    unitConverter.updateConversion(i);
    qConverter.calcYDepCoordinates(locCoord, i);

    // loop over the events
    double signal(1.);  // ignorable garbage
    double errorSq(1.); // ignorable garbage

    SlimEvents qList;
    for (size_t j = 0; j < yVals.size(); ++j) {
      const double &yVal = yVals[j];
      const double &esqVal = eVals[j] * eVals[j]; // error squared (variance)
      if (yVal > 0)                               // TODO, is this condition right?
      {
        double val = unitConverter.convertUnits(xVals[j]);
        qConverter.calcMatrixCoord(val, locCoord, signal, errorSq);
        for (size_t dim = 0; dim < DIMS; ++dim) {
          buffer[dim] = locCoord[dim]; // TODO. Looks un-necessary to me. Can't
                                       // we just add localCoord directly to
                                       // qVec
        }
        V3D qVec(buffer[0], buffer[1], buffer[2]);
        if (std::isnan(qVec[0]) || std::isnan(qVec[1]) || std::isnan(qVec[2]))
          continue;
        // Account for counts in histograms by increasing the qList with the
        // same q-point
        qList.emplace_back(std::pair<double, double>(yVal, esqVal), qVec);
      }
    }
    PARALLEL_CRITICAL(addHisto) { integrator.addEvents(qList); }
    prog.report();
    PARALLEL_END_INTERRUPT_REGION
  } // end of loop over spectra
  PARALLEL_CHECK_INTERRUPT_REGION
  integrator.populateCellsWithPeaks();
}
=======
/// Algorithm's category for identification. @see Algorithm::category
const std::string IntegrateEllipsoids::category() const { return "Crystal\\Integration"; }
//---------------------------------------------------------------------
>>>>>>> added wrapper algo, wip test

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
    alg = std::dynamic_pointer_cast<Algorithm>(createChildAlgorithm("IntegrateEllipsoidsV1"));
  } else {
    // v2
    alg = std::dynamic_pointer_cast<Algorithm>(createChildAlgorithm("IntegrateEllipsoidsV2"));
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
