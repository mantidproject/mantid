#ifndef FAKEOBJECTS_H_
#define FAKEOBJECTS_H_

/*
 * FakeObjects.h: Fake Tester objects for APITest
 *
 *  Created on: Jul 5, 2011
 *      Author: Janik Zikovsky
 */

#include "MantidAPI/ISpectrum.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/ISpectraDetectorMap.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/INearestNeighbours.h"
#include "gmock/gmock.h"
#include <iostream>
#include <fstream>
#include <map>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid;


//===================================================================================================================
/** Helper class that implements ISpectrum */
class SpectrumTester : public ISpectrum
{
public:
  SpectrumTester() : ISpectrum() {}
  SpectrumTester(const specid_t specNo) : ISpectrum(specNo) {}

  virtual void setData(const MantidVec& Y) { data = Y; }
  virtual void setData(const MantidVec& Y, const MantidVec& E ) { data = Y; data_E=E; }

  virtual void setData(const MantidVecPtr& Y) { data.assign(Y->begin(), Y->end()); }
  virtual void setData(const MantidVecPtr& Y, const MantidVecPtr& E ) {data.assign(Y->begin(), Y->end()); data_E.assign(E->begin(), E->end()); }

  virtual void setData(const MantidVecPtr::ptr_type& Y) { data.assign(Y->begin(), Y->end()); }
  virtual void setData(const MantidVecPtr::ptr_type& Y, const MantidVecPtr::ptr_type& E) {data.assign(Y->begin(), Y->end()); data_E.assign(E->begin(), E->end()); }

  virtual MantidVec& dataY() { return data; }
  virtual MantidVec& dataE() { return data_E; }

  virtual const MantidVec& dataY() const { return data; }
  virtual const MantidVec& dataE() const { return data_E; }

  virtual size_t getMemorySize() const { return data.size() * sizeof(double) * 2; }

  /// Mask the spectrum to this value
  virtual void clearData()
  {
    // Assign the value to the data and error arrays
    MantidVec & yValues = this->dataY();
    std::fill(yValues.begin(), yValues.end(), 0.0);
    MantidVec & eValues = this->dataE();
    std::fill(eValues.begin(), eValues.end(), 0.0);
  }

protected:
  MantidVec data;
  MantidVec data_E;
};



//===================================================================================================================
class WorkspaceTester : public MatrixWorkspace
{
public:
  WorkspaceTester(Mantid::Geometry::INearestNeighboursFactory* nnFactory) : MatrixWorkspace(nnFactory){}
  WorkspaceTester() : MatrixWorkspace() {}
  virtual ~WorkspaceTester() {}

  // Empty overrides of virtual methods
  virtual size_t getNumberHistograms() const { return spec;}
  const std::string id() const {return "WorkspaceTester";}
  void init(const size_t& numspec, const size_t& j, const size_t&k)
  {
    spec = numspec;
    vec.resize(spec);
    for (size_t i=0; i<spec; i++)
    {
      vec[i].dataX().resize(j,1.0);
      vec[i].dataY().resize(k,1.0);
      vec[i].dataE().resize(k,1.0);
      vec[i].addDetectorID(detid_t(i));
      vec[i].setSpectrumNo(specid_t(i));
    }

    // Put an 'empty' axis in to test the getAxis method
    m_axes.resize(0);
    m_axes.push_back(new NumericAxis(1));
    m_axes[0]->title() = "1";
    m_axes.push_back(new NumericAxis(1));
    m_axes[1]->title() = "2";

    generateSpectraMap();
  }
  size_t size() const {return vec.size() * blocksize();}
  size_t blocksize() const {return vec[0].dataY().size();}
  ISpectrum * getSpectrum(const size_t index) { return &vec[index]; }
  const ISpectrum * getSpectrum(const size_t index) const { return &vec[index];; }

private:
  std::vector<SpectrumTester> vec;
  size_t spec;
};

//Helper and typedef for mocking NearestNeighbourFactory usage
class MockNearestNeighboursFactory : public Mantid::Geometry::INearestNeighboursFactory
{
public:
  MOCK_METHOD3(create, Mantid::Geometry::INearestNeighbours*(boost::shared_ptr<const Mantid::Geometry::Instrument>,const Mantid::Geometry::ISpectraDetectorMap&, bool));
  virtual ~MockNearestNeighboursFactory()
  {
  }
};

//Helper typedef and type for mocking NearestNeighbour map usage.
typedef std::map<Mantid::specid_t, double> SpectrumDistanceMap;
class MockNearestNeighbours : public Mantid::Geometry::INearestNeighbours {
 public:
  MOCK_CONST_METHOD2(neighbours, SpectrumDistanceMap(const specid_t spectrum, const double radius));
  MOCK_CONST_METHOD3(neighbours, SpectrumDistanceMap(const specid_t spectrum, bool force, const int numberofneighbours));
  MOCK_METHOD0(die, void());
  virtual ~MockNearestNeighbours()
  {
    die();
  }
};




#endif /* FAKEOBJECTS_H_ */
