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
#include "MantidAPI/ITableWorkspace.h"
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
#include <string>

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

//===================================================================================================================
class TableWorkspaceTester : public ITableWorkspace {
public:
  TableWorkspaceTester() {}
  ~TableWorkspaceTester() {}

  const std::string id() const { return "TableWorkspaceTester"; }

  size_t getMemorySize() const {
    throw std::runtime_error("getMemorySize not implemented");
  }

  Column_sptr addColumn(const std::string&, const std::string&) {
    throw std::runtime_error("addColumn not implemented");
  }

  LogManager_sptr logs() {
    throw std::runtime_error("logs not implemented");
  }

  LogManager_const_sptr getLogs() const {
    throw std::runtime_error("getLogs not implemented");
  }

  void removeColumn(const std::string&) {
    throw std::runtime_error("removeColumn not implemented");
  }

  ITableWorkspace* clone() const {
    throw std::runtime_error("removeColumn not implemented");
  }

  size_t columnCount() const {
    throw std::runtime_error("columnCount not implemented");
  }

  Column_sptr getColumn(const std::string&) {
    throw std::runtime_error("getColumn(str) not implemented");
  }

  Column_const_sptr getColumn(const std::string&) const {
    throw std::runtime_error("getColumn(str) const not implemented");
  }

  Column_sptr getColumn(size_t) {
    throw std::runtime_error("getColumn(size_t) not implemented");
  }

  Column_const_sptr getColumn(size_t) const {
    throw std::runtime_error("getColumn(size_t) const not implemented");
  }

  std::vector<std::string> getColumnNames() const {
    throw std::runtime_error("getColumnNames not implemented");
  }

  size_t rowCount() const {
    throw std::runtime_error("rowCount not implemented");
  }

  void setRowCount(size_t) {
    throw std::runtime_error("setRowCount not implemented");
  }

  size_t insertRow(size_t) {
    throw std::runtime_error("insertRow not implemented");
  }

  void removeRow(size_t) {
    throw std::runtime_error("removeRow not implemented");
  }

  void find(size_t, size_t&, const size_t&) {
    throw std::runtime_error("find not implemented");
  }

  void find(double, size_t&, const size_t&) {
    throw std::runtime_error("find not implemented");
  }

  void find(float, size_t&, const size_t&) {
    throw std::runtime_error("find not implemented");
  }

  void find(Boolean, size_t&, const size_t&) {
    throw std::runtime_error("find not implemented");
  }

  void find(std::string, size_t&, const size_t&) {
    throw std::runtime_error("find not implemented");
  }

  void find(V3D, size_t&, const size_t&) {
    throw std::runtime_error("find not implemented");
  }
};

//===================================================================================================================
class ColumnTester : public Column {
  size_t size() const {
    throw std::runtime_error("size not implemented");
  }

  std::type_info &get_type_info() const {
    throw std::runtime_error("get_type_info not implemented");
  }

  std::type_info &get_pointer_type_info() const {
    throw std::runtime_error("get_pointer_type_info not implemented");
  }

  void print(size_t, std::ostream&) const {
    throw std::runtime_error("print not implemented");
  }

  bool isBool() const {
    throw std::runtime_error("isBool not implemented");
  }

  long int sizeOfData() const {
    throw std::runtime_error("sizeOfData not implemented");
  }

  Column* clone() const {
    throw std::runtime_error("clone not implemented");
  }

  double toDouble(size_t) const {
    throw std::runtime_error("toDouble not implemented");
  }

  void fromDouble(size_t,double) {
    throw std::runtime_error("fromDouble not implemented");
  }

protected:
  void resize(size_t) {
    throw std::runtime_error("resize not implemented");
  }
  void insert(size_t) {
    throw std::runtime_error("insert not implemented");
  }
  void remove(size_t) {
    throw std::runtime_error("remove not implemented");
  }
  void* void_pointer(size_t) {
    throw std::runtime_error("void_pointer not implemented");
  }
  const void* void_pointer(size_t) const {
    throw std::runtime_error("void_pointer const not implemented");
  }

};
#endif /* FAKEOBJECTS_H_ */
