#ifndef SPECTRADETECTORMAPTEST_H_
#define SPECTRADETECTORMAPTEST_H_

#include <cxxtest/TestSuite.h>
#include <vector>

#include "MantidAPI/Instrument.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/DetectorGroup.h"
#include "MantidAPI/SpectraDetectorMap.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

class SpectraDetectorMapTest : public CxxTest::TestSuite
{
public:
  SpectraDetectorMapTest()
  {
    offset = 100000;
    length = 100;
    populateInstrument(inst,length);
    populateSDMap(sdMap,inst,length,offset);
  }

  void testPopulate()
  {
    TS_ASSERT_EQUALS(sdMap.nElements(),length);
  }

  void testNdet()
  {
    for (int i = 0; i < length; i++)
    {
      TS_ASSERT_EQUALS(sdMap.ndet(offset+i),1);
    }
  }

  void testGetDetector()
  {
    for (int i = 0; i < length; i++)
    {
      boost::shared_ptr<IDetector> d = sdMap.getDetector(offset+i);
      TS_ASSERT_EQUALS(d->getID(),i);
    }
  }

  void testGetDetectors()
  {
    for (int i = 0; i < length; i++)
    {
      std::vector<IDetector*> dvec = sdMap.getDetectors(offset+i);
      TS_ASSERT_EQUALS(dvec.size(),1);
      TS_ASSERT_EQUALS(dvec[0]->getID(),i);
    }
  }

  void testRemap()
  {
    //use my own local instrument and sdmap as I will be altering them
    SpectraDetectorMap sdMapLocal;
    Instrument instLocal;

    populateInstrument(instLocal,length);
    populateSDMap(sdMapLocal,instLocal,length,offset);

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
    std::vector<int> dets;
    int detLength=20;
    for (int i = 0; i < detLength; i++)
    {
      dets.push_back(i);
    }

    //remap them
    std::vector<int> spectra = sdMap.getSpectra(dets);
    for (int i = 0; i < detLength; i++)
    {
      TS_ASSERT_EQUALS(spectra[i],dets[i]+offset);
    }

  }

private:
  void populateInstrument(Instrument& inst,int length)
  {
    ObjComponent *sample = new ObjComponent("sample");
    inst.markAsSamplePos(sample);
    for (int i = 0; i < length; i++)
    {
      Detector* det = new Detector("det",0);
      det->setID(i);
      det->setPos(i,i,i);
      inst.add(det);
      inst.markAsDetector(det);
    }
  }

  void populateSDMap(SpectraDetectorMap& sdMap, Instrument& inst,int length, int offset)
  {
    int* udet = new int [length];
    int* spec = new int [length];
    for (int i = 0; i < length; i++)
    {
      spec[i] = i+offset;
      udet[i] = i;
    }
    sdMap.populate(spec,udet,length,&inst);
    delete [] spec;
    delete [] udet;
  } 

  SpectraDetectorMap sdMap;
  Instrument inst;
  int offset;
  int length;
};

#endif /*SPECTRADETECTORMAPTEST_H_*/
