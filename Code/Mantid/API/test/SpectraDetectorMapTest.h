#ifndef SPECTRADETECTORMAPTEST_H_
#define SPECTRADETECTORMAPTEST_H_

#include <cxxtest/TestSuite.h>
#include <vector>

#include "MantidAPI/Instrument.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/DetectorGroup.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/MatrixWorkspace.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

class tstWorkspace: public MatrixWorkspace
{
    std::vector<double> vec;
public:

  /// Return the workspace typeID
    virtual const std::string id() const{return "tstWorkspace";}
  /// Returns the number of single indexable items in the workspace
    virtual int size() const{return 0;}
  /// Returns the size of each block of data returned by the dataX accessors
    virtual int blocksize() const{return 0;}
  /// Returns the number of histograms in the workspace
    virtual const int getNumberHistograms() const{return 0;}

  /// Returns the x data
    virtual std::vector<double>& dataX(int const index){return vec;}
  /// Returns the y data
  virtual std::vector<double>& dataY(int const index){return vec;}
  /// Returns the error data
  virtual std::vector<double>& dataE(int const index){return vec;}
  /// Returns the x data const
  virtual const std::vector<double>& dataX(int const index) const{return vec;}
  /// Returns the y data const
  virtual const std::vector<double>& dataY(int const index) const{return vec;}
  /// Returns the error const
  virtual const std::vector<double>& dataE(int const index) const{return vec;}
  //----------------------------------------------------------------------

  /// Returns the ErrorHelper applicable for this spectra
  virtual const IErrorHelper* errorHelper(int const index) const{return 0;}
  /// Sets the ErrorHelper for this spectra
  virtual void setErrorHelper(int const index,IErrorHelper* errorHelper){}
  /// Sets the ErrorHelper for this spectra
  virtual void setErrorHelper(int const index,const IErrorHelper* errorHelper){}
protected:
    void init(const int &NVectors, const int &XLength, const int &YLength){}
};

class NoDeleting
{
public:
    void operator()(void*){}
};

class SpectraDetectorMapTest : public CxxTest::TestSuite
{
public:
  SpectraDetectorMapTest()
  {
    offset = 100000;
    length = 100;
    populateInstrument(inst,length);
    populateSDMap(WS,inst,length,offset);
  }

  void testPopulate()
  {
      TS_ASSERT_EQUALS(WS.getSpectraMap()->nElements(),length);
  }

  void testNdet()
  {
    for (int i = 0; i < length; i++)
    {
      TS_ASSERT_EQUALS(WS.getSpectraMap()->ndet(offset+i),1);
    }
  }

  void testGetDetector()
  {
    for (int i = 0; i < length; i++)
    {
      boost::shared_ptr<IDetector> d = WS.getSpectraMap()->getDetector(offset+i);
      TS_ASSERT_EQUALS(d->getID(),i);
    }
  }

  void testGetDetectors()
  {
    for (int i = 0; i < length; i++)
    {
        std::vector<boost::shared_ptr<IDetector> > dvec = WS.getSpectraMap()->getDetectors(offset+i);
      TS_ASSERT_EQUALS(dvec.size(),1);
      TS_ASSERT_EQUALS(dvec[0]->getID(),i);
    }
  }

  void testRemap()
  {
    //use my own local instrument and sdmap as I will be altering them
    tstWorkspace LocalWS;
    Instrument instLocal;

    populateInstrument(instLocal,length);
    populateSDMap(LocalWS,instLocal,length,offset);

    TS_ASSERT_EQUALS(LocalWS.getSpectraMap()->nElements(),length);

    //remap to a new spectra that doesn't exist -> no action
    LocalWS.getSpectraMap()->remap(offset,offset+length+1);
    TS_ASSERT_EQUALS(LocalWS.getSpectraMap()->nElements(),length);

    //remap to a new spectra that does exist
    LocalWS.getSpectraMap()->remap(offset,offset+1);
    TS_ASSERT_EQUALS(LocalWS.getSpectraMap()->ndet(offset),0);
    TS_ASSERT_EQUALS(LocalWS.getSpectraMap()->ndet(offset+1),2);
  }

  void testSetMap()
  {
      tstWorkspace LocalWS;
      LocalWS.setSpectraMap(WS.getSpectraMap());
      TS_ASSERT_EQUALS(LocalWS.getSpectraMap()->nElements(),length);
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
    std::vector<int> spectra = WS.getSpectraMap()->getSpectra(dets);
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

  void populateSDMap(MatrixWorkspace& ws, Instrument& inst,int length, int offset)
  {
    int* udet = new int [length];
    int* spec = new int [length];
    for (int i = 0; i < length; i++)
    {
      spec[i] = i+offset;
      udet[i] = i;
    }
    ws.setInstrument(boost::shared_ptr<Instrument>(&inst,NoDeleting()));
    ws.getSpectraMap()->populate(spec,udet,length);
    delete [] spec;
    delete [] udet;
  } 

  tstWorkspace WS;
  //SpectraDetectorMap sdMap;
  Instrument inst;
  int offset;
  int length;
};

#endif /*SPECTRADETECTORMAPTEST_H_*/
