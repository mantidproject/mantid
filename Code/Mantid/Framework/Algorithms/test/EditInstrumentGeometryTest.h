#ifndef MANTID_ALGORITHMS_EDITTOFPOWDERDIFFRACTOMERGEOMETRYTEST_H_
#define MANTID_ALGORITHMS_EDITTOFPOWDERDIFFRACTOMERGEOMETRYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/EditInstrumentGeometry.h"
#include "MantidDataHandling/LoadNexusProcessed.h"
#include "MantidGeometry/Instrument.h"
#include "MantidAPI/ISpectrum.h"
#include "MantidGeometry/IDetector.h"

#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>


using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

class EditInstrumentGeometryTest : public CxxTest::TestSuite
{
public:

  void test_Initialize(){

    EditInstrumentGeometry editdetector;
    TS_ASSERT_THROWS_NOTHING(editdetector.initialize());
    TS_ASSERT(editdetector.isInitialized());

  }

  void test_SingleSpectrum()
  {
    // 1. Init
    EditInstrumentGeometry editdetector;
    editdetector.initialize();

    // 2. Load Data
    Mantid::DataHandling::LoadNexusProcessed loader;
    loader.initialize();
    loader.setProperty("Filename","PG3_2583.nxs");
    const std::string inputWS = "inputWS";
    loader.setPropertyValue("OutputWorkspace",inputWS);
    loader.execute();

    // 3. Set Property
    TS_ASSERT_THROWS_NOTHING( editdetector.setPropertyValue("Workspace", inputWS) );
    TS_ASSERT_THROWS_NOTHING( editdetector.setPropertyValue("SpectrumIDs","1") );
    TS_ASSERT_THROWS_NOTHING( editdetector.setPropertyValue("L2","3.45") );
    TS_ASSERT_THROWS_NOTHING( editdetector.setPropertyValue("Polar","90.09") );
    TS_ASSERT_THROWS_NOTHING( editdetector.setPropertyValue("Azimuthal","1.84") );
    TS_ASSERT_THROWS_NOTHING( editdetector.setProperty("NewInstrument", false) );

    // 4. Run
    TS_ASSERT_THROWS_NOTHING( editdetector.execute() );
    TS_ASSERT( editdetector.isExecuted() );

    // 5. Check result
    Mantid::API::MatrixWorkspace_sptr workspace;
    TS_ASSERT_THROWS_NOTHING( workspace = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
      (Mantid::API::AnalysisDataService::Instance().retrieve(inputWS)) );

    API::ISpectrum* spectrum1 = workspace->getSpectrum(0);
    Geometry::Instrument_const_sptr instrument = workspace->getInstrument();

    std::set<detid_t> detids = spectrum1->getDetectorIDs();
    TS_ASSERT_EQUALS(detids.size(), 1);
    detid_t detid = 0;
    std::set<detid_t>::iterator it;
    for (it = detids.begin(); it != detids.end(); ++it){
      detid = *it;
    }
    Geometry::IDetector_const_sptr detector = instrument->getDetector(detid);
    double r, tth, phi;
    detector->getPos().getSpherical(r, tth, phi);
    TS_ASSERT_DELTA(r, 3.45, 0.000001);
    TS_ASSERT_DELTA(tth, 90.09, 0.000001);
    TS_ASSERT_DELTA(phi, 1.84, 0.000001);

  }


};


#endif /* MANTID_ALGORITHMS_EDITTOFPOWDERDIFFRACTOMERGEOMETRYTEST_H_ */

