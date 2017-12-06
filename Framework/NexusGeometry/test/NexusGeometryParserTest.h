//----------------
// Includes
//----------------

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ConfigService.h"
#include "MantidNexusGeometry/NexusGeometryParser.h"

#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include <string>

using namespace Mantid;
using namespace NexusGeometry;

class NexusGeometryParserTest : public CxxTest::TestSuite {
public:
  Geometry::Instrument_sptr extractInstrument() {
    // Initialise abstract instrument builder
    iAbstractBuilder_sptr iAbsBuilder_sptr(
        new iAbstractBuilder(this->testInstName));
    this->iAbsBuilder_sptr = iAbsBuilder_sptr;
    // Initialise the parser
    auto fullpath = Kernel::ConfigService::Instance().getFullPath(
        this->nexusFilename, true, Poco::Glob::GLOB_DEFAULT);

    NexusGeometryParser parser(fullpath, this->iAbsBuilder_sptr);
    // Parse the nexus file
    this->exitStatus = parser.parseNexusGeometry();
    TS_ASSERT(this->exitStatus == NO_ERROR);

    // Check correct number of detectors, skip monitors
    TS_ASSERT(
        this->iAbsBuilder_sptr->_unAbstractInstrument()->getNumberDetectors(
            true) == 256);

    return this->iAbsBuilder_sptr->_unAbstractInstrument();
  }

  std::unique_ptr<Geometry::DetectorInfo> extractDetectorInfo() {
    auto instrument = extractInstrument();
    Geometry::ParameterMap pmap;
    return std::move(std::get<1>(instrument->makeBeamline(pmap)));
  }

  std::pair<std::unique_ptr<Geometry::ComponentInfo>,
            std::unique_ptr<Geometry::DetectorInfo>>
  extractBeamline() {
    auto instrument = extractInstrument();
    Geometry::ParameterMap pmap;
    return {std::move(std::get<0>(instrument->makeBeamline(pmap))),
            std::move(std::get<1>(instrument->makeBeamline(pmap)))};
  }

  void test_basic_instrument_information() {
    auto beamline = extractBeamline();
    auto componentInfo = std::move(beamline.first);
    auto detectorInfo = std::move(beamline.second);
    TSM_ASSERT_EQUALS(
        "Detectors + 1 monitor", detectorInfo->size(),
        128 * 2 + 1); // TODO, currently NOT counting monitor. Needs fixing.
    TSM_ASSERT_EQUALS(
        "Detectors + 3 non-detector components", componentInfo->size(),
        detectorInfo->size() +
            3); // TODO, currently NOT counting source + sample + root!
  }

  void test_simple_translation() {
    auto detectorInfo = extractDetectorInfo();
    // First pixel in bank 2
    auto det0Postion = Kernel::toVector3d(
        detectorInfo->position(detectorInfo->indexOf(1100000)));
    Eigen::Vector3d unitVector(0, 0,
                               1); // Fixed in file location vector attribute
    const double magnitude = 4.0;  // Fixed in file location value
    Eigen::Vector3d offset(-0.498, -0.200, 0.00); // All offsets for pixel x,
                                                  // and y specified separately,
                                                  // z defaults to 0
    auto expectedPosition = offset + (magnitude * unitVector);
    TS_ASSERT(det0Postion.isApprox(expectedPosition));
  }

  void test_complex_translation() {

    auto detectorInfo = extractDetectorInfo();
    // First pixel in bank 1

    auto det0Postion = Kernel::toVector3d(
        detectorInfo->position(detectorInfo->indexOf(2100000)));
    Eigen::Vector3d unitVectorTranslation(
        0.2651564830210424, 0.0,
        0.9642053928037905);         // Fixed in file location vector attribute
    const double magnitude = 4.148;  // Fixed in file location value
    const double rotation = -15.376; // Fixed in file orientation value
    Eigen::Vector3d rotationAxis(
        0, -1, 0); // Fixed in file orientation vector attribute
    Eigen::Vector3d offset(-0.498, -0.200, 0.00); // All offsets for pixel x,
                                                  // and y specified separately,
                                                  // z defaults to 0
    auto affine = Eigen::Transform<double, 3, Eigen::Affine>::Identity();
    // Tranlsation of bank
    affine *= Eigen::Translation3d(magnitude * unitVectorTranslation);
    // Rotation of bank
    affine *= Eigen::Quaterniond(
        Eigen::AngleAxisd(rotation * M_PI / 180, rotationAxis));
    auto expectedPosition = affine * offset;
    TS_ASSERT(det0Postion.isApprox(expectedPosition, 1e-3));
  }

private:
  std::string testInstName = "testInstrument";
  H5std_string nexusFilename = "SMALLFAKE_example_geometry.hdf5";
  iAbstractBuilder_sptr iAbsBuilder_sptr;
  ParsingErrors exitStatus;
};
