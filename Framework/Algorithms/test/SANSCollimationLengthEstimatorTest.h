// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_SANSCOLLIMATIONLENGTHESTIMATORTEST_H
#define MANTID_ALGORITHMS_SANSCOLLIMATIONLENGTHESTIMATORTEST_H
#include "MantidAlgorithms/AddSampleLog.h"
#include "MantidAlgorithms/SANSCollimationLengthEstimator.h"
#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/LogFilter.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::Algorithms;
using Mantid::Kernel::V3D;
using namespace Mantid::Geometry;

namespace {

// don't care about Y values, just use 1.0 everywhere
struct ones {
  double operator()(const double, size_t) { return 1.0; }
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
  Detector *det0(nullptr);
  if (!detShapeXML.empty()) {
    auto shape = ShapeFactory().createShape(detShapeXML);
    det0 = new Detector("det0", id, shape, nullptr);
  } else {
    det0 = new Detector("det0", id, nullptr);
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
  Mantid::Types::Core::DateAndTime startTime("2010-01-01T00:10:00");
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
    std::string methodType = "", double collimationLengthCorrection = 20,
    double collimationLengthIncrement = 2, double guideCutoff = 130,
    double numberOfGuides = 5, V3D sourcePosition = V3D(0.0, 0.0, -25.0),
    V3D samplePosition = V3D(0.0, 0.0, 0.0),
    std::vector<double> guideLogDetails = std::vector<double>()) {
  auto ws2d = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
      ones(), static_cast<int>(nhist), x0, x1, dx);

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
} // namespace

class SANSCollimationLengthEstimatorTest : public CxxTest::TestSuite {
public:
  void
  test_that_collimation_length_is_provided_for_simple_instrument_without_guides() {
    // Arrange
    double collimationLengthCorrection = 20;
    double collimationLengthIncrement = -1;
    double guideCutoff = -1;
    double numberOfGuides = -1;
    V3D sourcePosition = V3D(0.0, 0.0, -25.0);
    V3D samplePosition = V3D(0.0, 0.0, 0.0);

    auto testWorkspace =
        createTestWorkspace(10, 0, 10, 0.1, "", collimationLengthCorrection,
                            collimationLengthIncrement, guideCutoff,
                            numberOfGuides, sourcePosition, samplePosition);
    auto collimationLengthEstimator = SANSCollimationLengthEstimator();

    // Act
    auto length =
        collimationLengthEstimator.provideCollimationLength(testWorkspace);

    // Assert
    auto expectedCollimationLength =
        V3D(sourcePosition - samplePosition).norm() -
        collimationLengthCorrection;
    TSM_ASSERT_EQUALS("Should produce a length of 5.", length,
                      expectedCollimationLength);
  }

  void
  test_that_default_value_of_4_is_produced_if_collimation_length_is_not_specified() {
    // Arrange
    double collimationLengthCorrection = -1;
    double collimationLengthIncrement = -1;
    double guideCutoff = -1;
    double numberOfGuides = -1;
    V3D sourcePosition = V3D(0.0, 0.0, -25.0);
    V3D samplePosition = V3D(0.0, 0.0, 0.0);

    auto testWorkspace =
        createTestWorkspace(10, 0, 10, 0.1, "", collimationLengthCorrection,
                            collimationLengthIncrement, guideCutoff,
                            numberOfGuides, sourcePosition, samplePosition);
    auto collimationLengthEstimator = SANSCollimationLengthEstimator();
    // Act
    auto length =
        collimationLengthEstimator.provideCollimationLength(testWorkspace);
    // Assert
    // Note that the default length of 4 was requested by RKH.
    TSM_ASSERT_EQUALS("Should produce a default length of 4", length, 4);
  }

  void test_that_invalid_collimation_method_throws_an_error() {
    // Arrange
    double collimationLengthCorrection = 20;
    double collimationLengthIncrement = -1;
    std::string collimationMethod = "undefined_method";
    double guideCutoff = -1;
    double numberOfGuides = -1;
    V3D sourcePosition = V3D(0.0, 0.0, -25.0);
    V3D samplePosition = V3D(0.0, 0.0, 0.0);

    auto testWorkspace = createTestWorkspace(
        10, 0, 10, 0.1, collimationMethod, collimationLengthCorrection,
        collimationLengthIncrement, guideCutoff, numberOfGuides, sourcePosition,
        samplePosition);
    auto collimationLengthEstimator = SANSCollimationLengthEstimator();
    // Act + Assert
    TSM_ASSERT_THROWS(
        "Should throw an exception since we don't have the requested method "
        "implemneted",
        collimationLengthEstimator.provideCollimationLength(testWorkspace),
        const std::invalid_argument &);
  }

  void test_that_missing_guide_cutoff_produces_a_default_value() {
    // Arrange
    double collimationLengthCorrection = 20;
    double collimationLengthIncrement = 12;
    std::string collimationMethod = "guide";
    double guideCutoff = -1;
    double numberOfGuides = 5;
    V3D sourcePosition = V3D(0.0, 0.0, -25.0);
    V3D samplePosition = V3D(0.0, 0.0, 0.0);

    auto testWorkspace = createTestWorkspace(
        10, 0, 10, 0.1, collimationMethod, collimationLengthCorrection,
        collimationLengthIncrement, guideCutoff, numberOfGuides, sourcePosition,
        samplePosition);
    auto collimationLengthEstimator = SANSCollimationLengthEstimator();
    // Act + Assert
    TSM_ASSERT_EQUALS(
        "Should produce a fallback value of 25-20=5 since the guide cutoffs "
        "are missing",
        collimationLengthEstimator.provideCollimationLength(testWorkspace),
        5.0);
  }

  void test_that_missing_number_of_guides_produces_a_default_value() {
    // Arrange
    double collimationLengthCorrection = 20;
    double collimationLengthIncrement = 12;
    std::string collimationMethod = "guide";
    double guideCutoff = 123;
    double numberOfGuides = -1;
    V3D sourcePosition = V3D(0.0, 0.0, -25.0);
    V3D samplePosition = V3D(0.0, 0.0, 0.0);

    auto testWorkspace = createTestWorkspace(
        10, 0, 10, 0.1, collimationMethod, collimationLengthCorrection,
        collimationLengthIncrement, guideCutoff, numberOfGuides, sourcePosition,
        samplePosition);
    auto collimationLengthEstimator = SANSCollimationLengthEstimator();
    // Act + Assert
    TSM_ASSERT_EQUALS(
        "Should produce a fallback value of 25-20=5 since the number of guides "
        "spec is missing",
        collimationLengthEstimator.provideCollimationLength(testWorkspace),
        5.0);
  }

  void
  test_that_missing_collimation_length_increment_produces_a_default_value() {
    // Arrange
    double collimationLengthCorrection = 20;
    double collimationLengthIncrement = -1;
    std::string collimationMethod = "guide";
    double guideCutoff = 123;
    double numberOfGuides = 12;
    V3D sourcePosition = V3D(0.0, 0.0, -25.0);
    V3D samplePosition = V3D(0.0, 0.0, 0.0);

    auto testWorkspace = createTestWorkspace(
        10, 0, 10, 0.1, collimationMethod, collimationLengthCorrection,
        collimationLengthIncrement, guideCutoff, numberOfGuides, sourcePosition,
        samplePosition);
    auto collimationLengthEstimator = SANSCollimationLengthEstimator();
    // Act + Assert
    TSM_ASSERT_EQUALS(
        "Should produce a fallback value of 25-20=5 since the collimation "
        "length increment is missing.",
        collimationLengthEstimator.provideCollimationLength(testWorkspace),
        5.0);
  }

  void
  test_that_mismatch_of_log_guides_with_specified_number_of_guides_throws() {
    // Arrange
    double collimationLengthCorrection = 20;
    double collimationLengthIncrement = 2;
    std::string collimationMethod = "guide";
    double guideCutoff = 123;
    double numberOfGuides = 12;
    V3D sourcePosition = V3D(0.0, 0.0, -25.0);
    V3D samplePosition = V3D(0.0, 0.0, 0.0);

    auto testWorkspace = createTestWorkspace(
        10, 0, 10, 0.1, collimationMethod, collimationLengthCorrection,
        collimationLengthIncrement, guideCutoff, numberOfGuides, sourcePosition,
        samplePosition);
    auto collimationLengthEstimator = SANSCollimationLengthEstimator();
    // Act + Assert
    TSM_ASSERT_EQUALS(
        "Should produce a fallback value of 25-20=5 since there is a mismatch "
        "between the number of guides in the log and in the spec",
        collimationLengthEstimator.provideCollimationLength(testWorkspace),
        5.0);
  }

  void test_that_5_log_guides_are_all_picked_up_and_contribute() {
    // Arrange
    double collimationLengthCorrection = 20;
    double collimationLengthIncrement = 2;
    std::string collimationMethod = "guide";
    double guideCutoff = 130;
    double numberOfGuides = 5;
    V3D sourcePosition = V3D(0.0, 0.0, -25.0);
    V3D samplePosition = V3D(0.0, 0.0, 0.0);

    std::vector<double> guideLogDetails;
    guideLogDetails.push_back(guideCutoff + 10);
    guideLogDetails.push_back(guideCutoff - 10);
    guideLogDetails.push_back(guideCutoff + 10);
    guideLogDetails.push_back(guideCutoff - 10);
    guideLogDetails.push_back(guideCutoff + 10);

    auto testWorkspace = createTestWorkspace(
        10, 0, 10, 0.1, collimationMethod, collimationLengthCorrection,
        collimationLengthIncrement, guideCutoff, numberOfGuides, sourcePosition,
        samplePosition, guideLogDetails);
    auto collimationLengthEstimator = SANSCollimationLengthEstimator();
    // Act
    auto length =
        collimationLengthEstimator.provideCollimationLength(testWorkspace);
    // Assert
    auto expectedCollimationLength =
        V3D(sourcePosition - samplePosition).norm() -
        collimationLengthCorrection +
        static_cast<double>(guideLogDetails.size()) *
            collimationLengthIncrement;

    TSM_ASSERT_EQUALS("Should have a collimation length of 5+2*5", length,
                      expectedCollimationLength);
  }

  void test_that_only_3_log_guides_are_all_picked_up_and_contribute() {
    // Arrange
    double collimationLengthCorrection = 20;
    double collimationLengthIncrement = 2;
    std::string collimationMethod = "guide";
    double guideCutoff = 130;
    double numberOfGuides = 5;
    V3D sourcePosition = V3D(0.0, 0.0, -25.0);
    V3D samplePosition = V3D(0.0, 0.0, 0.0);

    std::vector<double> guideLogDetails;
    guideLogDetails.push_back(guideCutoff - 10); // Guide 1 -- Is flipped here
    guideLogDetails.push_back(guideCutoff + 10); // Guide 2 -- Is flipped here
    guideLogDetails.push_back(guideCutoff + 10); // Guide 3
    guideLogDetails.push_back(guideCutoff - 10); // Guide 4
    guideLogDetails.push_back(guideCutoff + 10); // Guide 5

    auto testWorkspace = createTestWorkspace(
        10, 0, 10, 0.1, collimationMethod, collimationLengthCorrection,
        collimationLengthIncrement, guideCutoff, numberOfGuides, sourcePosition,
        samplePosition, guideLogDetails);
    auto collimationLengthEstimator = SANSCollimationLengthEstimator();
    // Act
    auto length =
        collimationLengthEstimator.provideCollimationLength(testWorkspace);
    // Assert
    auto expectedCollimationLength =
        V3D(sourcePosition - samplePosition).norm() -
        collimationLengthCorrection + 3 * collimationLengthIncrement;
    TSM_ASSERT_EQUALS("Should have a collimation length of 5+2*3", length,
                      expectedCollimationLength);
  }

  void test_that_only_1_log_guides_is_picked_up_and_contributes() {
    // Arrange
    double collimationLengthCorrection = 20;
    double collimationLengthIncrement = 2;
    std::string collimationMethod = "guide";
    double guideCutoff = 130;
    double numberOfGuides = 5;
    V3D sourcePosition = V3D(0.0, 0.0, -25.0);
    V3D samplePosition = V3D(0.0, 0.0, 0.0);

    std::vector<double> guideLogDetails;
    guideLogDetails.push_back(guideCutoff - 10); // Guide 1 -- Is flipped here
    guideLogDetails.push_back(guideCutoff + 10); // Guide 2 -- Is flipped here
    guideLogDetails.push_back(guideCutoff - 10); // Guide 3 -- Is flipped here
    guideLogDetails.push_back(guideCutoff + 10); // Guide 4 -- Is flipped here
    guideLogDetails.push_back(guideCutoff + 10); // Guide 5

    auto testWorkspace = createTestWorkspace(
        10, 0, 10, 0.1, collimationMethod, collimationLengthCorrection,
        collimationLengthIncrement, guideCutoff, numberOfGuides, sourcePosition,
        samplePosition, guideLogDetails);
    auto collimationLengthEstimator = SANSCollimationLengthEstimator();
    // Act
    auto length =
        collimationLengthEstimator.provideCollimationLength(testWorkspace);
    // Assert
    auto expectedCollimationLength =
        V3D(sourcePosition - samplePosition).norm() -
        collimationLengthCorrection + 1 * collimationLengthIncrement;
    TSM_ASSERT_EQUALS("Should have a collimation length of 5+2*3", length,
                      expectedCollimationLength);
  }
};
#endif
