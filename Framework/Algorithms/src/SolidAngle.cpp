// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/SolidAngle.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/UnitFactory.h"

#include <cfloat>

namespace Mantid {
namespace Algorithms {

// Register with the algorithm factory
DECLARE_ALGORITHM(SolidAngle)

using namespace Kernel;
using namespace API;
using namespace Geometry;

namespace {
constexpr double MM_TO_METERS = 1. / 1000.;

/**
 * Returns the angle between the sample-to-pixel vector and its
 * projection on the X-Z (vertical tube) or Y-Z (horizontal tube) plane.
 * Note, in all cases Y is assumed to be the pointing-up direction, Z is the
 * beam direction.
 */
double getTubeAngle(const DetectorInfo &detectorInfo, size_t index,
                    const bool vertical) {
  const auto sampleDetVec =
      detectorInfo.position(index) - detectorInfo.samplePosition();
  auto inPlane = sampleDetVec;
  if (vertical)
    inPlane.setY(0.0);
  else
    inPlane.setX(0.0);
  return sampleDetVec.angle(inPlane);
}

/**
 *Returns correct angular function for the specified method
 */
std::function<double(size_t)>
getSolidAngleFunction(const DetectorInfo &detectorInfo,
                      const std::string &method, const double pixelArea) {
  if (method == "GenericShape") {
    const auto &samplePosition = detectorInfo.samplePosition();

    return [&detectorInfo, &samplePosition](size_t index) {
      return detectorInfo.detector(index).solidAngle(samplePosition);
    };
  } else if (method == "Rectangular") {
    return [&detectorInfo, pixelArea](size_t index) {
      const double cosTheta = std::cos(detectorInfo.twoTheta(index));
      const double l2 = detectorInfo.l2(index);
      return pixelArea * cosTheta / (l2 * l2);
    };
  } else if (method == "VerticalTube") {
    return [&detectorInfo, pixelArea](size_t index) {
      const double cosAlpha = std::cos(getTubeAngle(detectorInfo, index, true));
      const double l2 = detectorInfo.l2(index);
      return pixelArea * cosAlpha / (l2 * l2);
    };
  } else if (method == "HorizontalTube") {
    return [&detectorInfo, pixelArea](size_t index) {
      const double cosAlpha =
          std::cos(getTubeAngle(detectorInfo, index, false));
      const double l2 = detectorInfo.l2(index);
      return pixelArea * cosAlpha / (l2 * l2);
    };
  } else if (method == "VerticalWing") {
    return [&detectorInfo, pixelArea](size_t index) {
      const double cosTheta = std::cos(detectorInfo.twoTheta(index));
      const double cosAlpha = std::cos(getTubeAngle(detectorInfo, index, true));
      const double l2 = detectorInfo.l2(index);
      return pixelArea * cosAlpha * cosAlpha * cosAlpha / (l2 * l2 * cosTheta * cosTheta);
    };
  } else if (method == "HorizontalWing") {
    return [&detectorInfo, pixelArea](size_t index) {
      const double cosTheta = std::cos(detectorInfo.twoTheta(index));
      const double cosAlpha =
          std::cos(getTubeAngle(detectorInfo, index, false));
      const double l2 = detectorInfo.l2(index);
      return pixelArea * cosAlpha * cosAlpha * cosAlpha/ (l2 * l2 * cosTheta * cosTheta);
    };
  } else {
    throw std::runtime_error("Unknown method of solid angle calculation.");
  }
}
} // namespace

/// Initialisation method
void SolidAngle::init() {
  declareProperty(std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input,
                      boost::make_shared<InstrumentValidator>()),
                  "This workspace is used to identify the instrument to use "
                  "and also which\n"
                  "spectra to create a solid angle for. If the Max and Min "
                  "spectra values are\n"
                  "not provided one solid angle will be created for each "
                  "spectra in the input\n"
                  "workspace");
  declareProperty(std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "The name of the workspace to be created as the output of "
                  "the algorithm.  A workspace of this name will be created "
                  "and stored in the Analysis Data Service.");

  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty("StartWorkspaceIndex", 0, mustBePositive,
                  "The index number of the first spectrum for which to find "
                  "the solid angle\n"
                  "(default: 0)");
  declareProperty("EndWorkspaceIndex", EMPTY_INT(), mustBePositive,
                  "The index of the last spectrum whose solid angle is to be "
                  "found (default: the\n"
                  "last spectrum in the workspace)");

  const std::vector<std::string> methods{"GenericShape", "Rectangular",
                                         "VerticalTube", "HorizontalTube",
                                         "VerticalWing", "HorizontalWing"};
  declareProperty(
      "Method", "GenericShape",
      boost::make_shared<StringListValidator>(methods),
      "Select the method to calculate the Solid Angle.");
}

/** Executes the algorithm
 */
void SolidAngle::exec() {
  // Get the workspaces
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  int m_MinSpec = getProperty("StartWorkspaceIndex");
  int m_MaxSpec = getProperty("EndWorkspaceIndex");

  const int numberOfSpectra = static_cast<int>(inputWS->getNumberHistograms());

  // Check 'StartSpectrum' is in range 0-numberOfSpectra
  if (m_MinSpec > numberOfSpectra) {
    g_log.warning("StartWorkspaceIndex out of range! Set to 0.");
    m_MinSpec = 0;
  }
  if (isEmpty(m_MaxSpec))
    m_MaxSpec = numberOfSpectra - 1;
  if (m_MaxSpec > numberOfSpectra - 1 || m_MaxSpec < m_MinSpec) {
    g_log.warning("EndWorkspaceIndex out of range! Set to max detector number");
    m_MaxSpec = numberOfSpectra - 1;
  }

  MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create(
      inputWS, numberOfSpectra, 2, 1);
  // The result of this will be a distribution
  outputWS->setDistribution(true);
  outputWS->setYUnit("");
  outputWS->setYUnitLabel("Steradian");
  setProperty("OutputWorkspace", outputWS);

  const auto &spectrumInfo = inputWS->spectrumInfo();
  const auto &detectorInfo = inputWS->detectorInfo();

  // this is the solid angle of the pixel at 2theta=0
  // this is used only if Method != GenericShape
  double pixelAreaZero = 0.;
  const std::string method = getProperty("Method");

  if (method != "GenericShape") {
    const auto instrument = inputWS->getInstrument();
    if (instrument->hasParameter("x-pixel-size") &&
        instrument->hasParameter("y-pixel-size")) {
      const double pixelSizeX =
          instrument->getNumberParameter("x-pixel-size")[0] * MM_TO_METERS;
      const double pixelSizeY =
          instrument->getNumberParameter("y-pixel-size")[0] * MM_TO_METERS;
      pixelAreaZero = pixelSizeX * pixelSizeY; // l2 is retrieved per pixel
    } else {
      // TODO: get the l2 as Z coordinate of the whole bank, and pixel sizes
      // from bounding box
      throw std::runtime_error(
          "Missing necessary instrument parameters for non generic shape.");
    }
  }

  const auto solidAngleFunction =
      getSolidAngleFunction(detectorInfo, method, pixelAreaZero);
  int failCount = 0;
  Progress prog(this, 0.0, 1.0, numberOfSpectra);

  // Loop over the histograms (detector spectra)
  PARALLEL_FOR_IF(Kernel::threadSafe(*outputWS, *inputWS))
  for (int j = 0; j < numberOfSpectra; ++j) {
    PARALLEL_START_INTERUPT_REGION
    outputWS->mutableX(j)[0] = inputWS->x(j).front();
    outputWS->mutableX(j)[1] = inputWS->x(j).back();
    outputWS->mutableE(j) = 0.;
    outputWS->mutableY(j) = 0.; // default value for not calculated
    if (j >= m_MinSpec && j < m_MaxSpec) {
      if (spectrumInfo.hasDetectors(j)) {
        double solidAngle = 0.0;
        for (const auto detID : inputWS->getSpectrum(j).getDetectorIDs()) {
          const auto index = detectorInfo.indexOf(detID);
          if (!detectorInfo.isMasked(index) && !detectorInfo.isMonitor(index))
            solidAngle += solidAngleFunction(index);
        }
        outputWS->mutableY(j)[0] = solidAngle;
      } else {
        ++failCount;
      }
    }

    prog.report();
    PARALLEL_END_INTERUPT_REGION
  } // loop over spectra
  PARALLEL_CHECK_INTERUPT_REGION

  if (failCount != 0) {
    g_log.information() << "Unable to calculate solid angle for " << failCount
                        << " spectra. The solid angle will be set to zero for "
                           "those detectors.\n";
  }
}

} // namespace Algorithms
} // namespace Mantid
