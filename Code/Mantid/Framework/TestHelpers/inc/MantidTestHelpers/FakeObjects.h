/*********************************************************************************
 *  PLEASE READ THIS!!!!!!!
 *
 *  This header MAY NOT be included in any test from a package below API
 *    (e.g. Kernel, Geometry).
 *  Conversely, this file MAY NOT be modified to use anything from a package
 *higher
 *  than API (e.g. any algorithm or concrete workspace), even if via the
 *factory.
 *********************************************************************************/
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
#include "MantidAPI/RefAxis.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/INearestNeighbours.h"
#include <iostream>
#include <fstream>
#include <map>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid;

//===================================================================================================================
/** Helper class that implements ISpectrum */
class SpectrumTester : public ISpectrum {
public:
  SpectrumTester() : ISpectrum() {}
  SpectrumTester(const specid_t specNo) : ISpectrum(specNo) {}

  virtual void setData(const MantidVec &Y) { data = Y; }
  virtual void setData(const MantidVec &Y, const MantidVec &E) {
    data = Y;
    data_E = E;
  }

  virtual void setData(const MantidVecPtr &Y) {
    data.assign(Y->begin(), Y->end());
  }
  virtual void setData(const MantidVecPtr &Y, const MantidVecPtr &E) {
    data.assign(Y->begin(), Y->end());
    data_E.assign(E->begin(), E->end());
  }

  virtual void setData(const MantidVecPtr::ptr_type &Y) {
    data.assign(Y->begin(), Y->end());
  }
  virtual void setData(const MantidVecPtr::ptr_type &Y,
                       const MantidVecPtr::ptr_type &E) {
    data.assign(Y->begin(), Y->end());
    data_E.assign(E->begin(), E->end());
  }

  virtual MantidVec &dataY() { return data; }
  virtual MantidVec &dataE() { return data_E; }

  virtual const MantidVec &dataY() const { return data; }
  virtual const MantidVec &dataE() const { return data_E; }

  virtual size_t getMemorySize() const {
    return data.size() * sizeof(double) * 2;
  }

  /// Mask the spectrum to this value
  virtual void clearData() {
    // Assign the value to the data and error arrays
    MantidVec &yValues = this->dataY();
    std::fill(yValues.begin(), yValues.end(), 0.0);
    MantidVec &eValues = this->dataE();
    std::fill(eValues.begin(), eValues.end(), 0.0);
  }

protected:
  MantidVec data;
  MantidVec data_E;
};

//===================================================================================================================
class WorkspaceTester : public MatrixWorkspace {
public:
  WorkspaceTester(Mantid::Geometry::INearestNeighboursFactory *nnFactory)
      : MatrixWorkspace(nnFactory), spec(0) {}
  WorkspaceTester() : MatrixWorkspace(), spec(0) {}
  virtual ~WorkspaceTester() {}

  // Empty overrides of virtual methods
  virtual size_t getNumberHistograms() const { return spec; }
  const std::string id() const { return "WorkspaceTester"; }
  void init(const size_t &numspec, const size_t &j, const size_t &k) {
    spec = numspec;
    vec.resize(spec);
    for (size_t i = 0; i < spec; i++) {
      vec[i].dataX().resize(j, 1.0);
      vec[i].dataY().resize(k, 1.0);
      vec[i].dataE().resize(k, 1.0);
      vec[i].addDetectorID(detid_t(i));
      vec[i].setSpectrumNo(specid_t(i + 1));
    }

    // Put an 'empty' axis in to test the getAxis method
    m_axes.resize(2);
    m_axes[0] = new Mantid::API::RefAxis(j, this);
    m_axes[1] = new Mantid::API::SpectraAxis(this);
  }
  size_t size() const { return vec.size() * blocksize(); }
  size_t blocksize() const { return vec[0].dataY().size(); }
  ISpectrum *getSpectrum(const size_t index) { return &vec[index]; }
  const ISpectrum *getSpectrum(const size_t index) const {
    return &vec[index];
    ;
  }
  void generateHistogram(const std::size_t, const MantidVec &, MantidVec &,
                         MantidVec &, bool) const {}
  virtual Mantid::Kernel::SpecialCoordinateSystem
  getSpecialCoordinateSystem() const {
    return Mantid::Kernel::None;
  }

private:
  std::vector<SpectrumTester> vec;
  size_t spec;
};

#endif /* FAKEOBJECTS_H_ */
