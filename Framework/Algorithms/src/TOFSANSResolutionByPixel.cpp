//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/TOFSANSResolutionByPixel.h"
#include "MantidAlgorithms/TOFSANSResolutionByPixelCalculator.h"
#include "MantidAlgorithms/SANSCollimationLengthEstimator.h"
#include "MantidAlgorithms/GravitySANSHelper.h"
#include "MantidAlgorithms/CloneWorkspace.h"
#include "MantidAlgorithms/RebinToWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/Interpolation.h"
#include "MantidKernel/ITimeSeriesProperty.h"
#include "MantidKernel/UnitFactory.h"

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
                      "InputWorkspace", "", Direction::Input,
                      boost::make_shared<WorkspaceUnitValidator>("Wavelength")),
                  "Name the workspace to calculate the resolution for, for "
                  "each pixel and wavelength");
  declareProperty(
      new WorkspaceProperty<Workspace>("OutputWorkspace", "",
                                       Direction::Output),
      "Name of the newly created workspace which contains the Q resolution.");
  auto positiveDouble = boost::make_shared<BoundedValidator<double>>();
  positiveDouble->setLower(0);
  declareProperty("DeltaR", 0.0, positiveDouble,
                  "Virtual ring width on the detector (mm).");
  declareProperty("SampleApertureRadius", 0.0, positiveDouble,
                  "Sample aperture radius, R2 (mm).");
  declareProperty("SourceApertureRadius", 0.0, positiveDouble,
                  "Source aperture radius, R1 (mm).");
  declareProperty(new WorkspaceProperty<>(
                      "SigmaModerator", "", Direction::Input,
                      boost::make_shared<WorkspaceUnitValidator>("Wavelength")),
                  "Moderator time spread (microseconds) as a"
                  "function of wavelength (Angstroms).");
  declareProperty("CollimationLength", 0.0, positiveDouble,
                  "Collimation length (m)");
  declareProperty("AccountForGravity", false,
                  "Whether to correct for the effects of gravity");
  declareProperty("ExtraLength", 0.0, positiveDouble,
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
  MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");
  double deltaR = getProperty("DeltaR");
  double R1 = getProperty("SourceApertureRadius");
  double R2 = getProperty("SampleApertureRadius");
  const bool doGravity = getProperty("AccountForGravity");

  // Check the input
  checkInput(inWS);

  // Setup outputworkspace
  auto outWS = setupOutputWorkspace(inWS);

  // Convert to meters
  deltaR /= 1000.0;
  R1 /= 1000.0;
  R2 /= 1000.0;

  // The moderator workspace needs to match the data workspace
  // in terms of wavelength binning
  const MatrixWorkspace_sptr sigmaModeratorVSwavelength =
      getModeratorWorkspace(inWS);

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

  if (LCollim == 0.0) {
    auto collimationLengthEstimator = SANSCollimationLengthEstimator();
    LCollim = collimationLengthEstimator.provideCollimationLength(inWS);
    g_log.information() << "No collimation length was specified. A default "
                           "collimation length was estimated to be " << LCollim
                        << std::endl;
  } else {
    g_log.information() << "The collimation length is  " << LCollim
                        << std::endl;
  }

  const int numberOfSpectra = static_cast<int>(inWS->getNumberHistograms());
  Progress progress(this, 0.0, 1.0, numberOfSpectra);

  for (int i = 0; i < numberOfSpectra; i++) {
    IDetector_const_sptr det;
    try {
      det = inWS->getDetector(i);
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
    const V3D samplePos = inWS->getInstrument()->getSample()->getPos();
    const V3D scatteredFlightPathV3D = det->getPos() - samplePos;
    const double L2 = scatteredFlightPathV3D.norm();

    TOFSANSResolutionByPixelCalculator calculator;
    const double waveLengthIndependentFactor =
        calculator.getWavelengthIndependentFactor(R1, R2, deltaR, LCollim, L2);

    // Multiplicative factor to go from lambda to Q
    // Don't get fooled by the function name...
    const double theta = inWS->detectorTwoTheta(det);
    double sinTheta = sin(theta / 2.0);
    double factor = 4.0 * M_PI * sinTheta;

    const MantidVec &xIn = inWS->readX(i);
    const size_t xLength = xIn.size();

    // Gravity correction
    boost::shared_ptr<GravitySANSHelper> grav;
    if (doGravity) {
      grav = boost::make_shared<GravitySANSHelper>(inWS, det,
                                                   getProperty("ExtraLength"));
    }

    // Get handles on the outputWorkspace
    MantidVec &yOut = outWS->dataY(i);
    // for each wavelenght bin of each pixel calculate a q-resolution
    for (size_t j = 0; j < xLength - 1; j++) {
      // use the midpoint of each bin
      const double wl = (xIn[j + 1] + xIn[j]) / 2.0;
      // Calculate q. Alternatively q could be calculated using ConvertUnit
      // If we include a gravity correction we need to adjust sinTheta
      // for each wavelength (in Angstrom)
      if (doGravity) {
        double sinThetaGrav = grav->calcSinTheta(wl);
        factor = 4.0 * M_PI * sinThetaGrav;
      }
      const double q = factor / wl;

      // wavelenght spread from bin assumed to be
      const double sigmaSpreadFromBin = xIn[j + 1] - xIn[j];

      // Get the uncertainty in Q
      auto sigmaQ = calculator.getSigmaQValue(
          lookUpTable.value(wl), waveLengthIndependentFactor, q, wl,
          sigmaSpreadFromBin, LCollim, L2);

      // Insert the Q value and the Q resolution into the outputworkspace
      yOut[j] = sigmaQ;
    }
    progress.report("Computing Q resolution");
  }

  // Set the y axis label
  outWS->setYUnitLabel("QResolution");

  setProperty("OutputWorkspace", outWS);
}

/**
 * Setup output workspace
 * @param inputWorkspace: the input workspace
 * @returns a copy of the input workspace
 */
MatrixWorkspace_sptr TOFSANSResolutionByPixel::setupOutputWorkspace(
    MatrixWorkspace_sptr inputWorkspace) {
  IAlgorithm_sptr duplicate = createChildAlgorithm("CloneWorkspace");
  duplicate->initialize();
  duplicate->setProperty<Workspace_sptr>("InputWorkspace", inputWorkspace);
  duplicate->execute();
  Workspace_sptr temp = duplicate->getProperty("OutputWorkspace");
  return boost::dynamic_pointer_cast<MatrixWorkspace>(temp);
}

/**
 * Get the moderator workspace
 * @param inputWorkspace: the input workspace
 * @returns the moderator workspace wiht the correct wavelength binning
 */
MatrixWorkspace_sptr TOFSANSResolutionByPixel::getModeratorWorkspace(
    Mantid::API::MatrixWorkspace_sptr inputWorkspace) {

  MatrixWorkspace_sptr sigmaModerator = getProperty("SigmaModerator");
  IAlgorithm_sptr rebinned = createChildAlgorithm("RebinToWorkspace");
  rebinned->initialize();
  rebinned->setProperty("WorkspaceToRebin", sigmaModerator);
  rebinned->setProperty("WorkspaceToMatch", inputWorkspace);
  rebinned->setPropertyValue("OutputWorkspace", "SigmaModeratorRebinned");
  rebinned->execute();
  MatrixWorkspace_sptr outWS = rebinned->getProperty("OutputWorkspace");
  return outWS;
}

/**
 * Check the input workspace
 * @param inWS: the input workspace
 */
void TOFSANSResolutionByPixel::checkInput(
    Mantid::API::MatrixWorkspace_sptr inWS) {
  // Make sure that input workspace has an instrument as we rely heavily on
  // thisa
  auto inst = inWS->getInstrument();
  if (inst->getName().empty()) {
    throw std::invalid_argument("TOFSANSResolutionByPixel: The input workspace "
                                "does not contain an instrument");
  }
}

} // namespace Algorithms
} // namespace Mantid
