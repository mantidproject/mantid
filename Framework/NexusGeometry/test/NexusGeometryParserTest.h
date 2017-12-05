//----------------
// Includes
//----------------

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ConfigService.h"
#include "MantidNexusGeometry/NexusGeometryParser.h"

#include <string>

using namespace Mantid;
using namespace NexusGeometry;

class NexusGeometryParserTest : public CxxTest::TestSuite {
public:
  void testParseNexusGeometry() {
    // Initialise abstract instrument builder
    iAbstractBuilder_sptr iAbsBuilder_sptr(
        new iAbstractBuilder(this->testInstName));
    this->iAbsBuilder_sptr = iAbsBuilder_sptr;
    // Initialise the parser
    auto fullpath = Kernel::ConfigService::Instance().getFullPath(
        this->nexusFilename, true, Poco::Glob::GLOB_DEFAULT);

    NexusGeometryParser parser(fullpath, this->iAbsBuilder_sptr);
    // Parse the nexus file
    this->exitStatus = parser.ParseNexusGeometry();
    TS_ASSERT(this->exitStatus == NO_ERROR);

    // Check correct number of detectors, skip monitors
    TS_ASSERT(
        this->iAbsBuilder_sptr->_unAbstractInstrument()->getNumberDetectors(
            true) == 256);
    // Check correct position of two separate monitors (one from each detector
    // panel)
    auto detector1 =
        this->iAbsBuilder_sptr->_unAbstractInstrument()->getDetector(2100000);
    auto detector2 =
        this->iAbsBuilder_sptr->_unAbstractInstrument()->getDetector(1100000);

    auto pos1 = detector1->getPos();
    auto pos2 = detector2->getPos();
    auto pos1PreCalc = Eigen::Vector3d(0.619826, -0.200000, 23.413000);
    auto pos2PreCalc = Eigen::Vector3d(-0.498000, -0.200000, 23.281000);
    for (int i = 0; i < 3; ++i) {
      TS_ASSERT_DELTA(pos1[i], pos1PreCalc[i], 0.0001)
      TS_ASSERT_DELTA(pos2[i], pos2PreCalc[i], 0.0001)
    }
  }

private:
  std::string testInstName = "testInstrument";
  H5std_string nexusFilename = "SMALLFAKE_example_geometry.hdf5";
  iAbstractBuilder_sptr iAbsBuilder_sptr;
  ParsingErrors exitStatus;
};
