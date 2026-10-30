// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/IntegratePeaksShapeMD.h"

#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeakShapeEllipsoid.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"

#include "MantidMDAlgorithms/EllipsoidTrajectoryNorm.h"
#include "MantidMDAlgorithms/Integrate3DEvents.h"
#include "MantidMDAlgorithms/MDTransfFactory.h"
#include "MantidMDAlgorithms/MDTransfQ3D.h"
#include "MantidMDAlgorithms/UnitsConversionHelper.h"

#include <algorithm>
#include <boost/math/special_functions/round.hpp>
#include <chrono>
#include <cmath>
#include <optional>
#include <string>
#include <vector>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

namespace Mantid::MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(IntegratePeaksShapeMD)

namespace {
// expected signal to noise below which a peak's shape is not refined, and above which it may move as far as the
// bounds allow, with the bounds scaled by its logarithm between them
constexpr double MIN_EXPECTED_SIGNAL_TO_NOISE = 3.0;
constexpr double MAX_EXPECTED_SIGNAL_TO_NOISE = 30.0;

/**
 * @brief The signal to noise of the intensity of a Gaussian peak on a flat background, from the Fisher information
 * of a Poisson fit of its intensity.
 *
 * @param intensity Integrated intensity of the peak.
 * @param background Background, per unit normalization, in the units of the intensity per unit volume.
 * @param exposure Expected counts per unit intensity at the peak.
 * @param sigmas Principal standard deviations of the peak.
 * @return The intensity over its standard deviation; 0 for a peak with no intensity or exposure.
 */
double expectedSignalToNoise(const double intensity, const double background, const double exposure,
                             const PeakEllipsoidExtent &sigmas) {
  if (!(intensity > 0.0) || !(exposure > 0.0))
    return 0.0;
  // integrated over the Mahalanobis radius u, with dV = 4 pi sigma_a sigma_b sigma_c u^2 du
  const double volume = sigmas[0] * sigmas[1] * sigmas[2];
  const double norm = 1.0 / (std::pow(2.0 * M_PI, 1.5) * volume);
  constexpr int n = 240;
  constexpr double du = 6.0 / n;
  double information = 0.0;
  for (int k = 0; k <= n; ++k) {
    const double u = k * du;
    const double g = norm * std::exp(-0.5 * u * u);
    const double weight = (k == 0 || k == n) ? 0.5 : 1.0;
    information += weight * 4.0 * M_PI * volume * u * u * g * g / (intensity * g + background) * du;
  }
  return intensity * std::sqrt(exposure * information);
}

/// A peak's profile fit with its stored shape and center
struct StoredShapeFit {
  double intensity{0.0};
  double sigma{0.0};
  double seconds{0.0};
  V3D center;
  ProfileFitNormalization normalization;
};
} // namespace

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

  declareProperty("AdjustShape", false,
                  "Only used if ProfileFit is true. If true, also refine the shape (covariance) of "
                  "each peak whose intensity is at least AdjustShapeMinSignalToNoise standard "
                  "deviations, as part of the profile fit, with each principal standard deviation "
                  "kept within MaxShapeChange of the peak's stored shape; weaker peaks, and peaks "
                  "that fall below AdjustShapeMinSignalToNoise once the uncertainty of the refined "
                  "shape is included, keep their stored shape. The refined shape is written to the "
                  "output peak. With AdjustCenter too, the center is refined only together with the "
                  "shape, within MaxCenterShift of the stored center, starting from the mode of the "
                  "counts if the peak is more significant there, so peaks that keep their stored "
                  "shape keep their stored center. With normalization, if any input intensities are "
                  "positive (e.g. structure factors from PredictPeaks), how far each peak's shape and "
                  "center may move is scaled by the signal to noise expected of it from them, and peaks "
                  "expected to be too weak keep their stored shapes; the intensities are always fitted "
                  "from the events.");

  auto atLeastOne = std::make_shared<BoundedValidator<double>>();
  atLeastOne->setLower(1.0);
  declareProperty("AdjustShapeMinSignalToNoise", 10.0, mustBePositive,
                  "Only used if AdjustShape is true. The smallest intensity over its standard "
                  "deviation (from a fit with the stored shape) for which a peak's shape is refined.");
  declareProperty("MaxShapeChange", 2.0, atLeastOne,
                  "Only used if AdjustShape is true. The largest factor by which each principal "
                  "standard deviation of a refined shape may differ from the stored shape's.");
  declareProperty("MaxCenterShift", 3.0, mustBePositive,
                  "Only used if AdjustShape and AdjustCenter are true. The largest distance a refined "
                  "center may move from the stored one, in standard deviations of the stored shape "
                  "along each of its principal axes.");

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("SolidAngleWorkspace", "", Direction::Input,
                                                                       PropertyMode::Optional),
                  "An input workspace containing integrated vanadium (a measure of the solid angle), "
                  "as used by MDNorm. If set together with FluxWorkspace, each peak's intensity is "
                  "normalized by flux and solid angle, and the InputWorkspace must have been cropped "
                  "with CropWorkspaceForMDNorm.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("FluxWorkspace", "", Direction::Input,
                                                                       PropertyMode::Optional),
                  "An input workspace containing the integrated flux as a function of momentum, as "
                  "used by MDNorm.");
  setPropertyGroup("SolidAngleWorkspace", "Vanadium normalization");
  setPropertyGroup("FluxWorkspace", "Vanadium normalization");

  declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "The output PeaksWorkspace will be a copy of the input PeaksWorkspace "
                  "with the peaks' integrated intensities.");

  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>("OutputDiagnosticsWorkspace", "",
                                                                       Direction::Output, PropertyMode::Optional),
                  "Only used with SolidAngleWorkspace and FluxWorkspace. A table with one row per peak, "
                  "giving the raw counts and normalization in the peak ellipsoid and background shell, "
                  "the number of detector pixels covering the peak, and flags for peaks only partly "
                  "covered by the momentum range of a pixel or by masked detectors, whether the "
                  "peak's shape was refined by AdjustShape, and the time spent integrating it. With "
                  "ProfileFit, "
                  "CountsPeak is the summed event weight in the fit region, CountsShell the fitted "
                  "background counts, NormPeak the expected counts per unit intensity of the fitted "
                  "peak, NormShell the normalization of the fit region, and the pixel count and flags "
                  "refer to the fit region.");
}

/**
 * @brief Checks that the normalization workspaces are given together, with the logs they require.
 *
 * @return A map from property names to error messages.
 */
std::map<std::string, std::string> IntegratePeaksShapeMD::validateInputs() {
  std::map<std::string, std::string> issues;
  const bool adjustShape = getProperty("AdjustShape");
  const bool profileFit = getProperty("ProfileFit");
  if (adjustShape && !profileFit)
    issues["AdjustShape"] = "AdjustShape requires ProfileFit.";

  MatrixWorkspace_const_sptr solidAngleWS = getProperty("SolidAngleWorkspace");
  MatrixWorkspace_const_sptr fluxWS = getProperty("FluxWorkspace");

  if (!solidAngleWS && !fluxWS) {
    if (!isDefault("OutputDiagnosticsWorkspace"))
      issues["OutputDiagnosticsWorkspace"] =
          "OutputDiagnosticsWorkspace requires SolidAngleWorkspace and FluxWorkspace.";
    return issues;
  }
  if (!solidAngleWS)
    issues["SolidAngleWorkspace"] = "SolidAngleWorkspace must be set together with FluxWorkspace.";
  if (!fluxWS)
    issues["FluxWorkspace"] = "FluxWorkspace must be set together with SolidAngleWorkspace.";

  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  if (inputWS && (!inputWS->run().hasProperty("MDNorm_low") || !inputWS->run().hasProperty("MDNorm_high")))
    issues["InputWorkspace"] = "Missing MDNorm_low or MDNorm_high log. Please use CropWorkspaceForMDNorm on the "
                               "InputWorkspace before integrating with SolidAngleWorkspace and FluxWorkspace.";
  return issues;
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

  // Each event is assigned to the peak whose center is nearest to it, within
  // RegionRadius, without indexing: satellite and unindexed peaks are
  // integrated as any other.
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
    qList.emplace_back(std::pair<double, double>(1.0, 1.0), V3D(peaks[i].getQLabFrame()));
  }

  const double regionRadius = getProperty("RegionRadius");
  const bool useOnePercentBackgroundCorrection = getProperty("UseOnePercentBackgroundCorrection");
  Integrate3DEvents integrator(qList, regionRadius, useOnePercentBackgroundCorrection);

  using Clock = std::chrono::steady_clock;
  auto seconds = [](Clock::time_point start) { return std::chrono::duration<double>(Clock::now() - start).count(); };
  const auto eventsStart = Clock::now();

  Progress prog(this, 0.0, 1.0, input_ws->getNumberHistograms());
  if (eventWS) {
    qListFromEventWS(integrator, prog, eventWS);
  } else {
    qListFromHistoWS(integrator, prog, histoWS);
  }
  const double eventsSeconds = seconds(eventsStart);
  const auto normalizationStart = Clock::now();

  const bool profileFit = getProperty("ProfileFit");
  const bool adjustCenter = getProperty("AdjustCenter");
  const bool adjustShape = getProperty("AdjustShape");

  // flux/solid-angle normalization of each peak's ellipsoid and background
  // shell, from the intersections of the detector trajectories with them
  MatrixWorkspace_sptr solidAngleWS = getProperty("SolidAngleWorkspace");
  MatrixWorkspace_sptr fluxWS = getProperty("FluxWorkspace");
  const bool normalize = solidAngleWS && fluxWS;
  std::optional<EllipsoidTrajectoryNorm> norm;
  std::vector<EllipsoidNormResult> normPeak, normInner, normOuter;
  ITableWorkspace_sptr diagnostics;
  if (normalize) {
    norm = EllipsoidTrajectoryNorm::fromWorkspaces(*input_ws, *solidAngleWS, *fluxWS);
    if (norm->numPixels() == 0)
      throw std::runtime_error("No unmasked detectors of the InputWorkspace were found in both the "
                               "SolidAngleWorkspace and the FluxWorkspace.");
  }
  if (normalize && !profileFit) {
    normPeak.resize(n_peaks);
    normInner.resize(n_peaks);
    normOuter.resize(n_peaks);
    PARALLEL_FOR_NO_WSP_CHECK()
    for (int i = 0; i < static_cast<int>(n_peaks); ++i) {
      const auto &peak = peaks[i];
      const auto *shape = dynamic_cast<const PeakShapeEllipsoid *>(&peak.getPeakShape());
      const auto center = peak.getQLabFrame();
      normPeak[i] = norm->integrate(center, shape->directions(), shape->abcRadii());
      normInner[i] = norm->integrate(center, shape->directions(), shape->abcRadiiBackgroundInner());
      normOuter[i] = norm->integrate(center, shape->directions(), shape->abcRadiiBackgroundOuter());
    }
  }
  if (normalize && !isDefault("OutputDiagnosticsWorkspace")) {
    diagnostics = std::make_shared<TableWorkspace>();
    diagnostics->addColumn("int", "PeakIndex");
    diagnostics->addColumn("double", "CountsPeak");
    diagnostics->addColumn("double", "CountsShell");
    diagnostics->addColumn("double", "NormPeak");
    diagnostics->addColumn("double", "NormShell");
    diagnostics->addColumn("int", "NumPixels");
    diagnostics->addColumn("bool", "ClippedByMomentumRange");
    diagnostics->addColumn("bool", "MaskedDetectors");
    diagnostics->addColumn("bool", "ShapeAdjusted");
    diagnostics->addColumn("double", "FitSeconds");
  }
  const double normalizationSeconds = seconds(normalizationStart);

  // time spent integrating peaks, by outcome
  double fixedSeconds = 0.0, refinedSeconds = 0.0, rejectedSeconds = 0.0;
  int numFixed = 0, numRefined = 0, numRejected = 0;

  // With AdjustShape and normalization, the peaks' input intensities, if any are positive (e.g. the squared
  // structure factors from PredictPeaks), say how strong each peak is expected to be: scaled to the intensities of
  // the strong peaks fitted with their stored shapes, they give the signal to noise expected of each peak, from
  // which how far its shape and center may move is scaled. A peak expected to be too weak keeps its stored shape
  // and center. Only the shapes depend on it; the intensities are always fitted from the events.
  const double minSignalToNoise = getProperty("AdjustShapeMinSignalToNoise");
  const double maxShapeChange = getProperty("MaxShapeChange");
  const double maxCenterShift = getProperty("MaxCenterShift");
  std::vector<double> relaxation;
  std::vector<StoredShapeFit> storedFits;
  if (profileFit && adjustShape && norm) {
    std::vector<double> expected(n_peaks);
    for (size_t i = 0; i < n_peaks; ++i)
      expected[i] = std::max(peaks[i].getIntensity(), 0.0);
    if (std::any_of(expected.begin(), expected.end(), [](double value) { return value > 0.0; })) {
      storedFits.resize(n_peaks);
      for (size_t i = 0; i < n_peaks; ++i) {
        const auto *shape = dynamic_cast<const PeakShapeEllipsoid *>(&peaks[i].getPeakShape());
        auto &fit = storedFits[i];
        const auto fitStart = Clock::now();
        integrator.integrateUsingShapeProfileFit(*shape, peaks[i].getQLabFrame(), false, fit.center, fit.intensity,
                                                 fit.sigma, &(*norm), &fit.normalization, nullptr);
        fit.seconds = seconds(fitStart);
      }
      // the scale of the input intensities, from the strong peaks
      std::vector<double> ratios;
      for (const double threshold : {10.0, 3.0}) {
        ratios.clear();
        for (size_t i = 0; i < n_peaks; ++i)
          if (expected[i] > 0.0 && storedFits[i].sigma > 0.0 &&
              storedFits[i].intensity > threshold * storedFits[i].sigma)
            ratios.emplace_back(storedFits[i].intensity / expected[i]);
        if (ratios.size() >= 3)
          break;
      }
      if (ratios.size() >= 3) {
        const auto middle = ratios.begin() + ratios.size() / 2;
        std::nth_element(ratios.begin(), middle, ratios.end());
        const double scale = *middle;
        relaxation.resize(n_peaks);
        int numExpectedWeak = 0;
        for (size_t i = 0; i < n_peaks; ++i) {
          const auto *shape = dynamic_cast<const PeakShapeEllipsoid *>(&peaks[i].getPeakShape());
          const auto &normalization = storedFits[i].normalization;
          const double background =
              normalization.regionNorm > 0.0 ? normalization.backgroundCounts / normalization.regionNorm : 0.0;
          const double signalToNoise =
              expectedSignalToNoise(scale * expected[i], background, normalization.peakNorm, shape->abcRadii());
          const double w = signalToNoise > 0.0
                               ? std::log(signalToNoise / MIN_EXPECTED_SIGNAL_TO_NOISE) /
                                     std::log(MAX_EXPECTED_SIGNAL_TO_NOISE / MIN_EXPECTED_SIGNAL_TO_NOISE)
                               : 0.0;
          relaxation[i] = std::clamp(w, 0.0, 1.0);
          if (!(relaxation[i] > 0.0))
            ++numExpectedWeak;
        }
        g_log.notice() << "Scaled the input intensities by " << scale << " from " << ratios.size() << " strong peaks; "
                       << numExpectedWeak << " peaks are expected too weak to refine\n";
      } else {
        g_log.warning() << "Too few strong peaks to scale the input intensities; they are not used\n";
      }
    }
  }

  for (size_t i = 0; i < n_peaks; i++) {
    auto &peak = peaks[i];
    const auto *shape = dynamic_cast<const PeakShapeEllipsoid *>(&peak.getPeakShape());

    double inti = 0.0;
    double sigi = 0.0;
    if (profileFit) {
      V3D center;
      ProfileFitNormalization fitNormalization;
      ProfileFitShape shapeFit;
      shapeFit.minSignalToNoise = minSignalToNoise;
      shapeFit.maxChange = maxShapeChange;
      shapeFit.maxCenterShift = maxCenterShift;
      double fitSeconds = 0.0;
      if (!relaxation.empty() && !(relaxation[i] > 0.0)) {
        // expected too weak to refine: the fit with its stored shape and center
        const auto &fit = storedFits[i];
        center = fit.center;
        inti = fit.intensity;
        sigi = fit.sigma;
        fitNormalization = fit.normalization;
        fitSeconds = fit.seconds;
      } else {
        if (!relaxation.empty()) {
          shapeFit.maxChange = 1.0 + relaxation[i] * (maxShapeChange - 1.0);
          shapeFit.maxCenterShift = relaxation[i] * maxCenterShift;
        }
        const auto fitStart = Clock::now();
        integrator.integrateUsingShapeProfileFit(*shape, peak.getQLabFrame(), adjustCenter, center, inti, sigi,
                                                 norm ? &(*norm) : nullptr, &fitNormalization,
                                                 adjustShape ? &shapeFit : nullptr);
        fitSeconds = seconds(fitStart) + (storedFits.empty() ? 0.0 : storedFits[i].seconds);
      }
      const bool shapeAdjusted = adjustShape && shapeFit.adjusted;
      if (shapeAdjusted) {
        refinedSeconds += fitSeconds;
        ++numRefined;
      } else if (adjustShape && shapeFit.attempted) {
        rejectedSeconds += fitSeconds;
        ++numRejected;
      } else {
        fixedSeconds += fitSeconds;
        ++numFixed;
      }
      if (diagnostics) {
        TableRow row = diagnostics->appendRow();
        row << static_cast<int>(i) << fitNormalization.counts << fitNormalization.backgroundCounts
            << fitNormalization.peakNorm << fitNormalization.regionNorm
            << static_cast<int>(fitNormalization.coverage.numPixels) << fitNormalization.coverage.clipped
            << fitNormalization.coverage.maskedIntersects << shapeAdjusted << fitSeconds;
      }
      // Persist a center correction found by AdjustCenter as the output
      // shape's translation -- peak.getQLabFrame() itself is left alone,
      // since it's tied to a specific detector pixel/TOF and isn't free to
      // move without checking that still corresponds to a valid trajectory.
      // A refined shape replaces the stored directions and radii, with the
      // background radii scaled with them.
      if (shapeAdjusted) {
        const auto &radii = shape->abcRadii();
        const auto &inner = shape->abcRadiiBackgroundInner();
        const auto &outer = shape->abcRadiiBackgroundOuter();
        double innerFactor = 0.0, outerFactor = 0.0;
        for (size_t k = 0; k < 3; ++k) {
          innerFactor += inner[k] / radii[k] / 3.0;
          outerFactor += outer[k] / radii[k] / 3.0;
        }
        PeakEllipsoidExtent newInner, newOuter;
        for (size_t k = 0; k < 3; ++k) {
          newInner[k] = shapeFit.sigmas[k] * innerFactor;
          newOuter[k] = shapeFit.sigmas[k] * outerFactor;
        }
        peak.setPeakShape(new PeakShapeEllipsoid(shapeFit.directions, shapeFit.sigmas, newInner, newOuter,
                                                 shape->frame(), shape->algorithmName(), shape->algorithmVersion(),
                                                 center));
      } else if (adjustCenter && center != shape->translation()) {
        peak.setPeakShape(new PeakShapeEllipsoid(
            shape->directions(), shape->abcRadii(), shape->abcRadiiBackgroundInner(), shape->abcRadiiBackgroundOuter(),
            shape->frame(), shape->algorithmName(), shape->algorithmVersion(), center));
      }
    } else if (normalize) {
      // I = V_peak * (N_peak / Norm_peak - N_shell / Norm_shell), the
      // background-subtracted mean normalized signal over the peak ellipsoid
      // times its volume, with the normalization treated as exact
      const auto fitStart = Clock::now();
      const auto counts = integrator.countUsingShape(*shape, peak.getQLabFrame());
      const double peakNorm = normPeak[i].norm;
      const double shellNorm = normOuter[i].norm - normInner[i].norm;
      const auto &radii = shape->abcRadii();
      const double peakVolume = 4.0 / 3.0 * M_PI * radii[0] * radii[1] * radii[2];
      if (peakNorm > 0.0) {
        double signal = counts.peak / peakNorm;
        double variance = counts.peakErrorSq / (peakNorm * peakNorm);
        if (shellNorm > 0.0) {
          signal -= counts.shell / shellNorm;
          variance += counts.shellErrorSq / (shellNorm * shellNorm);
        }
        inti = peakVolume * signal;
        sigi = peakVolume * std::sqrt(variance);
      }
      const double fitSeconds = seconds(fitStart);
      fixedSeconds += fitSeconds;
      ++numFixed;
      if (diagnostics) {
        TableRow row = diagnostics->appendRow();
        row << static_cast<int>(i) << counts.peak << counts.shell << peakNorm << shellNorm
            << static_cast<int>(normPeak[i].numPixels) << (normPeak[i].clipped || normOuter[i].clipped)
            << normOuter[i].maskedIntersects << false << fitSeconds;
      }
    } else {
      integrator.integrateUsingShape(*shape, peak.getQLabFrame(), inti, sigi);
    }

    peak.setIntensity(inti);
    peak.setSigmaIntensity(sigi);
  }

  g_log.notice() << "Gathered events in " << eventsSeconds << " s, set up normalization in " << normalizationSeconds
                 << " s, integrated " << numFixed << " peaks with their stored shapes in " << fixedSeconds << " s";
  if (adjustShape)
    g_log.notice() << ", refined the shapes of " << numRefined << " peaks in " << refinedSeconds
                   << " s and rejected the refinements of " << numRejected << " peaks in " << rejectedSeconds << " s";
  g_log.notice() << "\n";

  // This flag is used by the PeaksWorkspace to evaluate whether it has been
  // integrated.
  peak_ws->mutableRun().addProperty("PeaksIntegrated", 1, true);
  setProperty("OutputWorkspace", peak_ws);
  if (diagnostics)
    setProperty("OutputDiagnosticsWorkspace", diagnostics);
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

  // the preprocessed detector table has one row per detector (monitors and
  // spectra without detectors are left out), mapped to its spectrum by detIDMap
  const auto numDetectors = static_cast<int>(table->getLogs()->getPropertyValueAsType<uint32_t>("ActualDetectorsNum"));
  const auto &detIDMap = table->getColVector<size_t>("detIDMap");
  PARALLEL_FOR_IF(Kernel::threadSafe(*wksp))
  for (int i = 0; i < numDetectors; ++i) {
    PARALLEL_START_INTERRUPT_REGION

    UnitsConversionHelper unitConverter;
    unitConverter.initialize(m_targWSDescr, "Momentum");

    MDTransfQ3D qConverter;
    qConverter.initialize(m_targWSDescr);

    // skip masked detectors, as ConvertToMD does
    std::vector<Mantid::coord_t> locCoord(DIMS, 0.);
    if (!qConverter.calcYDepCoordinates(locCoord, i)) {
      prog.report();
      continue;
    }

    std::vector<double> buffer(DIMS);
    EventList events = wksp->getSpectrum(detIDMap[i]);

    events.switchTo(WEIGHTED_NOTIME);
    events.compressEvents(1e-5, &events);

    if (events.empty()) {
      prog.report();
      continue;
    }

    unitConverter.updateConversion(i);

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

  // the preprocessed detector table has one row per detector (monitors and
  // spectra without detectors are left out), mapped to its spectrum by detIDMap
  const auto numDetectors = static_cast<int>(table->getLogs()->getPropertyValueAsType<uint32_t>("ActualDetectorsNum"));
  const auto &detIDMap = table->getColVector<size_t>("detIDMap");
  PARALLEL_FOR_IF(Kernel::threadSafe(*wksp))
  for (int i = 0; i < numDetectors; ++i) {
    PARALLEL_START_INTERRUPT_REGION

    UnitsConversionHelper unitConverter;
    unitConverter.initialize(m_targWSDescr, "Momentum");

    MDTransfQ3D qConverter;
    qConverter.initialize(m_targWSDescr);

    // skip masked detectors, as ConvertToMD does
    std::vector<Mantid::coord_t> locCoord(DIMS, 0.);
    if (!qConverter.calcYDepCoordinates(locCoord, i)) {
      prog.report();
      continue;
    }

    const auto spectrum = detIDMap[i];
    const auto &xVals = wksp->points(spectrum);
    const auto &yVals = wksp->y(spectrum);
    const auto &eVals = wksp->e(spectrum);

    unitConverter.updateConversion(i);

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
