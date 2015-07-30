//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/TOFSANSResolutionByPixel.h"
#include "MantidAlgorithms/SANSCollimationLengthEstimator.h"
#include "MantidAlgorithms/GravitySANSHelper.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/Interpolation.h"
#include "MantidKernel/ITimeSeriesProperty.h"

#include "boost/math/special_functions/fpclassify.hpp"
#include "boost/lexical_cast.hpp"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(TOFSANSResolutionByPixel)

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

TOFSANSResolutionByPixel::TOFSANSResolutionByPixel()
    : API::Algorithm(), m_wl_resolution(0.) {}

void TOFSANSResolutionByPixel::init() {
  declareProperty(new WorkspaceProperty<>(
                      "Workspace", "", Direction::InOut,
                      boost::make_shared<WorkspaceUnitValidator>("Wavelength")),
                  "Name the workspace to calculate the resolution for, for "
                  "each pixel and wavelenght");

  auto positiveDouble = boost::make_shared<BoundedValidator<double>>();
  positiveDouble->setLower(0);
  declareProperty("DeltaR", 0.0, positiveDouble, "Pixel size (mm).");
  declareProperty("SampleApertureRadius", 0.0, positiveDouble,
                  "Sample aperture radius (mm).");
  declareProperty("SourceApertureRadius", 0.0, positiveDouble,
                  "Source aperture radius (mm).");
  declareProperty(new WorkspaceProperty<>(
                      "SigmaModerator", "", Direction::Input,
                      boost::make_shared<WorkspaceUnitValidator>("Wavelength")),
                  "Sigma moderator spread in units of microsec as a function "
                  "of wavelength.");
  declareProperty("CollimationLength", 0.0, positiveDouble, "Collimation length (m)");
  declareProperty("AccountForGravity", false,
                  "Whether to correct for the effects of gravity");
  declareProperty(
      "ExtraLength", 0.0, positiveDouble,
      "Additional length for gravity correction.");
}

/*
 * Double Boltzmann fit to the TOF resolution as a function of wavelength
 */
double TOFSANSResolutionByPixel::getTOFResolution(double wl) {
  UNUSED_ARG(wl);
  return m_wl_resolution;
}

void TOFSANSResolutionByPixel::exec() {
  MatrixWorkspace_sptr inOutWS = getProperty("Workspace");
  double deltaR = getProperty("DeltaR");
  double R1 = getProperty("SourceApertureRadius");
  double R2 = getProperty("SampleApertureRadius");
  const bool doGravity = getProperty("AccountForGravity");

  // Convert to meters
  deltaR /= 1000.0;
  R1 /= 1000.0;
  R2 /= 1000.0;

  const MatrixWorkspace_sptr sigmaModeratorVSwavelength =
      getProperty("SigmaModerator");

  // create interpolation table from sigmaModeratorVSwavelength
  Kernel::Interpolation lookUpTable;

  const MantidVec xInterpolate = sigmaModeratorVSwavelength->readX(0);
  const MantidVec yInterpolate = sigmaModeratorVSwavelength->readY(0);

  // prefer the input to be a pointworkspace and create interpolation function
  if (sigmaModeratorVSwavelength->isHistogramData()) {
    g_log.notice() << "mid-points of SigmaModerator histogram bins will be "
                      "used for interpolation.";

    for (size_t i = 0; i < xInterpolate.size() - 1; ++i) {
      const double midpoint = (xInterpolate[i + 1] + xInterpolate[i]) / 2.0;
      lookUpTable.addPoint(midpoint, yInterpolate[i]);
    }
  } else {
    for (size_t i = 0; i < xInterpolate.size(); ++i) {
      lookUpTable.addPoint(xInterpolate[i], yInterpolate[i]);
    }
  }

  // Get the collimation length
  double LCollim = getProperty("CollimationLength");
  const V3D samplePos = inOutWS->getInstrument()->getSample()->getPos();

  if (LCollim == 0.0) {
    auto collimationLengthEstimator = SANSCollimationLengthEstimator();
    LCollim = collimationLengthEstimator.provideCollimationLength(inOutWS);
    g_log.information() << "No collimation length was specified. A default collimation length was estimated to be " << LCollim << std::endl;
  } else {
    g_log.information() << "The collimation length is  " << LCollim << std::endl;
  }

  const int numberOfSpectra = static_cast<int>(inOutWS->getNumberHistograms());
  Progress progress(this, 0.0, 1.0, numberOfSpectra);

  for (int i = 0; i < numberOfSpectra; i++) {
    IDetector_const_sptr det;
    try {
      det = inOutWS->getDetector(i);
    } catch (Exception::NotFoundError &) {
      g_log.information() << "Spectrum index " << i
                          << " has no detector assigned to it - discarding"
                          << std::endl;
    }
    // If no detector found or if it's masked or a monitor, skip onto the next
    // spectrum
    if (!det || det->isMonitor() || det->isMasked())
      continue;

    // Get the flight path from the sample to the detector pixel
    const V3D scatteredFlightPathV3D = det->getPos() - samplePos;

    const double L2 = scatteredFlightPathV3D.norm();
    const double Lsum = LCollim + L2;

    // calculate part that is wavelenght independent
    const double dTheta2 = (4.0 * M_PI * M_PI / 12.0) *
                           (3.0 * R1 * R1 / (LCollim * LCollim) +
                            3.0 * R2 * R2 * Lsum * Lsum / (LCollim * LCollim * L2 * L2) +
                            (deltaR * deltaR) / (L2 * L2));

    // Multiplicative factor to go from lambda to Q
    // Don't get fooled by the function name...
    const double theta = inOutWS->detectorTwoTheta(det);
    double sinTheta = sin(theta / 2.0);
    double factor = 4.0 * M_PI * sinTheta;

    const MantidVec &xIn = inOutWS->readX(i);
    MantidVec &yIn = inOutWS->dataY(i);
    const size_t xLength = xIn.size();

    // for each wavelenght bin of each pixel calculate a q-resolution
    for (size_t j = 0; j < xLength - 1; j++) {
      // use the midpoint of each bin
      const double wl = (xIn[j + 1] + xIn[j]) / 2.0;
      // Calculate q. Alternatively q could be calculated using ConvertUnit
      // If we include a gravity correction we need to adjust sinTheta
      // for each wavelength (in Angstrom)
      if (doGravity) {
        GravitySANSHelper grav(inOutWS, det, getProperty("ExtraLength"));
        double sinThetaGrav = grav.calcSinTheta(wl);
        factor = 4.0 * M_PI * sinThetaGrav;
      }
      const double q = factor / wl;

      // wavelenght spread from bin assumed to be
      const double sigmaSpreadFromBin = xIn[j + 1] - xIn[j];

      // wavelenght spread from moderatorm, converted from microseconds to
      // wavelengths
      const double sigmaModerator =
          lookUpTable.value(wl) * 3.9560 / (1000.0 * Lsum);

      // calculate wavelenght resolution from moderator and histogram time bin
      const double sigmaLambda =
          std::sqrt(sigmaSpreadFromBin * sigmaSpreadFromBin / 12.0 +
                    sigmaModerator * sigmaModerator);

      // calculate sigmaQ for a given lambda and pixel
      const double sigmaOverLambdaTimesQ = q * sigmaLambda / wl;
      const double sigmaQ = std::sqrt(
          dTheta2 / (wl * wl) + sigmaOverLambdaTimesQ * sigmaOverLambdaTimesQ);

      // update inout workspace with this sigmaQ
      yIn[j] = sigmaQ;
    }

    progress.report("Computing Q resolution");
  }
}


} // namespace Algorithms
} // namespace Mantid
