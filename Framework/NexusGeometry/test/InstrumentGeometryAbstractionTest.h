#ifndef INSTRUMENT_GEOMETRY_ABSTRACTION_TEST_H_
#define INSTRUMENT_GEOMETRY_ABSTRACTION_TEST_H_

//---------------------------
// Includes
//---------------------------

#include <cxxtest/TestSuite.h>

#include "MantidBeamline/ComponentInfo.h"
#include "MantidBeamline/DetectorInfo.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidNexusGeometry/InstrumentGeometryAbstraction.h"

#include "Eigen/Core"

#include <string>

using namespace Mantid;
using namespace NexusGeometry;

class InstrumentGeometryAbstractionTest : public CxxTest::TestSuite {
public:
    void testConstructor(){
        TS_ASSERT_THROWS_NOTHING(InstrumentGeometryAbstraction iGeoAbstract(this->iTestName));
    }
private:
    std::string iTestName = "testInstrument";
};

#endif// INSTRUMENT_GEOMETRY_ABSTRACTION_TEST_H_
