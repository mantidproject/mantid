// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/TOFSANSResolutionByPixel.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidAlgorithms/GravitySANSHelper.h"
#include "MantidAlgorithms/SANSCollimationLengthEstimator.h"
#include "MantidAlgorithms/TOFSANSResolutionByPixelCalculator.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ITimeSeriesProperty.h"
#include "MantidKernel/Interpolation.h"
#include "MantidKernel/UnitFactory.h"

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
  declareProperty(std::make_unique<WorkspaceProperty<>>(
                      "InputWorkspace", "", Direction::Input,
                      boost::make_shared<WorkspaceUnitValidator>("Wavelength")),
                  "Name the workspace to calculate the resolution for, for "
                  "each pixel and wavelength");
  declareProperty(
      std::make_unique<WorkspaceProperty<Workspace>>("OutputWorkspace", "",
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
  declareProperty(std::make_unique<WorkspaceProperty<>>(
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

  const auto &xInterpolate = sigmaModeratorVSwavelength->points(0);
  const auto &yInterpolate = sigmaModeratorVSwavelength->y(0);

  // prefer the input to be a pointworkspace and create interpolation function
  if (sigmaModeratorVSwavelength->isHistogramData()) {
    g_log.notice() << "mid-points of SigmaModerator histogram bins will be "
                      "used for interpolation.";
  }

  for (size_t i = 0; i < xInterpolate.size(); ++i) {
    lookUpTable.addPoint(xInterpolate[i], yInterpolate[i]);
  }

  // Calculate the L1 distance
  const V3D samplePos = inWS->getInstrument()->getSample()->getPos();
  const V3D sourcePos = inWS->getInstrument()->getSource()->getPos();
  const V3D SSD = samplePos - sourcePos;
  const double L1 = SSD.norm();

  // Get the collimation length
  double LCollim = getProperty("CollimationLength");

  if (LCollim == 0.0) {
    auto collimationLengthEstimator = SANSCollimationLengthEstimator();
    LCollim = collimationLengthEstimator.provideCollimationLength(inWS);
    g_log.information() << "No collimation length was specified. A default "
                           "collimation length was estimated to be "
                        << LCollim << '\n';
  } else {
    g_log.information() << "The collimation length is  " << LCollim << '\n';
  }

  const auto numberOfSpectra = static_cast<int>(inWS->getNumberHistograms());
  Progress progress(this, 0.0, 1.0, numberOfSpectra);

  const auto &spectrumInfo = inWS->spectrumInfo();
  for (int i = 0; i < numberOfSpectra; i++) {
    IDetector_const_sptr det;
    if (!spectrumInfo.hasDetectors(i)) {
      g_log.information() << "Workspace index " << i
                          << " has no detector assigned to it - discarding\n";
      continue;
    }
    // If no detector found or if it's masked or a monitor, skip onto the next
    // spectrum
    if (spectrumInfo.isMonitor(i) || spectrumInfo.isMasked(i))
      continue;

    const double L2 = spectrumInfo.l2(i);
    TOFSANSResolutionByPixelCalculator calculator;
    const double waveLengthIndependentFactor =
        calculator.getWavelengthIndependentFactor(R1, R2, deltaR, LCollim, L2);

    // Multiplicative factor to go from lambda to Q
    // Don't get fooled by the function name...
    const double theta = spectrumInfo.twoTheta(i);
    double sinTheta = sin(0.5 * theta);
    double factor = 4.0 * M_PI * sinTheta;

    const auto &xIn = inWS->x(i);
    const size_t xLength = xIn.size();

    // Gravity correction
    std::unique_ptr<GravitySANSHelper> grav;
    if (doGravity) {
      grav = std::make_unique<GravitySANSHelper>(spectrumInfo, i,
                                                 getProperty("ExtraLength"));
    }

    // Get handles on the outputWorkspace
    auto &yOut = outWS->mutableY(i);
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
      auto sigmaQ = calculator.getSigmaQValue(lookUpTable.value(wl),
                                              waveLengthIndependentFactor, q,
                                              wl, sigmaSpreadFromBin, L1, L2);

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
