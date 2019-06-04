// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/TOFSANSResolution.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/VectorHelper.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(TOFSANSResolution)

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

TOFSANSResolution::TOFSANSResolution()
    : API::Algorithm(), m_wl_resolution(0.) {}

void TOFSANSResolution::init() {
  declareProperty(
      std::make_unique<WorkspaceProperty<>>(
          "InputWorkspace", "", Direction::InOut,
          boost::make_shared<WorkspaceUnitValidator>("MomentumTransfer")),
      "Name the workspace to calculate the resolution for");

  auto wsValidator = boost::make_shared<WorkspaceUnitValidator>("Wavelength");
  declareProperty(std::make_unique<WorkspaceProperty<>>(
                      "ReducedWorkspace", "", Direction::Input, wsValidator),
                  "I(Q) workspace");
  declareProperty(std::make_unique<ArrayProperty<double>>(
      "OutputBinning", boost::make_shared<RebinParamsValidator>()));

  declareProperty("MinWavelength", EMPTY_DBL(), "Minimum wavelength to use.");
  declareProperty("MaxWavelength", EMPTY_DBL(), "Maximum wavelength to use.");

  auto positiveDouble = boost::make_shared<BoundedValidator<double>>();
  positiveDouble->setLower(0);
  declareProperty("PixelSizeX", 5.15, positiveDouble,
                  "Pixel size in the X direction (mm).");
  declareProperty("PixelSizeY", 5.15, positiveDouble,
                  "Pixel size in the Y direction (mm).");
  declareProperty("SampleApertureRadius", 5.0, positiveDouble,
                  "Sample aperture radius (mm).");
  declareProperty("SourceApertureRadius", 10.0, positiveDouble,
                  "Source aperture radius (mm).");
  declareProperty("DeltaT", 250.0, positiveDouble, "TOF spread (microsec).");
}

/*
 * Double Boltzmann fit to the TOF resolution as a function of wavelength
 */
double TOFSANSResolution::getTOFResolution(double wl) {
  UNUSED_ARG(wl);
  return m_wl_resolution;
}

/*
 * Return the effective pixel size in X, in meters
 */
double TOFSANSResolution::getEffectiveXPixelSize() {
  double pixel_size_x = getProperty("PixelSizeX");
  return pixel_size_x / 1000.0;
}

/*
 * Return the effective pixel size in Y, in meters
 */
double TOFSANSResolution::getEffectiveYPixelSize() {
  double pixel_size_y = getProperty("PixelSizeY");
  return pixel_size_y / 1000.0;
}

void TOFSANSResolution::exec() {
  MatrixWorkspace_sptr iqWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr reducedWS = getProperty("ReducedWorkspace");
  EventWorkspace_sptr reducedEventWS =
      boost::dynamic_pointer_cast<EventWorkspace>(reducedWS);
  const double min_wl = getProperty("MinWavelength");
  const double max_wl = getProperty("MaxWavelength");
  double pixel_size_x = getEffectiveXPixelSize();
  double pixel_size_y = getEffectiveYPixelSize();
  double R1 = getProperty("SourceApertureRadius");
  double R2 = getProperty("SampleApertureRadius");
  // Convert to meters
  R1 /= 1000.0;
  R2 /= 1000.0;
  m_wl_resolution = getProperty("DeltaT");

  // Calculate the output binning
  const std::vector<double> binParams = getProperty("OutputBinning");

  // Count histogram for normalization
  const int xLength = static_cast<int>(iqWS->x(0).size());
  std::vector<double> XNorm(xLength - 1, 0.0);

  // Create workspaces with each component of the resolution for debugging
  // purposes
  MatrixWorkspace_sptr thetaWS = WorkspaceFactory::Instance().create(iqWS);
  declareProperty(
      std::make_unique<WorkspaceProperty<>>("ThetaError", "", Direction::Output));
  setPropertyValue("ThetaError", "__" + iqWS->getName() + "_theta_error");
  setProperty("ThetaError", thetaWS);
  thetaWS->setSharedX(0, iqWS->sharedX(0));
  auto &ThetaY = thetaWS->mutableY(0);

  MatrixWorkspace_sptr tofWS = WorkspaceFactory::Instance().create(iqWS);
  declareProperty(
      std::make_unique<WorkspaceProperty<>>("TOFError", "", Direction::Output));
  setPropertyValue("TOFError", "__" + iqWS->getName() + "_tof_error");
  setProperty("TOFError", tofWS);
  tofWS->setSharedX(0, iqWS->sharedX(0));
  auto &TOFY = tofWS->mutableY(0);

  // Initialize Dq
  HistogramData::HistogramDx DxOut(xLength - 1, 0.0);

  const int numberOfSpectra =
      static_cast<int>(reducedWS->getNumberHistograms());
  Progress progress(this, 0.0, 1.0, numberOfSpectra);

  const auto &spectrumInfo = reducedWS->spectrumInfo();
  const double L1 = spectrumInfo.l1();

  PARALLEL_FOR_IF(Kernel::threadSafe(*reducedWS, *iqWS))
  for (int i = 0; i < numberOfSpectra; i++) {
    PARALLEL_START_INTERUPT_REGION
    if (!spectrumInfo.hasDetectors(i)) {
      g_log.warning() << "Workspace index " << i
                      << " has no detector assigned to it - discarding\n";
      continue;
    }
    // Skip if we have a monitor or if the detector is masked.
    if (spectrumInfo.isMonitor(i) || spectrumInfo.isMasked(i))
      continue;

    const double L2 = spectrumInfo.l2(i);

    // Multiplicative factor to go from lambda to Q
    // Don't get fooled by the function name...
    const double theta = spectrumInfo.twoTheta(i);
    const double factor = 4.0 * M_PI * sin(0.5 * theta);

    const auto &XIn = reducedWS->x(i);
    const auto &YIn = reducedWS->y(i);
    const int wlLength = static_cast<int>(XIn.size());

    std::vector<double> _dx(xLength - 1, 0.0);
    std::vector<double> _norm(xLength - 1, 0.0);
    std::vector<double> _tofy(xLength - 1, 0.0);
    std::vector<double> _thetay(xLength - 1, 0.0);

    for (int j = 0; j < wlLength - 1; j++) {
      const double wl = (XIn[j + 1] + XIn[j]) / 2.0;
      const double wl_bin = XIn[j + 1] - XIn[j];
      if (!isEmpty(min_wl) && wl < min_wl)
        continue;
      if (!isEmpty(max_wl) && wl > max_wl)
        continue;
      const double q = factor / wl;
      int iq = 0;

      // Bin assignment depends on whether we have log or linear bins
      // TODO: change this so that we don't have to pass in the binning
      // parameters
      if (binParams[1] > 0.0) {
        iq = static_cast<int>(floor((q - binParams[0]) / binParams[1]));
      } else {
        iq = static_cast<int>(
            floor(log(q / binParams[0]) / log(1.0 - binParams[1])));
      }

      const double src_to_pixel = L1 + L2;
      const double dTheta2 =
          (3.0 * R1 * R1 / (L1 * L1) +
           3.0 * R2 * R2 * src_to_pixel * src_to_pixel / (L1 * L1 * L2 * L2) +
           2.0 * (pixel_size_x * pixel_size_x + pixel_size_y * pixel_size_y) /
               (L2 * L2)) /
          12.0;

      // This term is related to the TOF resolution
      const double dwl_over_wl =
          3.9560 * getTOFResolution(wl) / (1000.0 * (L1 + L2) * wl);
      // This term is related to the wavelength binning
      const double wl_bin_over_wl = wl_bin / wl;
      const double dq_over_q =
          std::sqrt(dTheta2 / (theta * theta) + dwl_over_wl * dwl_over_wl +
                    wl_bin_over_wl * wl_bin_over_wl);

      // By using only events with a positive weight, we use only the data
      // distribution and leave out the background events.
      // Note: we are looping over bins, therefore the xLength-1.
      if (iq >= 0 && iq < xLength - 1 && !std::isnan(dq_over_q) &&
          dq_over_q > 0 && YIn[j] > 0) {
        _dx[iq] += q * dq_over_q * YIn[j];
        _norm[iq] += YIn[j];
        _tofy[iq] += q * std::fabs(dwl_over_wl) * YIn[j];
        _thetay[iq] += q * std::sqrt(dTheta2) / theta * YIn[j];
      }
    }

    // Move over the distributions for that pixel
    // Note: we are looping over bins, therefore the xLength-1.
    PARALLEL_CRITICAL(iq) /* Write to shared memory - must protect */
    for (int iq = 0; iq < xLength - 1; iq++) {
      DxOut[iq] += _dx[iq];
      XNorm[iq] += _norm[iq];
      TOFY[iq] += _tofy[iq];
      ThetaY[iq] += _thetay[iq];
    }

    progress.report("Computing Q resolution");
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // Normalize according to the chosen weighting scheme
  // Note: we are looping over bins, therefore the xLength-1.
  for (int i = 0; i < xLength - 1; i++) {
    if (XNorm[i] == 0)
      continue;
    DxOut[i] /= XNorm[i];
    TOFY[i] /= XNorm[i];
    ThetaY[i] /= XNorm[i];
  }
  iqWS->setPointStandardDeviations(0, std::move(DxOut));
}
} // namespace Algorithms
} // namespace Mantid
