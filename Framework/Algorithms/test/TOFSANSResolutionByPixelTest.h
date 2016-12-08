#ifndef TOFSANSRESOLUTIONBYPIXELTEST_H_
#define TOFSANSRESOLUTIONBYPIXELTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/TOFSANSResolutionByPixel.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include "boost/shared_ptr.hpp"
#include <stdexcept>

using namespace Mantid::Algorithms;
using Mantid::Kernel::V3D;
using namespace Mantid::Geometry;
using namespace Mantid::API;

namespace {

// don't care about Y values, just use 1.0 everywhere
struct ones {
  double operator()(const double, size_t) { return 1.0; }
};

struct twos {
  double operator()(const double, size_t) { return 2.0; }
};

/**
 * Create a test instrument
 */
Mantid::Geometry::Instrument_sptr
createTestInstrument(const Mantid::detid_t id,
                     const Mantid::Kernel::V3D &detPos,
                     const std::string &detShapeXML = "",
                     V3D sourcePosition = V3D(0.0, 0.0, -25.0),
                     V3D samplePosition = V3D(0.0, 0.0, 0.0)) {

  // Requires an instrument.
  auto inst = boost::make_shared<Instrument>();
  inst->setName("TestName");

  // Source/sample
  auto *source = new ObjComponent("source");
  source->setPos(sourcePosition);
  inst->add(source);
  inst->markAsSource(source);
  auto *sampleHolder = new ObjComponent("samplePos");
  sampleHolder->setPos(samplePosition);
  inst->add(sampleHolder);
  inst->markAsSamplePos(sampleHolder);

  // Just give it a single detector
  Detector *det0(NULL);
  if (!detShapeXML.empty()) {
    auto shape = ShapeFactory().createShape(detShapeXML);
    det0 = new Detector("det0", id, shape, NULL);
  } else {
    det0 = new Detector("det0", id, NULL);
  }
  det0->setPos(detPos);
  inst->add(det0);
  inst->markAsDetector(det0);

  return inst;
}

/**
 * Set the instrument parameters
 */
void setInstrumentParametersForTOFSANS(
    const Mantid::API::MatrixWorkspace_sptr ws, std::string methodType = "",
    double collimationLengthCorrection = 20,
    double collimationLengthIncrement = 2, double guideCutoff = 130,
    double numberOfGuides = 5) {
  auto &pmap = ws->instrumentParameters();
  auto instrumentId = ws->getInstrument()->getComponentID();

  // Add the parameters
  if (collimationLengthCorrection > 0) {
    pmap.addDouble(instrumentId, "collimation-length-correction",
                   collimationLengthCorrection);
  }

  if (!methodType.empty()) {
    pmap.addString(instrumentId, "special-default-collimation-length-method",
                   methodType);
  }

  if (collimationLengthIncrement > 0) {
    pmap.addDouble(instrumentId, "guide-collimation-length-increment",
                   collimationLengthIncrement);
  }

  if (guideCutoff > 0) {
    pmap.addDouble(instrumentId, "guide-cutoff", guideCutoff);
  }

  if (numberOfGuides > 0) {
    pmap.addDouble(instrumentId, "number-of-guides", numberOfGuides);
  }
}

/*
 * Add a timer series sample log
 */
void addSampleLog(Mantid::API::MatrixWorkspace_sptr workspace,
                  std::string sampleLogName, double value,
                  unsigned int length) {
  auto timeSeries =
      new Mantid::Kernel::TimeSeriesProperty<double>(sampleLogName);
  Mantid::Kernel::DateAndTime startTime("2010-01-01T00:10:00");
  timeSeries->setUnits("mm");
  for (unsigned int i = 0; i < length; i++) {
    timeSeries->addValue(startTime + static_cast<double>(i), value);
  }
  workspace->mutableRun().addProperty(timeSeries, true);
}

/*
 *Create a test workspace with instrument and instrument parameters
 */
Mantid::API::MatrixWorkspace_sptr createTestWorkspace(
    const size_t nhist, const double x0, const double x1, const double dx,
    std::string methodType = "", bool isModerator = false,
    double collimationLengthCorrection = 20,
    double collimationLengthIncrement = 2, double guideCutoff = 130,
    double numberOfGuides = 5, V3D sourcePosition = V3D(0.0, 0.0, -25.0),
    V3D samplePosition = V3D(0.0, 0.0, 0.0),
    std::vector<double> guideLogDetails = std::vector<double>()) {
  Mantid::API::MatrixWorkspace_sptr ws2d;
  if (isModerator) {
    ws2d = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        twos(), static_cast<int>(nhist), x0, x1, dx, true);
  } else {
    ws2d = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        ones(), static_cast<int>(nhist), x0, x1, dx, true);
  }

  // Set the units to Wavelength (is needed for both types of worspaces)
  ws2d->getAxis(0)->setUnit("Wavelength");

  // Add the instrument with a single detector
  Mantid::detid_t id(1);
  double r(0.55), theta(66.5993), phi(0.0);
  Mantid::Kernel::V3D detPos;
  detPos.spherical_rad(r, theta * M_PI / 180.0, phi * M_PI / 180.0);
  ws2d->setInstrument(
      createTestInstrument(id, detPos, "", sourcePosition, samplePosition));

  // Set the instrument parameters
  setInstrumentParametersForTOFSANS(
      ws2d, methodType, collimationLengthCorrection, collimationLengthIncrement,
      guideCutoff, numberOfGuides);

  // Add sample log details
  if (!guideLogDetails.empty()) {
    auto numberOfLogs = static_cast<unsigned int>(guideLogDetails.size());
    for (unsigned i = 0; i < numberOfLogs; ++i) {
      auto logName = "Guide" + boost::lexical_cast<std::string>(
                                   i + 1); // Names are Guide1, Guide2, ...
      addSampleLog(ws2d, logName, guideLogDetails[i], numberOfLogs);
    }
  }

  // Link workspace with detector
  for (size_t i = 0; i < nhist; ++i) {
    const Mantid::specnum_t specID = static_cast<Mantid::specnum_t>(id + i);
    auto &spec = ws2d->getSpectrum(i);
    spec.setSpectrumNo(specID);
    spec.clearDetectorIDs();
    spec.addDetectorID(id);
  }
  return ws2d;
}
}

class TOFSANSResolutionByPixelTest : public CxxTest::TestSuite {
public:
  void testName() { TS_ASSERT_EQUALS(alg.name(), "TOFSANSResolutionByPixel") }

  void testCategory() { TS_ASSERT_EQUALS(alg.category(), "SANS") }

  void testInit() {
    alg.initialize();
    TS_ASSERT(alg.isInitialized())
  }

  void test_that_correct_resolution_is_calculated_without_gravity() {

    double collimationLengthCorrection = -1;
    double collimationLengthIncrement = -1;
    double guideCutoff = -1;
    double numberOfGuides = -1;
    V3D sourcePosition = V3D(0.0, 0.0, -25.0);
    V3D samplePosition = V3D(0.0, 0.0, 0.0);

    auto testWorkspace =
        createTestWorkspace(1, 0, 3, 1, "", false, collimationLengthCorrection,
                            collimationLengthIncrement, guideCutoff,
                            numberOfGuides, sourcePosition, samplePosition);

    auto sigmaModerator =
        createTestWorkspace(1, 0, 3, 1, "", true, collimationLengthCorrection,
                            collimationLengthIncrement, guideCutoff,
                            numberOfGuides, sourcePosition, samplePosition);

    const double deltaR = 1;
    const double sampleApertureRadius = 1;
    const double sourceApertureRadius = 1;

    TOFSANSResolutionByPixel alg;
    std::string outputWS("test");
    alg.initialize();

    alg.setProperty("InputWorkspace", testWorkspace);
    alg.setPropertyValue("OutputWorkspace", outputWS);
    alg.setProperty("DeltaR", deltaR);
    alg.setProperty("SampleApertureRadius", sampleApertureRadius);
    alg.setProperty("SourceApertureRadius", sourceApertureRadius);
    alg.setProperty("SigmaModerator", sigmaModerator);

    // Act
    alg.execute();

    // Assert
    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)));

    const auto &xOUT = result->x(0);
    const auto &xIN = testWorkspace->x(0);

    TSM_ASSERT_EQUALS("Output should have the same binning as the input.",
                      xOUT.size(), xIN.size());

    // Clean up
    AnalysisDataService::Instance().remove(outputWS);
  }

private:
  TOFSANSResolutionByPixel alg;
};
#endif /*TOFSANSRESOLUTIONBYPIXELTEST_H_*/
