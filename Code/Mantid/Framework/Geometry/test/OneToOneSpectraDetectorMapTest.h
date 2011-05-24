#ifndef ONETOONESPECTRADETECTORMAPTEST_H_
#define ONETOONESPECTRADETECTORMAPTEST_H_

#include "MantidGeometry/Instrument/OneToOneSpectraDetectorMap.h"
#include "cxxtest/TestSuite.h"

using Mantid::Geometry::OneToOneSpectraDetectorMap;
using Mantid::specid_t;
using Mantid::detid_t;

class OneToOneSpectraDetectorMapTest : public CxxTest::TestSuite
{
public:
  
  void test_That_Map_Construction_Gives_A_Map_Including_Both_Ends()
  {
    OneToOneSpectraDetectorMap eleven(0,10);
    TSM_ASSERT_EQUALS("Map should contain 11 elements", eleven.nElements(), 11);
    OneToOneSpectraDetectorMap single(0,0);
    TSM_ASSERT_EQUALS("Map should contain 1 element", single.nElements(), 1);
    OneToOneSpectraDetectorMap nonzero(5,10);
    TSM_ASSERT_EQUALS("Map should contain 1 element", nonzero.nElements(), 6);
  }

  void test_That_Each_Spectra_Is_Mapped_to_Exactly_One_Detector()
  {
    OneToOneSpectraDetectorMap spectramap(0,4);
    for( specid_t i = 0; i < 5; ++i )
    {
      TSM_ASSERT_EQUALS("Expected a 1:1 mapping", spectramap.ndet(i), 1);
    }
  }

  void test_That_A_Valid_Spectrum_Returns_The_Same_Number_For_The_DetectorID()
  {
    OneToOneSpectraDetectorMap spectramap(0,4);
    std::vector<detid_t> ids;
    TSM_ASSERT_THROWS_NOTHING("A valid spectrum number should not throw", ids = spectramap.getDetectors(2));
    TSM_ASSERT_EQUALS("The ID list should contain 1 element", ids.size(), 1);
    TSM_ASSERT_EQUALS("The element should equals 2", ids[0], 2);
  }

  void test_That_An_Invalid_Spectrum_Throws_When_Retrieving_Detectors()
  {
    OneToOneSpectraDetectorMap spectramap(0,4);
    TSM_ASSERT_THROWS("An invalid spectrum number should throw an out_of_range error", 
		      spectramap.getDetectors(5), std::out_of_range);
    TSM_ASSERT_THROWS("A invalid spectrum number should throw an out_of_range error", 
		      spectramap.getDetectors(-1), std::out_of_range);
  }
  
  void test_That_A_Valid_DetectorID_List_Returns_The_Same_Numbers()
  {
    OneToOneSpectraDetectorMap spectramap(0,4);
    const size_t ndets(3);
    std::vector<detid_t> detList(ndets);
    for( specid_t i = 0; i < specid_t(ndets); ++i)
    {
      detList[i] = i+1;
    }
    std::vector<specid_t> spectra;
    TSM_ASSERT_THROWS_NOTHING("A valid detector ID should not throw", 
			      spectra = spectramap.getSpectra(detList));
    TSM_ASSERT_EQUALS("The list should be the same size as the ID list", spectra.size(), 3);
    for( specid_t i = 0; i < specid_t(ndets); ++i)
    {
      TSM_ASSERT_EQUALS("The spectrum number should equal to detector ID", spectra[i], detList[i]);
    }
  }

  void test_That_An_Invalid_DetectorID_List_Throws_An_Invalid_Argument()
  {
    OneToOneSpectraDetectorMap spectramap(0,4);
    const size_t ndets(3);
    std::vector<detid_t> detList(ndets);
    detList[0] = 0;
    detList[1] = 1;
    detList[2] = 5;
    TSM_ASSERT_THROWS("An invalid detector ID list should throw", 
		      spectramap.getSpectra(detList), std::invalid_argument);
  }

  void test_Iterator_Behaviour()
  {
    OneToOneSpectraDetectorMap spectramap(0,4);
    spectramap.moveIteratorToStart();
    TSM_ASSERT_EQUALS("Current spectrum should be the first",spectramap.getCurrentSpectrum(), 0);
    TSM_ASSERT_EQUALS("We should have a next element", spectramap.hasNext(), true);
    TSM_ASSERT_THROWS_NOTHING("Advancing should not throw", spectramap.advanceIterator());
    TSM_ASSERT_EQUALS("Current spectrum should be the second",spectramap.getCurrentSpectrum(), 1);
    
    spectramap.advanceIterator(); //Now = 2
    spectramap.advanceIterator(); //Now = 3
    spectramap.advanceIterator(); //Now = 4
    TSM_ASSERT_EQUALS("We should not have a next element", spectramap.hasNext(), false);
    TSM_ASSERT_EQUALS("Current spectrum should be the end",spectramap.getCurrentSpectrum(), 4);

  }

  void test_Iterator_Behaviour_For_A_Single_Element()
  {
    OneToOneSpectraDetectorMap spectramap(5,5);
    TSM_ASSERT_EQUALS("Current spectrum should be the first",spectramap.getCurrentSpectrum(), 5);
    TSM_ASSERT_EQUALS("We should not have a next element", spectramap.hasNext(), false);
  }

};

#endif
