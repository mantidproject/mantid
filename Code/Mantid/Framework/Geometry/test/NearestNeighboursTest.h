#ifndef MANTID_TEST_GEOMETRY_NEARESTNEIGHBOURS
#define MANTID_TEST_GEOMETRY_NEARESTNEIGHBOURS

#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/IInstrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidGeometry/Instrument/NearestNeighbours.h"
#include "MantidGeometry/Instrument/OneToOneSpectraDetectorMap.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <map>

using namespace Mantid;
using namespace Mantid::Geometry;
using Mantid::Kernel::V3D;

/**
* Everything must be in one test or the instrument/detector list goes AWOL.
*/

class NearestNeighboursTest : public CxxTest::TestSuite
{
public:
  void testNeighbourFinding()
  {
    // Create Instrument and make it Parameterised
    Instrument_sptr instrument = boost::dynamic_pointer_cast<Instrument>(ComponentCreationHelper::createTestInstrumentCylindrical(2));
    boost::scoped_ptr<ISpectraDetectorMap> spectramap(new OneToOneSpectraDetectorMap(1, 18));
    TS_ASSERT_EQUALS(spectramap->nElements(), 18);
    // Default parameter map.
    ParameterMap_sptr pmap(new ParameterMap());
    // Parameterized instrument
    Instrument_sptr m_instrument(new Instrument(instrument, pmap));

    // Create the NearestNeighbours object directly.
    NearestNeighbours nn(m_instrument, *spectramap);

    detid2det_map m_detectors;
    m_instrument->getDetectors(m_detectors);

    // Need scaling vector since changes to NN ( 22/12/10 )
    Mantid::Geometry::BoundingBox bbox = Mantid::Geometry::BoundingBox();
    boost::shared_ptr<Detector> det = boost::dynamic_pointer_cast<Detector>(m_detectors[3]);
    det->getBoundingBox(bbox);
    V3D scale((bbox.xMax()-bbox.xMin()), (bbox.yMax()-bbox.yMin()), (bbox.zMax()-bbox.zMin()) );


    // Check instrument was created to our expectations
    ParameterMap_sptr p_map;
    TS_ASSERT_THROWS_NOTHING(p_map = m_instrument->getParameterMap());
    TS_ASSERT_EQUALS(m_detectors.size(), 18);

    // Check distances calculated in NearestNeighbours compare with those using getDistance on component
    std::map<specid_t, double> distances = nn.neighbours(5);
    std::map<specid_t, double>::iterator distIt;

    // We should have 8 neighbours when not specifying a range.
    TS_ASSERT_EQUALS(distances.size(), 8);

    for ( distIt = distances.begin(); distIt != distances.end(); ++distIt )
    {
      double nnDist = distIt->second;
      V3D delta = m_detectors[distIt->first]->getPos() - m_detectors[5]->getPos();
      double gmDist = delta.norm();
      TS_ASSERT_DELTA(nnDist, gmDist, 1e-12);
    }

    // Check that the 'radius' option works as expected
    // Lower radius
    distances = nn.neighbours(14, 0.008);
    TS_ASSERT_EQUALS(distances.size(), 4);

    // Higher than currently computed
    distances = nn.neighbours(14, 6.0);
    TS_ASSERT_EQUALS(distances.size(), 17);

    
  }
  
  // Let's try it with a rectangular detector.
  void testNeighbours_RectangularDetector()
  {
    // 2 Rectangular detectors, 16x16
    Instrument_sptr instrument = boost::dynamic_pointer_cast<Instrument>(ComponentCreationHelper::createTestInstrumentRectangular(2, 16) );

    // Test fails without a parameter map.
    boost::scoped_ptr<ISpectraDetectorMap> spectramap(new OneToOneSpectraDetectorMap(256, 767));
    // Default parameter map.
    ParameterMap_sptr pmap(new ParameterMap());
    // Parameterized instrument
    Instrument_sptr m_instrument(new Instrument(instrument, pmap));

    // Create the NearestNeighbours object directly.
    NearestNeighbours nn(m_instrument, *spectramap);

    // Correct # of detectors
    TS_ASSERT_EQUALS( m_instrument->getDetectorIDs().size(), 512);


    RectangularDetector_sptr bank1 = boost::dynamic_pointer_cast<RectangularDetector>(m_instrument->getComponentByName("bank1"));
    boost::shared_ptr<Detector> det = bank1->getAtXY(2,3);
    TS_ASSERT( det );
    std::map<specid_t, double> nb;

    // Too close!
    specid_t spec = 256 + 2*16+3; // This gives the spectrum number for this detector
    nb = nn.neighbours(spec, 0.003);
    TS_ASSERT_EQUALS( nb.size(), 0 );

    // The ones above below and next to it
    nb = nn.neighbours(spec, 0.016);
    TS_ASSERT_EQUALS( nb.size(), 4 );

  }

};

#endif /* MANTID_TEST_GEOMETRY_NEARESTNEIGHBOURS */
