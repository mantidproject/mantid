// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/Q1DWeighted.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidAlgorithms/GravitySANSHelper.h"
#include "MantidDataObjects/Histogram1D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/UnitConversion.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/VectorHelper.h"

constexpr double deg2rad = M_PI / 180.0;

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(Q1DWeighted)

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

void Q1DWeighted::init() {
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<HistogramValidator>();
  wsValidator->add<InstrumentValidator>();
  declareProperty(make_unique<WorkspaceProperty<>>(
                      "InputWorkspace", "", Direction::Input, wsValidator),
                  "Input workspace containing the SANS 2D data");
  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "Workspace that will contain the I(Q) data");
  declareProperty(
      make_unique<ArrayProperty<double>>(
          "OutputBinning", boost::make_shared<RebinParamsValidator>()),
      "The new bin boundaries in the form: <math>x_1,\\Delta x_1,x_2,\\Delta "
      "x_2,\\dots,x_n</math>");

  auto positiveInt = boost::make_shared<BoundedValidator<int>>();
  positiveInt->setLower(0);
  auto positiveDouble = boost::make_shared<BoundedValidator<double>>();
  positiveDouble->setLower(0);

  declareProperty("NPixelDivision", 1, positiveInt,
                  "Number of sub-pixels used for each detector pixel in each "
                  "direction.The total number of sub-pixels will be "
                  "NPixelDivision*NPixelDivision.");

  // Wedge properties
  declareProperty("NumberOfWedges", 2, positiveInt,
                  "Number of wedges to calculate.");
  declareProperty("WedgeAngle", 30.0, positiveDouble,
                  "Opening angle of the wedge, in degrees.");
  declareProperty("WedgeOffset", 0.0, positiveDouble,
                  "Wedge offset relative to the horizontal axis, in degrees.");
  declareProperty(
      make_unique<WorkspaceProperty<WorkspaceGroup>>(
          "WedgeWorkspace", "", Direction::Output, PropertyMode::Optional),
      "Name for the WorkspaceGroup containing the wedge I(q) distributions.");

  declareProperty("PixelSizeX", 5.15, positiveDouble,
                  "Pixel size in the X direction (mm).");
  declareProperty("PixelSizeY", 5.15, positiveDouble,
                  "Pixel size in the Y direction (mm).");
  declareProperty(
      "ErrorWeighting", false,
      "Choose whether each pixel contribution will be weighted by 1/error^2.");

  declareProperty("AsymmetricWedges", false,
                  "Choose to produce the results for asymmetric wedges.");

  declareProperty("AccountForGravity", false,
                  "Take the nominal gravity drop into account.");
}

void Q1DWeighted::exec() {
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

  // Get pixel size and pixel sub-division
  double pixelSizeX = getProperty("PixelSizeX");
  double pixelSizeY = getProperty("PixelSizeY");
  // Convert from mm to meters
  pixelSizeX /= 1000.0;
  pixelSizeY /= 1000.0;
  const int nSubPixels = getProperty("NPixelDivision");

  // Get weighting option
  const bool errorWeighting = getProperty("ErrorWeighting");

  // Get gravity flag
  const bool correctGravity = getProperty("AccountForGravity");

  // Calculate the output binning
  const std::vector<double> binParams = getProperty("OutputBinning");
  std::vector<double> qBinEdges;
  // number of Q bins in the output
  const size_t nQ = static_cast<size_t>(VectorHelper::createAxisFromRebinParams(
                        binParams, qBinEdges)) -
                    1;

  // number of spectra in the input
  const size_t nSpec = inputWS->getNumberHistograms();
  const V3D sourcePos = inputWS->getInstrument()->getSource()->getPos();
  const V3D samplePos = inputWS->getInstrument()->getSample()->getPos();
  // Beam line axis, to compute scattering angle
  const V3D beamLine = samplePos - sourcePos;

  // Get wedge properties
  const int wedges = getProperty("NumberOfWedges");
  const size_t nWedges = static_cast<size_t>(wedges);
  const double wedgeOffset = getProperty("WedgeOffset");
  const double wedgeAngle = getProperty("WedgeAngle");
  const bool asymmWedges = getProperty("AsymmetricWedges");

  // When symmetric wedges are requested (default), we need to divide
  // 180/nWedges. When asymmetric wedges are requested, we need to divide
  // 360/nWedges
  double wedgeFullAngle = 180.;
  if (asymmWedges) {
    wedgeFullAngle *= 2;
  }

  // get the number of wavelength bins in the input, note that the input is a
  // histogram
  const size_t nLambda = inputWS->readX(0).size() - 1;

  // we store everything in 3D arrays
  // index 1 : is for the wedges + the one for the full integration,
  //           if there are no wedges, the 1st dimension will be 1
  // index 2 : will iterate over lambda bins
  // index 3 : will iterate over Q bins
  // we want to do this, since we want to average the I(Q) in each lambda bin
  // then average all the I(Q)s together
  std::vector<std::vector<std::vector<double>>> intensities(
      nWedges + 1,
      std::vector<std::vector<double>>(nLambda, std::vector<double>(nQ, 0.0)));

  // the same for the errors
  std::vector<std::vector<std::vector<double>>> errors(
      nWedges + 1,
      std::vector<std::vector<double>>(nLambda, std::vector<double>(nQ, 0.0)));

  // the same dimensions for the normalisation weights
  std::vector<std::vector<std::vector<double>>> normalisation(
      nWedges + 1,
      std::vector<std::vector<double>>(nLambda, std::vector<double>(nQ, 0.0)));

  const auto &spectrumInfo = inputWS->spectrumInfo();

  // Set up the progress
  Progress progress(this, 0.0, 1.0, nSpec * nLambda);

  PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS))
  // first we loop over spectra
  for (int index = 0; index < static_cast<int>(nSpec); ++index) {
    PARALLEL_START_INTERUPT_REGION
    const size_t i = static_cast<size_t>(index);
    // skip spectra with no detectors, monitors or masked spectra
    if (!spectrumInfo.hasDetectors(i) || spectrumInfo.isMonitor(i) ||
        spectrumInfo.isMasked(i)) {
      continue;
    }

    // store masked bins
    std::vector<size_t> maskedBins;
    // check if we have masked bins
    if (inputWS->hasMaskedBins(i)) {
      maskedBins = inputWS->maskedBinsIndices(i);
    }

    // get readonly references to the input data
    const auto &XIn = inputWS->x(i);
    const auto &YIn = inputWS->y(i);
    const auto &EIn = inputWS->e(i);

    // get the position of the pixel wrt sample (normally 0,0,0).
    const V3D pos = spectrumInfo.position(i) - samplePos;

    // prepare a gravity helper, this is much faster than calculating
    // on-the-fly, see the caching in the helper
    GravitySANSHelper gravityHelper(spectrumInfo, i, 0.0);

    // loop over lambda bins
    for (size_t j = 0; j < nLambda; ++j) {

      // skip if the bin is masked
      if (std::binary_search(maskedBins.cbegin(), maskedBins.cend(), j)) {
        continue;
      }

      const double wavelength = (XIn[j] + XIn[j + 1]) / 2.;

      V3D correction;
      if (correctGravity) {
        correction.setY(gravityHelper.gravitationalDrop(wavelength));
      }

      // Each pixel might be sub-divided in the number of pixels given as input
      // parameter (NPixelDivision x NPixelDivision)
      for (int isub = 0; isub < nSubPixels * nSubPixels; ++isub) {

        // Find the position offset for this sub-pixel in real space
        const double subY = pixelSizeY *
                            ((isub % nSubPixels) - (nSubPixels - 1.0) / 2.0) /
                            nSubPixels;
        const double subX = pixelSizeX *
                            (floor(static_cast<double>(isub) / nSubPixels) -
                             (nSubPixels - 1.0) * 0.5) /
                            nSubPixels;

        // calculate Q
        const V3D position = pos - V3D(subX, subY, 0.0) + correction;
        const double sinTheta = sin(0.5 * position.angle(beamLine));
        const double q = 4.0 * M_PI * sinTheta / wavelength;

        if (q < qBinEdges.front() || q > qBinEdges.back()) {
          continue;
        }

        // after check above, no need to wrap this in try catch
        const size_t k = VectorHelper::indexOfValueFromEdges(qBinEdges, q);

        double w = 1.0;
        if (errorWeighting) {
          // When using the error as weight we have:
          //    w_i = 1/s_i^2   where s_i is the uncertainty on the ith
          //    pixel.
          //
          //    I(q_i) = (sum over i of I_i * w_i) / (sum over i of w_i)
          //       where all pixels i contribute to the q_i bin, and I_i is
          //       the intensity in the ith pixel.
          //
          //    delta I(q_i) = 1/sqrt( (sum over i of w_i) )  using simple
          //    error propagation.
          double err = 1.0;
          if (EIn[j] > 0)
            err = EIn[j];
          w /= nSubPixels * nSubPixels * err * err;
        }
        PARALLEL_CRITICAL(iqnorm) /* Write to shared memory - must protect */
        {
          // Fill in the data for full azimuthal integral
          intensities[0][j][k] += YIn[j] * w;
          errors[0][j][k] += w * w * EIn[j] * EIn[j];
          normalisation[0][j][k] += w;

          if (nWedges != 0) {
            // we do need to loop over all the wedges, since there is no
            // restriction for those; they can also overlap
            // that is the same pixel can simultaneously be in many wedges
            for (size_t iw = 0; iw < nWedges; ++iw) {
              double centerAngle =
                  static_cast<double>(iw) * M_PI / static_cast<double>(nWedges);
              if (asymmWedges) {
                centerAngle *= 2;
              }
              centerAngle += deg2rad * wedgeOffset;
              const V3D subPix = V3D(position.X(), position.Y(), 0.0);
              const double angle = fabs(
                  subPix.angle(V3D(cos(centerAngle), sin(centerAngle), 0.0)));
              if (angle < deg2rad * wedgeAngle * 0.5 ||
                  (!asymmWedges &&
                   fabs(M_PI - angle) < deg2rad * wedgeAngle * 0.5)) {
                // first index 0 is the full azimuth, need to offset +1
                intensities[iw + 1][j][k] += YIn[j] * w;
                errors[iw + 1][j][k] += w * w * EIn[j] * EIn[j];
                normalisation[iw + 1][j][k] += w;
              }
            }
          }
        }
      }
      progress.report("Computing I(Q)");
    }
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  MatrixWorkspace_sptr outputWS = createOutputWorkspace(inputWS, nQ);

  // Set the X vector for the output workspace
  outputWS->setBinEdges(0, qBinEdges);
  auto &YOut = outputWS->mutableY(0);
  auto &EOut = outputWS->mutableE(0);

  std::vector<double> normLambda(nQ, 0.0);

  for (size_t il = 0; il < nLambda; ++il) {
    for (size_t iq = 0; iq < nQ; ++iq) {
      const double &norm = normalisation[0][il][iq];
      if (norm != 0.) {
        YOut[iq] += intensities[0][il][iq] / norm;
        EOut[iq] += errors[0][il][iq] / (norm * norm);
        normLambda[iq] += 1.;
      }
    }
  }

  for (size_t i = 0; i < nQ; ++i) {
    YOut[i] /= normLambda[i];
    EOut[i] = sqrt(EOut[i]) / normLambda[i];
  }

  setProperty("OutputWorkspace", outputWS);

  if (nWedges != 0) {

    // Create workspace group that holds output workspaces
    auto wsgroup = boost::make_shared<WorkspaceGroup>();

    // Create wedge workspaces
    for (size_t iw = 0; iw < nWedges; ++iw) {
      const double centerAngle = static_cast<double>(iw) * wedgeFullAngle /
                                     static_cast<double>(nWedges) +
                                 wedgeOffset;
      MatrixWorkspace_sptr wedgeWs = createOutputWorkspace(inputWS, nQ);
      wedgeWs->setBinEdges(0, qBinEdges);
      wedgeWs->mutableRun().addProperty("wedge_angle", centerAngle, "degrees",
                                        true);

      auto &YOut = wedgeWs->mutableY(0);
      auto &EOut = wedgeWs->mutableE(0);

      std::vector<double> normLambda(nQ, 0.0);

      for (size_t il = 0; il < nLambda; ++il) {
        for (size_t iq = 0; iq < nQ; ++iq) {
          const double &norm = normalisation[iw + 1][il][iq];
          if (norm != 0.) {
            YOut[iq] += intensities[iw + 1][il][iq] / norm;
            EOut[iq] += errors[iw + 1][il][iq] / (norm * norm);
            normLambda[iq] += 1.;
          }
        }
      }

      for (size_t i = 0; i < nQ; ++i) {
        YOut[i] /= normLambda[i];
        EOut[i] = sqrt(EOut[i]) / normLambda[i];
      }

      wsgroup->addWorkspace(wedgeWs);
    }

    // set the output property
    std::string outputWSGroupName = getPropertyValue("WedgeWorkspace");
    if (outputWSGroupName.empty()) {
      std::string outputWSName = getPropertyValue("OutputWorkspace");
      outputWSGroupName = outputWSName + "_wedges";
      setPropertyValue("WedgeWorkspace", outputWSGroupName);
    }
    setProperty("WedgeWorkspace", wsgroup);
  }
}

MatrixWorkspace_sptr
Q1DWeighted::createOutputWorkspace(MatrixWorkspace_const_sptr parent,
                                   const size_t nBins) {

  MatrixWorkspace_sptr outputWS =
      WorkspaceFactory::Instance().create(parent, 1, nBins + 1, nBins);
  outputWS->getAxis(0)->unit() =
      UnitFactory::Instance().create("MomentumTransfer");
  outputWS->setYUnitLabel("1/cm");
  outputWS->setDistribution(true);
  return outputWS;
}

} // namespace Algorithms
} // namespace Mantid
