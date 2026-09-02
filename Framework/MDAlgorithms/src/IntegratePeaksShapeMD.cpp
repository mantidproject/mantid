// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/IntegratePeaksShapeMD.h"

#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Run.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeakShapeEllipsoid.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"

#include "MantidMDAlgorithms/Integrate3DEvents.h"
#include "MantidMDAlgorithms/MDTransfFactory.h"
#include "MantidMDAlgorithms/MDTransfQ3D.h"
#include "MantidMDAlgorithms/UnitsConversionHelper.h"

#include <boost/math/special_functions/round.hpp>
#include <cmath>
#include <string>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

namespace Mantid::MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(IntegratePeaksShapeMD)

/// Algorithm's name for identification. @see Algorithm::name
const std::string IntegratePeaksShapeMD::name() const { return "IntegratePeaksShapeMD"; }

/// Algorithm's version for identification. @see Algorithm::version
int IntegratePeaksShapeMD::version() const { return 1; }

/**
 * @brief Identifies the algorithm's category.
 *
 * @return std::string The category path, "Crystal\\Integration".
 */
const std::string IntegratePeaksShapeMD::category() const { return "Crystal\\Integration"; }

/**
 * @brief Defines the input, integration, fitting, and output properties for the algorithm.
 */
void IntegratePeaksShapeMD::init() {
  auto ws_valid = std::make_shared<CompositeValidator>();
  ws_valid->add<InstrumentValidator>();

  auto mustBePositive = std::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);

  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input, ws_valid),
      "An input MatrixWorkspace with time-of-flight units along "
      "X-axis and defined instrument with defined sample");

  declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>("PeaksWorkspace", "", Direction::InOut),
                  "Workspace with peaks to be integrated. Each peak must already have an "
                  "ellipsoidal shape set, e.g. from a previous IntegrateEllipsoids run.");

  declareProperty("RegionRadius", .35, mustBePositive,
                  "Only events at most this distance from a peak will be considered when "
                  "integrating. Must be at least as large as the largest background outer "
                  "radius among the peaks being integrated, or the background shell will be "
                  "truncated.");

  declareProperty("UseOnePercentBackgroundCorrection", true,
                  "If this options is enabled, then the top 1% of the background will be "
                  "removed before the background subtraction.");

  declareProperty("ProfileFit", false,
                  "If true, integrate by maximizing the Poisson log-likelihood of a Gaussian "
                  "peak plus a flat background rate fit against the raw events, instead of "
                  "counting events inside/outside ellipsoidal boundaries. In this mode the "
                  "peak radii are interpreted as the Gaussian's standard deviations (1-sigma), "
                  "and the background radii are unused.");

  declareProperty("AdjustCenter", false,
                  "Only used if ProfileFit is true. If true, also refine each peak's center by "
                  "a bounded Gauss-Newton correction (capped at one standard deviation from the "
                  "peak's stored Q) as part of the profile fit, instead of keeping it fixed at "
                  "the peak's stored Q. The peak's stored Q is not modified; the correction is "
                  "used only for this integration.");

  declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "The output PeaksWorkspace will be a copy of the input PeaksWorkspace "
                  "with the peaks' integrated intensities.");
}

/**
 * @brief Integrates peak intensities from an event or histogram workspace using QLab ellipsoidal peak shapes.
 *
 * @throws std::runtime_error If the input workspace is not an EventWorkspace or Workspace2D, a peak lacks a QLab
 * ellipsoidal shape, or fewer than three indexed peaks are available.
 */
void IntegratePeaksShapeMD::exec() {
  PeaksWorkspace_sptr input_peak_ws = getProperty("PeaksWorkspace");
  MatrixWorkspace_sptr input_ws = getProperty("InputWorkspace");
  EventWorkspace_sptr eventWS = std::dynamic_pointer_cast<EventWorkspace>(input_ws);

  Workspace2D_sptr histoWS = std::dynamic_pointer_cast<Workspace2D>(input_ws);
  if (!eventWS && !histoWS) {
    throw std::runtime_error("IntegratePeaksShapeMD needs either an "
                             "EventWorkspace or Workspace2D as input.");
  }

  PeaksWorkspace_sptr peak_ws = getProperty("OutputWorkspace");
  if (peak_ws != input_peak_ws) {
    peak_ws = input_peak_ws->clone();
  }

  std::vector<Peak> &peaks = peak_ws->getPeaks();
  size_t n_peaks = peak_ws->getNumberPeaks();

  // Integrate3DEvents uses UBinv to assign each event to its nearest peak
  // (by rounding UBinv*Q to the nearest h,k,l), so it must be derived from
  // the indexed peaks even though integration itself stays in Q-lab.
  std::vector<V3D> peak_q_list;
  std::vector<V3D> hkl_vectors;
  std::vector<std::pair<std::pair<double, double>, V3D>> qList;
  for (size_t i = 0; i < n_peaks; i++) {
    const auto *shape = dynamic_cast<const PeakShapeEllipsoid *>(&peaks[i].getPeakShape());
    if (!shape)
      throw std::runtime_error("Peak " + std::to_string(i) +
                               " does not have an ellipsoidal shape. Integrate the "
                               "PeaksWorkspace first, e.g. with IntegrateEllipsoids, "
                               "so that every peak has a shape to reuse.");
    if (shape->frame() != Kernel::QLab)
      throw std::runtime_error("Peak " + std::to_string(i) + " has an ellipsoidal shape that is not in QLab.");

    V3D hkl(peaks[i].getH(), peaks[i].getK(), peaks[i].getL());
    if (Geometry::IndexingUtils::ValidIndex(hkl, 1.0)) { // tolerance == 1 just checks for (0,0,0)
      peak_q_list.emplace_back(peaks[i].getQLabFrame());
      hkl_vectors.emplace_back(static_cast<double>(boost::math::iround<double>(hkl[0])),
                               static_cast<double>(boost::math::iround<double>(hkl[1])),
                               static_cast<double>(boost::math::iround<double>(hkl[2])));
    }
    qList.emplace_back(std::pair<double, double>(1.0, 1.0), V3D(peaks[i].getQLabFrame()));
  }

  if (peak_q_list.size() < 3)
    throw std::runtime_error("At least three linearly independent indexed peaks are needed.");

  Matrix<double> UB(3, 3, false);
  Geometry::IndexingUtils::Optimize_UB(UB, hkl_vectors, peak_q_list);
  Matrix<double> UBinv(UB);
  UBinv.Invert();
  UBinv *= (1.0 / (2.0 * M_PI));

  const bool useOnePercentBackgroundCorrection = getProperty("UseOnePercentBackgroundCorrection");
  Integrate3DEvents integrator(qList, UBinv, getProperty("RegionRadius"), useOnePercentBackgroundCorrection);

  Progress prog(this, 0.0, 1.0, input_ws->getNumberHistograms());
  if (eventWS) {
    qListFromEventWS(integrator, prog, eventWS);
  } else {
    qListFromHistoWS(integrator, prog, histoWS);
  }

  const bool profileFit = getProperty("ProfileFit");
  const bool adjustCenter = getProperty("AdjustCenter");

  for (size_t i = 0; i < n_peaks; i++) {
    auto &peak = peaks[i];
    const auto *shape = dynamic_cast<const PeakShapeEllipsoid *>(&peak.getPeakShape());

    double inti = 0.0;
    double sigi = 0.0;
    if (profileFit) {
      integrator.integrateUsingShapeProfileFit(*shape, peak.getQLabFrame(), adjustCenter, inti, sigi);
    } else {
      integrator.integrateUsingShape(*shape, peak.getQLabFrame(), inti, sigi);
    }

    peak.setIntensity(inti);
    peak.setSigmaIntensity(sigi);
  }

  // This flag is used by the PeaksWorkspace to evaluate whether it has been
  // integrated.
  peak_ws->mutableRun().addProperty("PeaksIntegrated", 1, true);
  setProperty("OutputWorkspace", peak_ws);
}

void IntegratePeaksShapeMD::qListFromEventWS(Integrate3DEvents &integrator, Progress &prog, EventWorkspace_sptr &wksp) {
  const std::string ELASTIC("Elastic");
  const std::string Q3D("Q3D");
  const std::size_t DIMS(3);

  MDWSDescription m_targWSDescr;
  m_targWSDescr.setMinMax(std::vector<double>(3, -2000.), std::vector<double>(3, 2000.));
  m_targWSDescr.buildFromMatrixWS(wksp, Q3D, ELASTIC);
  m_targWSDescr.setLorentsCorr(false);

  Mantid::API::Algorithm_sptr childAlg = createChildAlgorithm("PreprocessDetectorsToMD", 0., .5);
  childAlg->setProperty("InputWorkspace", wksp);
  childAlg->executeAsChildAlg();

  DataObjects::TableWorkspace_sptr table = childAlg->getProperty("OutputWorkspace");
  if (!table)
    throw(std::runtime_error("Can not retrieve results of \"PreprocessDetectorsToMD\""));

  m_targWSDescr.m_PreprDetTable = table;

  auto numSpectra = static_cast<int>(wksp->getNumberHistograms());
  PARALLEL_FOR_IF(Kernel::threadSafe(*wksp))
  for (int i = 0; i < numSpectra; ++i) {
    PARALLEL_START_INTERRUPT_REGION

    UnitsConversionHelper unitConverter;
    unitConverter.initialize(m_targWSDescr, "Momentum");

    MDTransfQ3D qConverter;
    qConverter.initialize(m_targWSDescr);

    std::vector<double> buffer(DIMS);
    EventList events = wksp->getSpectrum(i);

    events.switchTo(WEIGHTED_NOTIME);
    events.compressEvents(1e-5, &events);

    if (events.empty()) {
      prog.report();
      continue;
    }

    std::vector<Mantid::coord_t> locCoord(DIMS, 0.);
    unitConverter.updateConversion(i);
    qConverter.calcYDepCoordinates(locCoord, i);

    double signal(1.);
    double errorSq(1.);
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
    }
    PARALLEL_CRITICAL(addEvents) { integrator.addEvents(qList, false); }

    prog.report();
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION
}

/**
 * @brief Converts histogram workspace data to Q-space events for integration.
 *
 * Processes positive-intensity histogram bins and adds their Q-space positions,
 * intensities, and squared uncertainties to the integrator.
 *
 * @param integrator Receives the converted Q-space events.
 * @param prog Reports processing progress.
 * @param wksp Histogram workspace containing the data to convert.
 * @throws std::runtime_error If detector preprocessing results cannot be retrieved.
 */
void IntegratePeaksShapeMD::qListFromHistoWS(Integrate3DEvents &integrator, Progress &prog, Workspace2D_sptr &wksp) {
  const std::string ELASTIC("Elastic");
  const std::string Q3D("Q3D");
  const std::size_t DIMS(3);

  MDWSDescription m_targWSDescr;
  m_targWSDescr.setMinMax(std::vector<double>(3, -2000.), std::vector<double>(3, 2000.));
  m_targWSDescr.buildFromMatrixWS(wksp, Q3D, ELASTIC);
  m_targWSDescr.setLorentsCorr(false);

  Mantid::API::Algorithm_sptr childAlg = createChildAlgorithm("PreprocessDetectorsToMD", 0., .5);
  childAlg->setProperty("InputWorkspace", wksp);
  childAlg->executeAsChildAlg();

  DataObjects::TableWorkspace_sptr table = childAlg->getProperty("OutputWorkspace");
  if (!table)
    throw(std::runtime_error("Can not retrieve results of \"PreprocessDetectorsToMD\""));
  m_targWSDescr.m_PreprDetTable = table;

  auto numSpectra = static_cast<int>(wksp->getNumberHistograms());
  PARALLEL_FOR_IF(Kernel::threadSafe(*wksp))
  for (int i = 0; i < numSpectra; ++i) {
    PARALLEL_START_INTERRUPT_REGION

    UnitsConversionHelper unitConverter;
    unitConverter.initialize(m_targWSDescr, "Momentum");

    MDTransfQ3D qConverter;
    qConverter.initialize(m_targWSDescr);

    const auto &xVals = wksp->points(i);
    const auto &yVals = wksp->y(i);
    const auto &eVals = wksp->e(i);

    std::vector<Mantid::coord_t> locCoord(DIMS, 0.);
    unitConverter.updateConversion(i);
    qConverter.calcYDepCoordinates(locCoord, i);

    double signal(1.);
    double errorSq(1.);

    std::vector<std::pair<std::pair<double, double>, V3D>> qList;

    for (size_t j = 0; j < yVals.size(); ++j) {
      const double &yVal = yVals[j];
      const double &esqVal = eVals[j] * eVals[j];
      if (yVal > 0) {
        double val = unitConverter.convertUnits(xVals[j]);
        qConverter.calcMatrixCoord(val, locCoord, signal, errorSq);
        V3D qVec(locCoord[0], locCoord[1], locCoord[2]);

        if (std::isnan(qVec[0]) || std::isnan(qVec[1]) || std::isnan(qVec[2]))
          continue;
        qList.emplace_back(std::pair<double, double>(yVal, esqVal), qVec);
      }
    }
    PARALLEL_CRITICAL(addHisto) { integrator.addEvents(qList, false); }
    prog.report();
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION
}
} // namespace Mantid::MDAlgorithms
