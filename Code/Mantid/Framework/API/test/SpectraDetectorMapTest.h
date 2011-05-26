#ifndef SPECTRADETECTORMAPTEST_H_
#define SPECTRADETECTORMAPTEST_H_

#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/Exception.h"
#include <cxxtest/TestSuite.h>
#include <vector>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid;

class SpectraDetectorMapTest : public CxxTest::TestSuite
{
public:
  SpectraDetectorMapTest()
  {
    offset = 100000;
    length = 100;
    populateSDMap(sdMap,length,offset);
  }

  void testPopulate()
  {
    TS_ASSERT_EQUALS(sdMap.nElements(),length);
  }

  void testpopulateSimple()
  {
    SpectraDetectorMap sdMapLocal;
    sdMapLocal.populateSimple(10, 200);
    TS_ASSERT_EQUALS(sdMapLocal.nElements(),200-10);
    std::vector<detid_t> detsOut = sdMapLocal.getDetectors(10);
    TS_ASSERT_EQUALS(detsOut.size(),1);
    TS_ASSERT_EQUALS(detsOut[0], 10);
    detsOut = sdMapLocal.getDetectors(199);
    TS_ASSERT_EQUALS(detsOut[0], 199);
  }

  void testAddSpectrumEntries()
  {
    //use my own local sdmap as I will be altering it
    SpectraDetectorMap sdMapLocal;
    TS_ASSERT_EQUALS(sdMapLocal.nElements(),0);
    std::vector<detid_t> dets;
    dets.push_back(10);
    dets.push_back(20);
    TS_ASSERT_THROWS_NOTHING(sdMapLocal.addSpectrumEntries(1,dets));
    TS_ASSERT_EQUALS(sdMapLocal.nElements(),2);
    TS_ASSERT_EQUALS(sdMapLocal.ndet(1),2);
    std::vector<detid_t> detsOut = sdMapLocal.getDetectors(1);
    TS_ASSERT_EQUALS(detsOut.size(),2);
    TS_ASSERT_EQUALS(detsOut[0],10);
    TS_ASSERT_EQUALS(detsOut[1],20);
  }
  
  void testClear()
  {
    //use my own local sdmap as I will be altering it
    SpectraDetectorMap sdMapLocal;
    populateSDMap(sdMapLocal,length,offset);
    TS_ASSERT_EQUALS(sdMapLocal.nElements(),length);
    
    TS_ASSERT_THROWS_NOTHING(sdMapLocal.clear());
    TS_ASSERT_EQUALS(sdMapLocal.nElements(),0);
  }
  
  void testNdet()
  {
    for (int i = 0; i < length; i++)
    {
      TS_ASSERT_EQUALS(sdMap.ndet(offset+i),1);
    }
  }

  void testGetDetectors()
  {
    for (int i = 0; i < length; i++)
    {
      std::vector<detid_t> dvec = sdMap.getDetectors(offset+i);
      TS_ASSERT_EQUALS(dvec.size(),1);
      TS_ASSERT_EQUALS(dvec[0],i);
    }
  }

  void testRemap()
  {
    //use my own local sdmap as I will be altering it
    SpectraDetectorMap sdMapLocal;
    populateSDMap(sdMapLocal,length,offset);

    TS_ASSERT_EQUALS(sdMapLocal.nElements(),length);

    //remap to a new spectra that doesn't exist -> no action
    sdMapLocal.remap(offset,offset+length+1);
    TS_ASSERT_EQUALS(sdMapLocal.nElements(),length);

    //remap to a new spectra that does exist
    sdMapLocal.remap(offset,offset+1);
    TS_ASSERT_EQUALS(sdMapLocal.ndet(offset),0);
    TS_ASSERT_EQUALS(sdMapLocal.ndet(offset+1),2);
  }


  void testGetSpectra()
  {
    //create a vector of detectorids to map
    std::vector<detid_t> dets;
    int detLength=20;
    for (int i = 0; i < detLength; i++)
    {
      dets.push_back(i);
    }

    //remap them
    std::vector<specid_t> spectra = sdMap.getSpectra(dets);
    for (int i = 0; i < detLength; i++)
    {
      TS_ASSERT_EQUALS(spectra[i],dets[i]+offset);
    }
  }

  void testOperatorEquals()
  {
    TS_ASSERT( sdMap == sdMap )
  }

  void testOperatorNotEquals()
  {
    SpectraDetectorMap sdMapLocal;
    TS_ASSERT( sdMap != sdMapLocal )
  }

  void test_Iterator_Behaviour()
  {
    SpectraDetectorMap sdmapLocal;
    sdmapLocal.populateSimple(10, 15);
    Geometry::ISpectraDetectorMap::const_iterator itr = sdmapLocal.cbegin();
    TSM_ASSERT_EQUALS("Current spectrum should be the first", *itr, std::make_pair(10,10));
    ++itr;
    TSM_ASSERT_EQUALS("Current spectrum should be the second",*itr, std::make_pair(11,11));
    ++itr;
  }

  void test_Iterating_Over_The_Whole_Map()
  {
    SpectraDetectorMap sdmapLocal;
    sdmapLocal.populateSimple(10, 15);
    TS_ASSERT_EQUALS(sdmapLocal.nElements(), 5);
    // Insert an element with the same spectra but different ID
    sdmapLocal.addSpectrumEntries(14, std::vector<detid_t>(1,16));
    const size_t iterations = sdmapLocal.nElements();
    TS_ASSERT_EQUALS(iterations, 6);
    doIteratorRangeTest(sdmapLocal.cbegin(),sdmapLocal.cend(), 6);    
  }

  void test_Iterating_Over_A_Map_With_A_Single_Entry()
  {
    SpectraDetectorMap sdmapLocal;
    sdmapLocal.populateSimple(0,1);
    TS_ASSERT_EQUALS(sdmapLocal.nElements(), 1);
    doIteratorRangeTest(sdmapLocal.cbegin(), sdmapLocal.cend(), 1);
  }

  void test_Iterating_Over_An_Empty_Map()
  {
    SpectraDetectorMap sdmapLocal;
    TS_ASSERT_EQUALS(sdmapLocal.nElements(), 0);
    doIteratorRangeTest(sdmapLocal.cbegin(), sdmapLocal.cend(), 0);
  }


private:
  void populateSDMap(SpectraDetectorMap& sdMap, int length, const int offset)
  {
    int* udet = new int [length];
    int* spec = new int [length];
    for (int i = 0; i < length; i++)
    {
      spec[i] = i+offset;
      udet[i] = i;
    }
    sdMap.populate(spec,udet,length);
    delete [] spec;
    delete [] udet;
  } 
  
  void doIteratorRangeTest(SpectraDetectorMap::const_iterator itr, 
                           const SpectraDetectorMap::const_iterator & iend, 
                           const int remaining_itrs)
  {
    int nloops(0);
    for( ; itr != iend; ++itr )
    {
      ++nloops;
    }
    std::ostringstream os;
    os << remaining_itrs << " further iteration(s) should have been performed";
    TSM_ASSERT_EQUALS(os.str().c_str(), nloops, remaining_itrs);
  }


  SpectraDetectorMap sdMap;
  int offset;
  int length;
};

#endif /*SPECTRADETECTORMAPTEST_H_*/
