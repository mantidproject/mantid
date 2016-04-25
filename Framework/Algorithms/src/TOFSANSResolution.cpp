//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/TOFSANSResolution.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/VectorHelper.h"

#include "boost/math/special_functions/fpclassify.hpp"

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
      make_unique<WorkspaceProperty<>>(
          "InputWorkspace", "", Direction::InOut,
          boost::make_shared<WorkspaceUnitValidator>("MomentumTransfer")),
      "Name the workspace to calculate the resolution for");

  auto wsValidator = boost::make_shared<WorkspaceUnitValidator>("Wavelength");
  declareProperty(make_unique<WorkspaceProperty<>>(
                      "ReducedWorkspace", "", Direction::Input, wsValidator),
                  "I(Q) workspace");
  declareProperty(make_unique<ArrayProperty<double>>(
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

void TOFSANSResolution::exec() {
  MatrixWorkspace_sptr iqWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr reducedWS = getProperty("ReducedWorkspace");
  EventWorkspace_sptr reducedEventWS =
      boost::dynamic_pointer_cast<EventWorkspace>(reducedWS);
  const double min_wl = getProperty("MinWavelength");
  const double max_wl = getProperty("MaxWavelength");
  double pixel_size_x = getProperty("PixelSizeX");
  double pixel_size_y = getProperty("PixelSizeY");
  double R1 = getProperty("SourceApertureRadius");
  double R2 = getProperty("SampleApertureRadius");
  // Convert to meters
  pixel_size_x /= 1000.0;
  pixel_size_y /= 1000.0;
  R1 /= 1000.0;
  R2 /= 1000.0;
  m_wl_resolution = getProperty("DeltaT");

  // Calculate the output binning
  const std::vector<double> binParams = getProperty("OutputBinning");

  // Count histogram for normalization
  const int xLength = static_cast<int>(iqWS->readX(0).size());
  std::vector<double> XNorm(xLength - 1, 0.0);

  // Create workspaces with each component of the resolution for debugging
  // purposes
  MatrixWorkspace_sptr thetaWS = WorkspaceFactory::Instance().create(iqWS);
  declareProperty(
      make_unique<WorkspaceProperty<>>("ThetaError", "", Direction::Output));
  setPropertyValue("ThetaError", "__" + iqWS->getName() + "_theta_error");
  setProperty("ThetaError", thetaWS);
  thetaWS->setX(0, iqWS->readX(0));
  MantidVec &ThetaY = thetaWS->dataY(0);

  MatrixWorkspace_sptr tofWS = WorkspaceFactory::Instance().create(iqWS);
  declareProperty(
      make_unique<WorkspaceProperty<>>("TOFError", "", Direction::Output));
  setPropertyValue("TOFError", "__" + iqWS->getName() + "_tof_error");
  setProperty("TOFError", tofWS);
  tofWS->setX(0, iqWS->readX(0));
  MantidVec &TOFY = tofWS->dataY(0);

  // Initialize Dq
  MantidVec &DxOut = iqWS->dataDx(0);
  for (int i = 0; i < xLength - 1; i++)
    DxOut[i] = 0.0;

  const V3D samplePos = reducedWS->getInstrument()->getSample()->getPos();
  const V3D sourcePos = reducedWS->getInstrument()->getSource()->getPos();
  const V3D SSD = samplePos - sourcePos;
  const double L1 = SSD.norm();

  const int numberOfSpectra =
      static_cast<int>(reducedWS->getNumberHistograms());
  Progress progress(this, 0.0, 1.0, numberOfSpectra);

  PARALLEL_FOR2(reducedWS, iqWS)
  for (int i = 0; i < numberOfSpectra; i++) {
    PARALLEL_START_INTERUPT_REGION
    IDetector_const_sptr det;
    try {
      det = reducedWS->getDetector(i);
    } catch (Exception::NotFoundError &) {
      g_log.warning() << "Workspace index " << i
                      << " has no detector assigned to it - discarding"
                      << std::endl;
      // Catch if no detector. Next line tests whether this happened - test
      // placed
      // outside here because Mac Intel compiler doesn't like 'continue' in a
      // catch
      // in an openmp block.
    }
    // If no detector found or if it's masked or a monitor, skip onto the next
    // spectrum
    if (!det || det->isMonitor() || det->isMasked())
      continue;

    // Get the flight path from the sample to the detector pixel
    const V3D scattered_flight_path = det->getPos() - samplePos;

    // Multiplicative factor to go from lambda to Q
    // Don't get fooled by the function name...
    const double theta = reducedWS->detectorTwoTheta(*det);
    const double factor = 4.0 * M_PI * sin(0.5 * theta);

    const MantidVec &XIn = reducedWS->readX(i);
    const MantidVec &YIn = reducedWS->readY(i);
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

      const double L2 = scattered_flight_path.norm();
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
      // distribution and
      // leave out the background events
      if (iq >= 0 && iq < xLength - 1 && !boost::math::isnan(dq_over_q) &&
          dq_over_q > 0 && YIn[j] > 0) {
        _dx[iq] += q * dq_over_q * YIn[j];
        _norm[iq] += YIn[j];
        _tofy[iq] += q * std::fabs(dwl_over_wl) * YIn[j];
        _thetay[iq] += q * std::sqrt(dTheta2) / theta * YIn[j];
      }
    }

    // Move over the distributions for that pixel
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
  for (int i = 0; i < xLength - 1; i++) {
    if (XNorm[i] == 0)
      continue;
    DxOut[i] /= XNorm[i];
    TOFY[i] /= XNorm[i];
    ThetaY[i] /= XNorm[i];
  }
}
} // namespace Algorithms
} // namespace Mantid
