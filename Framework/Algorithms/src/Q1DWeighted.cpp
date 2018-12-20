#include "MantidAlgorithms/Q1DWeighted.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/Histogram1D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/RebinParamsValidator.h"
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
}

void Q1DWeighted::exec() {
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

  // Calculate the output binning
  const std::vector<double> binParams = getProperty("OutputBinning");
  // XOut defines the output histogram, so its length is equal to the number of
  // bins + 1
  HistogramData::BinEdges XOut(0);
  const int sizeOut =
      VectorHelper::createAxisFromRebinParams(binParams, XOut.mutableRawData());

  // Get pixel size and pixel sub-division
  double pixelSizeX = getProperty("PixelSizeX");
  double pixelSizeY = getProperty("PixelSizeY");
  // Convert from mm to meters
  pixelSizeX /= 1000.0;
  pixelSizeY /= 1000.0;
  int nSubPixels = getProperty("NPixelDivision");

  // Get weighting option
  const bool errorWeighting = getProperty("ErrorWeighting");

  // Now create the output workspace
  MatrixWorkspace_sptr outputWS =
      WorkspaceFactory::Instance().create(inputWS, 1, sizeOut, sizeOut - 1);
  outputWS->getAxis(0)->unit() =
      UnitFactory::Instance().create("MomentumTransfer");
  outputWS->setYUnitLabel("1/cm");
  outputWS->setDistribution(true);
  setProperty("OutputWorkspace", outputWS);

  // Set the X vector for the output workspace
  outputWS->setBinEdges(0, XOut);
  auto &YOut = outputWS->mutableY(0);
  auto &EOut = outputWS->mutableE(0);

  const int numSpec = static_cast<int>(inputWS->getNumberHistograms());

  const V3D sourcePos = inputWS->getInstrument()->getSource()->getPos();
  const V3D samplePos = inputWS->getInstrument()->getSample()->getPos();

  const int xLength = static_cast<int>(inputWS->readX(0).size());
  constexpr double fmp = 4.0 * M_PI;

  // Set up the progress reporting object
  Progress progress(this, 0.0, 1.0, numSpec * (xLength - 1));

  // Count histogram for normalization
  std::vector<double> XNormLambda(sizeOut - 1, 0.0);

  // Beam line axis, to compute scattering angle
  V3D beamLine = samplePos - sourcePos;

  // Get wedge properties
  const int nWedges = getProperty("NumberOfWedges");
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

  // Create wedge workspaces
  std::vector<MatrixWorkspace_sptr> wedgeWorkspaces;
  for (int iWedge = 0; iWedge < nWedges; iWedge++) {
    double centerAngle = wedgeFullAngle / nWedges * iWedge;
    centerAngle += wedgeOffset;

    MatrixWorkspace_sptr wedgeWs =
        WorkspaceFactory::Instance().create(inputWS, 1, sizeOut, sizeOut - 1);
    wedgeWs->getAxis(0)->unit() =
        UnitFactory::Instance().create("MomentumTransfer");
    wedgeWs->setYUnitLabel("1/cm");
    wedgeWs->setDistribution(true);
    wedgeWs->setBinEdges(0, XOut);
    wedgeWs->mutableRun().addProperty("wedge_angle", centerAngle, "degrees",
                                      true);
    wedgeWorkspaces.push_back(wedgeWs);
  }

  // Count histogram for wedge normalization
  std::vector<std::vector<double>> wedgeXNormLambda(
      nWedges, std::vector<double>(sizeOut - 1, 0.0));

  const auto &spectrumInfo = inputWS->spectrumInfo();

  PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS, *outputWS))
  // Loop over all xLength-1 detector channels
  // Note: xLength -1, because X is a histogram and has a number of boundaries
  // equal to the number of detector channels + 1.
  for (int j = 0; j < xLength - 1; j++) {
    PARALLEL_START_INTERUPT_REGION

    std::vector<double> lambdaIq(sizeOut - 1, 0.0);
    std::vector<double> lambdaIqErr(sizeOut - 1, 0.0);
    std::vector<double> XNorm(sizeOut - 1, 0.0);

    // Wedges
    std::vector<std::vector<double>> wedgeLambdaIq(
        nWedges, std::vector<double>(sizeOut - 1, 0.0));
    std::vector<std::vector<double>> wedgeLambdaIqErr(
        nWedges, std::vector<double>(sizeOut - 1, 0.0));
    std::vector<std::vector<double>> wedgeXNorm(
        nWedges, std::vector<double>(sizeOut - 1, 0.0));

    for (int i = 0; i < numSpec; i++) {
      if (!spectrumInfo.hasDetectors(i)) {
        g_log.warning() << "Workspace index " << i
                        << " has no detector assigned to it - discarding\n";
        continue;
      }
      // Skip if we have a monitor or if the detector is masked.
      if (spectrumInfo.isMonitor(i) || spectrumInfo.isMasked(i))
        continue;

      // Get the current spectrum for both input workspaces
      auto &XIn = inputWS->x(i);
      auto &YIn = inputWS->y(i);
      auto &EIn = inputWS->e(i);

      // Each pixel is sub-divided in the number of pixels given as input
      // parameter (NPixelDivision)
      for (int isub = 0; isub < nSubPixels * nSubPixels; isub++) {
        // Find the position offset for this sub-pixel in real space
        double subY = pixelSizeY *
                      ((isub % nSubPixels) - (nSubPixels - 1.0) / 2.0) /
                      nSubPixels;
        double subX = pixelSizeX *
                      (floor(static_cast<double>(isub) / nSubPixels) -
                       (nSubPixels - 1.0) * 0.5) /
                      nSubPixels;

        // Find the position of this sub-pixel in real space and compute Q
        // For reference - in the case where we don't use sub-pixels, simply
        // use:
        //     double sinTheta = sin( spectrumInfo.twoTheta(i)/2.0 );
        V3D pos = spectrumInfo.position(i) - V3D(subX, subY, 0.0) - samplePos;
        double sinTheta = sin(0.5 * pos.angle(beamLine));
        double factor = fmp * sinTheta;
        double q = factor * 2.0 / (XIn[j] + XIn[j + 1]);
        int iq = 0;

        // Bin assignment depends on whether we have log or linear bins
        if (binParams.size() == 3) {
          if (binParams[1] > 0.0) {
            iq = static_cast<int>(floor((q - binParams[0]) / binParams[1]));
          } else {
            iq = static_cast<int>(
                floor(log(q / binParams[0]) / log(1.0 - binParams[1])));
          }
          // If we got a more complicated binning, find the q bin the slow way
        } else {
          for (int iQbin = 0; iQbin < static_cast<int>(XOut.size()) - 1;
               iQbin++) {
            if (q >= XOut[iQbin] && q < XOut[(iQbin + 1)]) {
              iq = iQbin;
              break;
            }
          }
        }

        if (iq >= 0 && iq < sizeOut - 1) {
          double w = 1.0;
          if (errorWeighting) {
            // When using the error as weight we have:
            //    w_i = 1/s_i^2   where s_i is the uncertainty on the ith pixel.
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
            w = 1.0 / (nSubPixels * nSubPixels * err * err);
          }

          PARALLEL_CRITICAL(iqnorm) /* Write to shared memory - must protect */
          {
            lambdaIq[iq] += YIn[j] * w;
            lambdaIqErr[iq] += w * w * EIn[j] * EIn[j];
            XNorm[iq] += w;

            // Fill in the wedge data
            for (int iWedge = 0; iWedge < nWedges; iWedge++) {
              double centerAngle = M_PI / nWedges * iWedge;
              if (asymmWedges) {
                centerAngle *= 2;
              }
              centerAngle += deg2rad * wedgeOffset;
              V3D subPix = V3D(pos.X(), pos.Y(), 0.0);
              double angle = fabs(
                  subPix.angle(V3D(cos(centerAngle), sin(centerAngle), 0.0)));
              if (angle < deg2rad * wedgeAngle * 0.5 ||
                  (!asymmWedges &&
                   fabs(M_PI - angle) < deg2rad * wedgeAngle * 0.5)) {
                wedgeLambdaIq[iWedge][iq] += YIn[j] * w;
                wedgeLambdaIqErr[iWedge][iq] += w * w * EIn[j] * EIn[j];
                wedgeXNorm[iWedge][iq] += w;
              }
            }
          }
        }
      }
      progress.report("Computing I(Q)");
    }
    // Normalize according to the chosen weighting scheme
    PARALLEL_CRITICAL(iq) /* Write to shared memory - must protect */
    {
      for (int k = 0; k < sizeOut - 1; k++) {
        if (XNorm[k] > 0) {
          YOut[k] += lambdaIq[k] / XNorm[k];
          EOut[k] += lambdaIqErr[k] / XNorm[k] / XNorm[k];
          XNormLambda[k] += 1.0;
        }

        // Normalize wedges
        for (int iWedge = 0; iWedge < nWedges; iWedge++) {
          if (wedgeXNorm[iWedge][k] > 0) {
            auto &wedgeYOut = wedgeWorkspaces[iWedge]->mutableY(0);
            auto &wedgeEOut = wedgeWorkspaces[iWedge]->mutableE(0);
            wedgeYOut[k] += wedgeLambdaIq[iWedge][k] / wedgeXNorm[iWedge][k];
            wedgeEOut[k] += wedgeLambdaIqErr[iWedge][k] /
                            wedgeXNorm[iWedge][k] / wedgeXNorm[iWedge][k];
            wedgeXNormLambda[iWedge][k] += 1.0;
          }
        }
      }
    }
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // Normalize according to the chosen weighting scheme
  for (int i = 0; i < sizeOut - 1; i++) {
    YOut[i] /= XNormLambda[i];
    EOut[i] = sqrt(EOut[i]) / XNormLambda[i];
  }
  for (int iWedge = 0; iWedge < nWedges; iWedge++) {
    for (int i = 0; i < sizeOut - 1; i++) {
      auto &wedgeYOut = wedgeWorkspaces[iWedge]->mutableY(0);
      auto &wedgeEOut = wedgeWorkspaces[iWedge]->mutableE(0);
      wedgeYOut[i] /= wedgeXNormLambda[iWedge][i];
      wedgeEOut[i] = sqrt(wedgeEOut[i]) / wedgeXNormLambda[iWedge][i];
    }
  }

  if (nWedges != 0) {
    // Create workspace group that holds output workspaces
    auto wsgroup = boost::make_shared<WorkspaceGroup>();
    for (auto &wedgeWorkspace : wedgeWorkspaces) {
      wsgroup->addWorkspace(wedgeWorkspace);
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

} // namespace Algorithms
} // namespace Mantid
