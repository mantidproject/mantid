#ifndef MANTID_TEST_GEOMETRY_NEARESTNEIGHBOURS
#define MANTID_TEST_GEOMETRY_NEARESTNEIGHBOURS

#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/NearestNeighbours.h"
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

//=====================================================================================
// Functional tests
//=====================================================================================
class NearestNeighboursTest : public CxxTest::TestSuite
{
public:
  static ISpectrumDetectorMapping buildSpectrumDetectorMapping(const specid_t start, const specid_t end)
  {
    boost::unordered_map<specid_t, std::set<detid_t>> map;
    for ( specid_t i = start; i <= end; ++i )
    {
      map[i].insert(i);
    }
    return map;
  }

private:

  /// Helper type giving access to protected methods. Makes testing of NN internals possible.
  class ExposedNearestNeighbours : public Mantid::Geometry::NearestNeighbours
  {
  public:
      ExposedNearestNeighbours(boost::shared_ptr<const Instrument> instrument,
        const ISpectrumDetectorMapping & spectraMap, bool ignoreMasked=false) : NearestNeighbours(instrument, spectraMap, ignoreMasked){}

      //Direct access to intermdiate spectra detectors
      std::map<specid_t, IDetector_const_sptr> getSpectraDetectors()
      {
        return NearestNeighbours::getSpectraDetectors(m_instrument, m_spectraMap);
      }
  };

public:

  void doTestWithNeighbourNumbers(int actualNeighboursNumber, int expectedNeighboursNumber)
  {
     // Create Instrument and make it Parameterised
    Instrument_sptr instrument = boost::dynamic_pointer_cast<Instrument>(ComponentCreationHelper::createTestInstrumentCylindrical(2));
    const ISpectrumDetectorMapping spectramap = buildSpectrumDetectorMapping(1, 18);
    TS_ASSERT_EQUALS(spectramap.size(), 18);
    // Default parameter map.
    ParameterMap_sptr pmap(new ParameterMap());
    // Parameterized instrument
    Instrument_sptr m_instrument(new Instrument(instrument, pmap));

    // Create the NearestNeighbours object directly.
    NearestNeighbours nn(actualNeighboursNumber, m_instrument, spectramap);

    // Check distances calculated in NearestNeighbours compare with those using getDistance on component
    std::map<specid_t, V3D> distances = nn.neighbours(14);

    // We should have 8 neighbours when not specifying a range.
    TS_ASSERT_EQUALS(expectedNeighboursNumber, distances.size());
  }

  void testNeighbourFindingWithRadius()
  {
    // Create Instrument and make it Parameterised
    Instrument_sptr instrument = boost::dynamic_pointer_cast<Instrument>(ComponentCreationHelper::createTestInstrumentCylindrical(2));
    const ISpectrumDetectorMapping spectramap = buildSpectrumDetectorMapping(1, 18);
    TS_ASSERT_EQUALS(spectramap.size(), 18);
    // Default parameter map.
    ParameterMap_sptr pmap(new ParameterMap());
    // Parameterized instrument
    Instrument_sptr m_instrument(new Instrument(instrument, pmap));

    // Create the NearestNeighbours object directly.
    NearestNeighbours nn(m_instrument, spectramap);

    detid2det_map m_detectors;
    m_instrument->getDetectors(m_detectors);

    // Need scaling vector since changes to NN ( 22/12/10 )
    Mantid::Geometry::BoundingBox bbox = Mantid::Geometry::BoundingBox();
    boost::shared_ptr<const Detector> det = boost::dynamic_pointer_cast<const Detector>(m_detectors[3]);
    det->getBoundingBox(bbox);
    V3D scale((bbox.xMax()-bbox.xMin()), (bbox.yMax()-bbox.yMin()), (bbox.zMax()-bbox.zMin()) );


    // Check instrument was created to our expectations
    ParameterMap_sptr p_map;
    TS_ASSERT_THROWS_NOTHING(p_map = m_instrument->getParameterMap());
    TS_ASSERT_EQUALS(m_detectors.size(), 18);

    // Check distances calculated in NearestNeighbours compare with those using getDistance on component
    std::map<specid_t, V3D> distances = nn.neighbours(5);
    std::map<specid_t, V3D>::iterator distIt;

    // We should have 8 neighbours when not specifying a range.
    TS_ASSERT_EQUALS(distances.size(), 8);

    for ( distIt = distances.begin(); distIt != distances.end(); ++distIt )
    {
      double nnDist = distIt->second.norm();
      V3D delta = m_detectors[distIt->first]->getPos() - m_detectors[5]->getPos();
      double gmDist = delta.norm();
      TS_ASSERT_DELTA(nnDist, gmDist, 1e-12);
    }

    // Check that the 'radius' option works as expected
    // Lower radius
    distances = nn.neighboursInRadius(14, 0.008);
    TS_ASSERT_EQUALS(distances.size(), 4);

    // Higher than currently computed
    distances = nn.neighboursInRadius(14, 6.0);
    TS_ASSERT_EQUALS(distances.size(), 17);
  }



  void testNeighbourFindingWithNeighbourNumberSpecified()
  {
    doTestWithNeighbourNumbers(1, 1);
    doTestWithNeighbourNumbers(2, 2);
    doTestWithNeighbourNumbers(3, 3);
  }

  
  // Let's try it with a rectangular detector.
  void testNeighbours_RectangularDetector()
  {
    // 2 Rectangular detectors, 16x16
    Instrument_sptr instrument = boost::dynamic_pointer_cast<Instrument>(ComponentCreationHelper::createTestInstrumentRectangular(2, 16) );

    // Test fails without a parameter map.
    const ISpectrumDetectorMapping spectramap = buildSpectrumDetectorMapping(256, 767);
    // Default parameter map.
    ParameterMap_sptr pmap(new ParameterMap());
    // Parameterized instrument
    Instrument_sptr m_instrument(new Instrument(instrument, pmap));

    // Create the NearestNeighbours object directly.
    NearestNeighbours nn(m_instrument, spectramap);

    // Correct # of detectors
    TS_ASSERT_EQUALS( m_instrument->getDetectorIDs().size(), 512);


    RectangularDetector_const_sptr bank1 = boost::dynamic_pointer_cast<const RectangularDetector>(m_instrument->getComponentByName("bank1"));
    boost::shared_ptr<const Detector> det = bank1->getAtXY(2,3);
    TS_ASSERT( det );
    std::map<specid_t, V3D> nb;

    // Too close!
    specid_t spec = 256 + 2*16+3; // This gives the spectrum number for this detector
    nb = nn.neighboursInRadius(spec, 0.003);
    TS_ASSERT_EQUALS( nb.size(), 0 );

    // The ones above below and next to it
    nb = nn.neighboursInRadius(spec, 0.016);
    TS_ASSERT_EQUALS( nb.size(), 4 );

  }

  void testIgnoreAndApplyMasking()
  {
    Instrument_sptr instrument = boost::dynamic_pointer_cast<Instrument>(ComponentCreationHelper::createTestInstrumentCylindrical(2));
    const ISpectrumDetectorMapping spectramap = buildSpectrumDetectorMapping(1, 18);

    // Default parameter map.
    ParameterMap_sptr pmap(new ParameterMap());

    //Mask the first 5 detectors
    for(Mantid::specid_t i = 1; i < 3; i++)
    {
      if ( const Geometry::ComponentID det = instrument->getDetector(*spectramap.at(i).begin())->getComponentID() )
      {
        pmap->addBool(det,"masked",true);
      }
    }

    // Parameterized instrument
    Instrument_sptr m_instrument(new Instrument(instrument, pmap));

    IDetector_const_sptr det = m_instrument->getDetector(*spectramap.at(1).begin());

    // Create the NearestNeighbours object directly. Ignore any masking.
    ExposedNearestNeighbours ignoreMaskedNN(m_instrument, spectramap, true);
    // Create the NearestNeighbours object directly. Account for any masking.
    ExposedNearestNeighbours accountForMaskedNN(m_instrument, spectramap, false);

    size_t sizeWithoutMasking = ignoreMaskedNN.getSpectraDetectors().size(); 
    size_t sizeWithMasking = accountForMaskedNN.getSpectraDetectors().size(); 

    TSM_ASSERT_EQUALS("Without masking should get 18 spectra back", 18, sizeWithoutMasking); 
    TSM_ASSERT("Must have less detectors available after applying masking", sizeWithoutMasking > sizeWithMasking); 
  }

};

//=====================================================================================
// Performance tests
//=====================================================================================
class NearestNeighboursTestPerformance : public CxxTest::TestSuite
{

public:

  void testUsingRadius()
  {
    Instrument_sptr instrument = boost::dynamic_pointer_cast<Instrument>(ComponentCreationHelper::createTestInstrumentCylindrical(2));
    const ISpectrumDetectorMapping spectramap = NearestNeighboursTest::buildSpectrumDetectorMapping(1, 18);
    // Default parameter map.
    ParameterMap_sptr pmap(new ParameterMap());
    // Parameterized instrument
    Instrument_sptr m_instrument(new Instrument(instrument, pmap));

    // Create the NearestNeighbours object directly.
    NearestNeighbours nn(m_instrument, spectramap);
    for(size_t i = 0; i < 2000; i++)
    {
      nn.neighboursInRadius(1, 5.0);
    }
  }

  void testUsingDefault()
  {
    Instrument_sptr instrument = boost::dynamic_pointer_cast<Instrument>(ComponentCreationHelper::createTestInstrumentCylindrical(2));
    const ISpectrumDetectorMapping spectramap = NearestNeighboursTest::buildSpectrumDetectorMapping(1, 18);
    // Default parameter map.
    ParameterMap_sptr pmap(new ParameterMap());
    // Parameterized instrument
    Instrument_sptr m_instrument(new Instrument(instrument, pmap));

    // Create the NearestNeighbours object directly.
    NearestNeighbours nn(m_instrument, spectramap);
    for(size_t i = 0; i < 2000; i++)
    {
      nn.neighboursInRadius(1, 0.0);
    }
  }

  void testUsingNumberOfNeighbours()
  {
    Instrument_sptr instrument = boost::dynamic_pointer_cast<Instrument>(ComponentCreationHelper::createTestInstrumentCylindrical(2));
    const ISpectrumDetectorMapping spectramap = NearestNeighboursTest::buildSpectrumDetectorMapping(1, 18);
    // Default parameter map.
    ParameterMap_sptr pmap(new ParameterMap());
    // Parameterized instrument
    Instrument_sptr m_instrument(new Instrument(instrument, pmap));

    // Create the NearestNeighbours object directly.
    for(size_t i = 0; i < 2000; i++)
    {
      NearestNeighbours nn(8, m_instrument, spectramap);
      nn.neighbours(1);
    }
  }
};


#endif /* MANTID_TEST_GEOMETRY_NEARESTNEIGHBOURS */
