#ifndef ONETOONESPECTRADETECTORMAPTEST_H_
#define ONETOONESPECTRADETECTORMAPTEST_H_

#include "MantidGeometry/Instrument/OneToOneSpectraDetectorMap.h"
#include "cxxtest/TestSuite.h"

using Mantid::Geometry::OneToOneSpectraDetectorMap;
using Mantid::Geometry::ISpectraDetectorMap;
using Mantid::specid_t;
using Mantid::detid_t;

class OneToOneSpectraDetectorMapTest : public CxxTest::TestSuite
{
public:
  
  void test_That_Default_Construction_Gives_An_Empty_Map()
  {
    OneToOneSpectraDetectorMap empty;
    TSM_ASSERT_EQUALS("A default map is empty", empty.nElements(), 0);
  }

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

  void test_That_Two_Objects_With_The_Same_Start_And_End_Are_Considered_Equal()
  {
    OneToOneSpectraDetectorMap lhs(0,4);
    OneToOneSpectraDetectorMap rhs(0,4);
    TSM_ASSERT("Two objects with equal start and end should be equal", lhs==rhs);
  }

  void test_That_Two_Objects_With_The_Different_Start_And_End_Are_Not_Considered_Equal()
  {
    OneToOneSpectraDetectorMap lhs(0,4);
    OneToOneSpectraDetectorMap rhs(1,4);
    TSM_ASSERT("Two objects with different start and end should not be equal", lhs!=rhs);
    OneToOneSpectraDetectorMap lhs2(0,4);
    OneToOneSpectraDetectorMap rhs2(1,4);
    TSM_ASSERT("Two objects with different start and end should not be equal", lhs2 != rhs2);
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
    ISpectraDetectorMap::const_iterator itr = spectramap.cbegin();
    TSM_ASSERT_EQUALS("Current spectrum should be the first", *itr, std::make_pair(0,0));
    ++itr;
    TSM_ASSERT_EQUALS("Current spectrum should be the second",*itr, std::make_pair(1,1));
    ++itr;
    doIteratorRangeTest(spectramap, itr, 3, 2);
  }

  void test_Iterators_For_Map_With_Single_Entry()
  {
    OneToOneSpectraDetectorMap spectramap(1,1);
    doIteratorRangeTest(spectramap, spectramap.cbegin(), 1, 1);
  }

  void test_Iterators_For_Empty_Map()
  {
    OneToOneSpectraDetectorMap spectramap;
    doIteratorRangeTest(spectramap, spectramap.cbegin(), 0, 0);
  }

private:
  
  void doIteratorRangeTest(const ISpectraDetectorMap &spectramap, 
                           ISpectraDetectorMap::const_iterator  itr, 
                           const int remaining_itrs, const specid_t current_value)
  {
    int nloops(0);
    specid_t value = current_value;
    ISpectraDetectorMap::const_iterator iend = spectramap.cend();
    for( ; itr != iend; ++itr )
    {
      ISpectraDetectorMap::value_type current = std::make_pair(value, value);
      TS_ASSERT_EQUALS(*itr, current);
      TS_ASSERT_EQUALS(itr->first, current.first);
      TS_ASSERT_EQUALS(itr->second, current.second);
      ++nloops;
      ++value;
    }
    std::ostringstream os;
    os << remaining_itrs << " further iteration(s) should have been performed";
    TSM_ASSERT_EQUALS(os.str().c_str(), nloops, remaining_itrs);

  }


};

#endif
