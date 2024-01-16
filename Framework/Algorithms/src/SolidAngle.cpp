// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
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

#include <atomic>

namespace Mantid::Algorithms {

// Register with the algorithm factory
DECLARE_ALGORITHM(SolidAngle)

namespace SolidAngleMethods {
static const std::string GENERIC_SHAPE = "GenericShape";
static const std::string RECTANGLE = "Rectangle";
static const std::string VERTICAL_TUBE = "VerticalTube";
static const std::string HORIZONTAL_TUBE = "HorizontalTube";
static const std::string VERTICAL_WING = "VerticalWing";
static const std::string HORIZONTAL_WING = "HorizontalWing";
} // namespace SolidAngleMethods

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace SolidAngleMethods;

namespace SolidAngleHelpers {

constexpr double MM_TO_METERS = 1. / 1000.;

/**
 * Returns the angle between the sample-to-pixel vector and its
 * projection on the X-Z (vertical tube) or Y-Z (horizontal tube) plane.
 * Note, in all cases Y is assumed to be the pointing-up direction, Z is the
 * beam direction.
 */
struct AlphaAngleCalculator {
  AlphaAngleCalculator(const DetectorInfo &detectorInfo)
      : m_detectorInfo(detectorInfo), m_samplePos(detectorInfo.samplePosition()) {}
  double getAlpha(size_t index) const {
    const auto sampleDetVec = m_detectorInfo.position(index) - m_samplePos;
    auto inPlane = sampleDetVec;
    project(inPlane);
    return sampleDetVec.cosAngle(inPlane);
  }
  virtual void project(V3D &v) const = 0;
  virtual ~AlphaAngleCalculator() = default;

private:
  const DetectorInfo &m_detectorInfo;
  const V3D m_samplePos;
};

struct AlphaAngleVertical : public AlphaAngleCalculator {
  using AlphaAngleCalculator::AlphaAngleCalculator;
  void project(V3D &v) const override { v.setY(0.0); }
};

struct AlphaAngleHorizontal : public AlphaAngleCalculator {
  using AlphaAngleCalculator::AlphaAngleCalculator;
  void project(V3D &v) const override { v.setX(0.0); }
};

/**
 *Creates the solid angle calculator based on the selected method.
 */
struct SolidAngleCalculator {
  SolidAngleCalculator(const ComponentInfo &componentInfo, const DetectorInfo &detectorInfo, const std::string &method,
                       const double pixelArea)
      : m_componentInfo(componentInfo), m_detectorInfo(detectorInfo), m_pixelArea(pixelArea),
        m_samplePos(detectorInfo.samplePosition()), m_beamLine(m_samplePos - detectorInfo.sourcePosition()) {
    if (method.find("Vertical") != std::string::npos) {
      m_alphaAngleCalculator = std::make_unique<AlphaAngleVertical>(detectorInfo);
    } else if (method.find("Horizontal") != std::string::npos) {
      m_alphaAngleCalculator = std::make_unique<AlphaAngleHorizontal>(detectorInfo);
    }
  }
  virtual double solidAngle(size_t index) const = 0;
  virtual ~SolidAngleCalculator() = default;

protected:
  const ComponentInfo &m_componentInfo;
  const DetectorInfo &m_detectorInfo;
  const double m_pixelArea;
  const V3D m_samplePos;
  const V3D m_beamLine;
  std::unique_ptr<const AlphaAngleCalculator> m_alphaAngleCalculator;
};

struct GenericShape : public SolidAngleCalculator {
  using SolidAngleCalculator::SolidAngleCalculator;
  GenericShape(const ComponentInfo &componentInfo, const DetectorInfo &detectorInfo, const std::string &method,
               const double pixelArea, const int numberOfCylinderSlices)
      : SolidAngleCalculator(componentInfo, detectorInfo, method, pixelArea),
        m_numberOfCylinderSlices(numberOfCylinderSlices) {}
  double solidAngle(size_t index) const override {
    return m_detectorInfo.detector(index).solidAngle(Geometry::SolidAngleParams(m_samplePos, m_numberOfCylinderSlices));
  }

private:
  int m_numberOfCylinderSlices;
};

struct Rectangle : public SolidAngleCalculator {
  using SolidAngleCalculator::SolidAngleCalculator;
  double solidAngle(size_t index) const override {
    const V3D sampleDetVec = m_detectorInfo.position(index) - m_samplePos;
    const double cosTheta = sampleDetVec.cosAngle(m_beamLine);
    const double l2 = m_detectorInfo.l2(index);
    const V3D scaleFactor = m_componentInfo.scaleFactor(index);
    const double scaledPixelArea = m_pixelArea * scaleFactor[0] * scaleFactor[1];
    return scaledPixelArea * cosTheta / (l2 * l2);
  }
};

struct Tube : public SolidAngleCalculator {
  using SolidAngleCalculator::SolidAngleCalculator;
  double solidAngle(size_t index) const override {
    const double cosAlpha = m_alphaAngleCalculator->getAlpha(index);
    const double l2 = m_detectorInfo.l2(index);
    const V3D scaleFactor = m_componentInfo.scaleFactor(index);
    const double scaledPixelArea = m_pixelArea * scaleFactor[0] * scaleFactor[1];
    return scaledPixelArea * cosAlpha / (l2 * l2);
  }
};

struct Wing : public SolidAngleCalculator {
  using SolidAngleCalculator::SolidAngleCalculator;
  double solidAngle(size_t index) const override {
    const V3D sampleDetVec = m_detectorInfo.position(index) - m_samplePos;
    const double cosTheta = sampleDetVec.cosAngle(m_beamLine);
    const double cosAlpha = m_alphaAngleCalculator->getAlpha(index);
    const double l2 = m_detectorInfo.l2(index);
    const V3D scaleFactor = m_componentInfo.scaleFactor(index);
    const double scaledPixelArea = m_pixelArea * scaleFactor[0] * scaleFactor[1];
    return scaledPixelArea * cosAlpha * cosAlpha * cosAlpha / (l2 * l2 * cosTheta * cosTheta);
  }
};

} // namespace SolidAngleHelpers

/// Initialisation method
void SolidAngle::init() {
  declareProperty(std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>("InputWorkspace", "", Direction::Input,
                                                                            std::make_shared<InstrumentValidator>()),
                  "This workspace is used to identify the instrument to use "
                  "and also which\n"
                  "spectra to create a solid angle for. If the Max and Min "
                  "spectra values are\n"
                  "not provided one solid angle will be created for each "
                  "spectra in the input\n"
                  "workspace");
  declareProperty(std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "The name of the workspace to be created as the output of "
                  "the algorithm.  A workspace of this name will be created "
                  "and stored in the Analysis Data Service.");

  auto mustBePositive = std::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty("StartWorkspaceIndex", 0, mustBePositive,
                  "The index number of the first spectrum for which to find "
                  "the solid angle\n"
                  "(default: 0)");
  declareProperty("EndWorkspaceIndex", EMPTY_INT(), mustBePositive,
                  "The index of the last spectrum whose solid angle is to be "
                  "found (default: the\n"
                  "last spectrum in the workspace)");

  const std::vector<std::string> methods{GENERIC_SHAPE,   RECTANGLE,     VERTICAL_TUBE,
                                         HORIZONTAL_TUBE, VERTICAL_WING, HORIZONTAL_WING};
  declareProperty("Method", GENERIC_SHAPE, std::make_shared<StringListValidator>(methods),
                  "Select the method to calculate the Solid Angle.");

  auto greaterThanTwo = std::make_shared<BoundedValidator<int>>();
  greaterThanTwo->setLower(3);
  declareProperty("NumberOfCylinderSlices", 10, greaterThanTwo,
                  "The number of angular slices used when triangulating a cylinder in order to calculate the solid "
                  "angle of a tube detector.");
}

/** Executes the algorithm
 */
void SolidAngle::exec() {
  // Get the workspaces
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  int m_MinSpec = getProperty("StartWorkspaceIndex");
  int m_MaxSpec = getProperty("EndWorkspaceIndex");

  const auto numberOfSpectra = static_cast<int>(inputWS->getNumberHistograms());

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

  MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create(inputWS, numberOfSpectra, 2, 1);
  // The result of this will be a distribution
  outputWS->setDistribution(true);
  outputWS->setYUnit("");
  outputWS->setYUnitLabel("Steradian");
  setProperty("OutputWorkspace", outputWS);

  const auto &spectrumInfo = inputWS->spectrumInfo();
  const auto &detectorInfo = inputWS->detectorInfo();
  const auto &componentInfo = inputWS->componentInfo();

  // this is the pixel area that is supposed to be constant for the whole
  // instrument this is used only if Method != GenericShape
  double pixelArea = 0.;
  const std::string method = getProperty("Method");

  using namespace SolidAngleHelpers;
  if (method != GENERIC_SHAPE) {
    const auto instrument = inputWS->getInstrument();
    if (instrument->hasParameter("x-pixel-size") && instrument->hasParameter("y-pixel-size")) {
      const double pixelSizeX = instrument->getNumberParameter("x-pixel-size")[0] * MM_TO_METERS;
      const double pixelSizeY = instrument->getNumberParameter("y-pixel-size")[0] * MM_TO_METERS;
      pixelArea = pixelSizeX * pixelSizeY; // l2 is retrieved per pixel
    } else {
      // TODO: try to get the pixel sizes from bounding box
      throw std::runtime_error("Missing necessary instrument parameters for non generic shape: "
                               "x-pixel-size and y-pixel-size [in mm].");
    }
  }

  int numberOfCylinderSlices = getProperty("NumberOfCylinderSlices");
  std::unique_ptr<SolidAngleCalculator> solidAngleCalculator;
  if (method == GENERIC_SHAPE) {
    solidAngleCalculator =
        std::make_unique<GenericShape>(componentInfo, detectorInfo, method, pixelArea, numberOfCylinderSlices);
  } else if (method == RECTANGLE) {
    solidAngleCalculator = std::make_unique<Rectangle>(componentInfo, detectorInfo, method, pixelArea);
  } else if (method == VERTICAL_TUBE || method == HORIZONTAL_TUBE) {
    solidAngleCalculator = std::make_unique<Tube>(componentInfo, detectorInfo, method, pixelArea);
  } else if (method == VERTICAL_WING || method == HORIZONTAL_WING) {
    solidAngleCalculator = std::make_unique<Wing>(componentInfo, detectorInfo, method, pixelArea);
  }

  std::atomic<size_t> failCount{0};
  Progress prog(this, 0.0, 1.0, numberOfSpectra);
  // Loop over the histograms (detector spectra)
  PARALLEL_FOR_IF(Kernel::threadSafe(*outputWS, *inputWS))
  for (int j = m_MinSpec; j <= m_MaxSpec; ++j) {
    PARALLEL_START_INTERRUPT_REGION
    initSpectrum(*inputWS, *outputWS, j);
    if (spectrumInfo.hasDetectors(j)) {
      double solidAngle = 0.0;
      for (const auto detID : inputWS->getSpectrum(j).getDetectorIDs()) {
        const auto index = detectorInfo.indexOf(detID);
        if (!detectorInfo.isMasked(index) && !detectorInfo.isMonitor(index)) {
          solidAngle += solidAngleCalculator->solidAngle(index);
        }
      }
      outputWS->mutableY(j)[0] = solidAngle;
    } else {
      ++failCount;
    }
    prog.report();
    PARALLEL_END_INTERRUPT_REGION
  } // loop over spectra
  PARALLEL_CHECK_INTERRUPT_REGION
  g_log.warning() << "min max total" << m_MinSpec << " " << m_MaxSpec << " " << numberOfSpectra;

  auto &outputSpectrumInfo = outputWS->mutableSpectrumInfo();
  // Loop over the histograms (detector spectra)
  for (int j = 0; j < m_MinSpec; ++j) {
    initSpectrum(*inputWS, *outputWS, j);
    // SpectrumInfo::setMasked is NOT threadsafe.
    outputSpectrumInfo.setMasked(j, true);
    prog.report();
  } // loop over spectra

  // Loop over the histograms (detector spectra)
  for (int j = m_MaxSpec + 1; j < numberOfSpectra; ++j) {
    initSpectrum(*inputWS, *outputWS, j);
    // SpectrumInfo::setMasked is NOT threadsafe.
    outputSpectrumInfo.setMasked(j, true);
    prog.report();
  } // loop over spectra

  if (failCount != 0) {
    g_log.information() << "Unable to calculate solid angle for " << failCount
                        << " spectra. The solid angle will be set to zero for "
                           "those detectors.\n";
  }
}

/**
 * SolidAngle::initSpectrum Sets the default value for the spectra for which
 * solid angle is not calculated.
 */
void SolidAngle::initSpectrum(const MatrixWorkspace &inputWS, MatrixWorkspace &outputWS, const size_t wsIndex) {
  outputWS.mutableX(wsIndex)[0] = inputWS.x(wsIndex).front();
  outputWS.mutableX(wsIndex)[1] = inputWS.x(wsIndex).back();
  outputWS.mutableE(wsIndex) = 0.;
  outputWS.mutableY(wsIndex) = 0.; // default value for not calculated
}

} // namespace Mantid::Algorithms
