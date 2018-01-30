//----------------
// Includes
//----------------

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ConfigService.h"
#include "MantidNexusGeometry/NexusGeometryParser.h"

#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Surfaces/Cylinder.h"
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
    auto beamline = instrument->makeBeamline(pmap);
    return {std::move(std::get<0>(beamline)), std::move(std::get<1>(beamline))};
  }

  void test_basic_instrument_information() {
    auto beamline = extractBeamline();
    auto componentInfo = std::move(beamline.first);
    auto detectorInfo = std::move(beamline.second);
    TSM_ASSERT_EQUALS("Detectors + 1 monitor", detectorInfo->size(),
                      128 * 2 + 1);
    TSM_ASSERT_EQUALS("Detectors + 3 non-detector components",
                      componentInfo->size(), detectorInfo->size() + 3);
  }

  void test_simple_translation() {
    auto beamline = extractBeamline();
    auto componentInfo = std::move(beamline.first);
    auto detectorInfo = std::move(beamline.second);
    // First pixel in bank 2
    auto det0Position = Kernel::toVector3d(
        detectorInfo->position(detectorInfo->indexOf(1100000)));
    Eigen::Vector3d unitVector(0, 0,
                               1); // Fixed in file location vector attribute
    const double magnitude = 4.0;  // Fixed in file location value
    Eigen::Vector3d offset(-0.498, -0.200, 0.00); // All offsets for pixel x,
                                                  // and y specified separately,
                                                  // z defaults to 0
    auto expectedDet0Position = offset + (magnitude * unitVector);
    TS_ASSERT(det0Position.isApprox(expectedDet0Position));

    auto sourcePosition =
        Kernel::toVector3d(componentInfo->position(componentInfo->source()));
    TS_ASSERT(sourcePosition.isApprox(Eigen::Vector3d(0, 0, -34.281)));

    const size_t monitorIndex = 0;
    TS_ASSERT(detectorInfo->isMonitor(monitorIndex));
    TSM_ASSERT("Monitor has no shape",
               !componentInfo->hasValidShape(monitorIndex));
    const auto &det1Shape = componentInfo->shape(1);
    const auto &det2Shape = componentInfo->shape(2);
    TSM_ASSERT_EQUALS("Pixel shapes should be the same within bank", &det1Shape,
                      &det2Shape);
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
    // Rotation of bank
    affine *= Eigen::Quaterniond(
        Eigen::AngleAxisd(rotation * M_PI / 180, rotationAxis));
    // Tranlsation of bank
    affine *= Eigen::Translation3d(magnitude * unitVectorTranslation);
    auto expectedPosition = affine * offset;
    TS_ASSERT(det0Postion.isApprox(expectedPosition, 1e-3));
  }

private:
  std::string testInstName = "testInstrument";
  H5std_string nexusFilename = "SMALLFAKE_example_geometry.hdf5";
  iAbstractBuilder_sptr iAbsBuilder_sptr;
  ParsingErrors exitStatus;
};
