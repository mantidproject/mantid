#ifndef MANTID_GEOMETRY_NEARESTNEIGHBOURSFACTORYTEST_H_
#define MANTID_GEOMETRY_NEARESTNEIGHBOURSFACTORYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidGeometry/Instrument/NearestNeighboursFactory.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

#include "NearestNeighboursTest.h"

using namespace Mantid;
using namespace Mantid::Geometry;

class NearestNeighboursFactoryTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NearestNeighboursFactoryTest *createSuite() { return new NearestNeighboursFactoryTest(); }
  static void destroySuite( NearestNeighboursFactoryTest *suite ) { delete suite; }


  void testCreateObject()
  {
    Instrument_sptr instrument = boost::dynamic_pointer_cast<Instrument>(ComponentCreationHelper::createTestInstrumentCylindrical(2));
    const ISpectrumDetectorMapping spectramap = NearestNeighboursTest::buildSpectrumDetectorMapping(1, 18);
    // Default parameter map.
    ParameterMap_sptr pmap(new ParameterMap());
    // Parameterized instrument
    Instrument_sptr m_instrument(new Instrument(instrument, pmap));

    NearestNeighboursFactory factory;
    NearestNeighbours* nn = NULL;

    TSM_ASSERT_THROWS_NOTHING("Create on Factory should not throw", nn = factory.create(instrument, spectramap););
    TSM_ASSERT("Null object created", nn != NULL);
    delete nn;
  }

  void testCreateObjectAsINearestNeighbourFactory()
  {
    Instrument_sptr instrument = boost::dynamic_pointer_cast<Instrument>(ComponentCreationHelper::createTestInstrumentCylindrical(2));
    const ISpectrumDetectorMapping spectramap = NearestNeighboursTest::buildSpectrumDetectorMapping(1, 18);
    // Default parameter map.
    ParameterMap_sptr pmap(new ParameterMap());
    // Parameterized instrument
    Instrument_sptr m_instrument(new Instrument(instrument, pmap));

    NearestNeighboursFactory factory;
    //Create the alias
    INearestNeighboursFactory& ifactory = factory;
    //The following will break if NearestNeighbours is not an INearestNeighbours object.
    INearestNeighbours* nn = ifactory.create(instrument, spectramap);
    //For consistency.
    TSM_ASSERT("Product should be a NearestNeighbours object.", NULL != dynamic_cast<NearestNeighbours*>(nn));
    delete nn;
  }
};


#endif /* MANTID_GEOMETRY_NEARESTNEIGHBOURSFACTORYTEST_H_ */
