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

#include <fstream>
#include <map>
#include <string>

#include "MantidAPI/ISpectrum.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/RefAxis.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidKernel/cow_ptr.h"

using namespace Mantid::API;
using namespace Mantid;

//===================================================================================================================
/** Helper class that implements ISpectrum */
class SpectrumTester : public ISpectrum {
public:
  SpectrumTester(HistogramData::Histogram::XMode xmode,
                 HistogramData::Histogram::YMode ymode)
      : ISpectrum(), m_histogram(xmode, ymode) {
    m_histogram.setCounts(0);
    m_histogram.setCountStandardDeviations(0);
  }
  SpectrumTester(const specnum_t specNo, HistogramData::Histogram::XMode xmode,
                 HistogramData::Histogram::YMode ymode)
      : ISpectrum(specNo), m_histogram(xmode, ymode) {
    m_histogram.setCounts(0);
    m_histogram.setCountStandardDeviations(0);
  }

  void setX(const Kernel::cow_ptr<HistogramData::HistogramX> &X) override {
    m_histogram.setX(X);
  }
  MantidVec &dataX() override { return m_histogram.dataX(); }
  const MantidVec &dataX() const override { return m_histogram.dataX(); }
  const MantidVec &readX() const override { return m_histogram.readX(); }
  Kernel::cow_ptr<HistogramData::HistogramX> ptrX() const override {
    return m_histogram.ptrX();
  }

  MantidVec &dataDx() override { return m_histogram.dataDx(); }
  const MantidVec &dataDx() const override { return m_histogram.dataDx(); }
  const MantidVec &readDx() const override { return m_histogram.readDx(); }

  MantidVec &dataY() override { return m_histogram.dataY(); }
  MantidVec &dataE() override { return m_histogram.dataE(); }

  const MantidVec &dataY() const override { return m_histogram.dataY(); }
  const MantidVec &dataE() const override { return m_histogram.dataE(); }

  size_t getMemorySize() const override {
    return readY().size() * sizeof(double) * 2;
  }

  /// Mask the spectrum to this value
  void clearData() override {
    // Assign the value to the data and error arrays
    MantidVec &yValues = this->dataY();
    std::fill(yValues.begin(), yValues.end(), 0.0);
    MantidVec &eValues = this->dataE();
    std::fill(eValues.begin(), eValues.end(), 0.0);
  }

protected:
  HistogramData::Histogram m_histogram;

private:
  const HistogramData::Histogram &histogramRef() const override {
    return m_histogram;
  }
  HistogramData::Histogram &mutableHistogramRef() override {
    return m_histogram;
  }
};

//===================================================================================================================
class WorkspaceTester : public MatrixWorkspace {
public:
  WorkspaceTester() : MatrixWorkspace(), spec(0) {}
  ~WorkspaceTester() override {}

  /// Returns a clone of the workspace
  std::unique_ptr<WorkspaceTester> clone() const {
    return std::unique_ptr<WorkspaceTester>(doClone());
  }

  // Empty overrides of virtual methods
  size_t getNumberHistograms() const override { return spec; }
  const std::string id() const override { return "WorkspaceTester"; }
  size_t size() const override { return vec.size() * blocksize(); }
  size_t blocksize() const override {
    return vec.empty() ? 0 : vec[0].dataY().size();
  }
  ISpectrum &getSpectrum(const size_t index) override {
    vec[index].setExperimentInfo(this, index);
    return vec[index];
  }
  const ISpectrum &getSpectrum(const size_t index) const override {
    return vec[index];
  }
  void generateHistogram(const std::size_t, const MantidVec &, MantidVec &,
                         MantidVec &, bool) const override {}
  Mantid::Kernel::SpecialCoordinateSystem
  getSpecialCoordinateSystem() const override {
    return Mantid::Kernel::None;
  }

protected:
  void init(const size_t &numspec, const size_t &j, const size_t &k) override {
    spec = numspec;
    vec.resize(spec, SpectrumTester(HistogramData::getHistogramXMode(j, k),
                                    HistogramData::Histogram::YMode::Counts));
    for (size_t i = 0; i < spec; i++) {
      vec[i].setExperimentInfo(this, i);
      vec[i].dataX().resize(j, 1.0);
      vec[i].dataY().resize(k, 1.0);
      vec[i].dataE().resize(k, 1.0);
      vec[i].addDetectorID(detid_t(i));
      vec[i].setSpectrumNo(specnum_t(i + 1));
    }

    // Put an 'empty' axis in to test the getAxis method
    m_axes.resize(2);
    m_axes[0] = new Mantid::API::RefAxis(j, this);
    m_axes[1] = new Mantid::API::SpectraAxis(this);
  }
  void init(const size_t &numspec,
            const HistogramData::Histogram &histogram) override {
    spec = numspec;
    vec.resize(spec, SpectrumTester(histogram.xMode(), histogram.yMode()));
    for (size_t i = 0; i < spec; i++) {
      vec[i].setHistogram(histogram);
      vec[i].addDetectorID(detid_t(i));
      vec[i].setSpectrumNo(specnum_t(i + 1));
    }

    // Put an 'empty' axis in to test the getAxis method
    m_axes.resize(2);
    m_axes[0] = new Mantid::API::RefAxis(histogram.x().size(), this);
    m_axes[1] = new Mantid::API::SpectraAxis(this);
  }

private:
  WorkspaceTester *doClone() const override {
    return new WorkspaceTester(*this);
  }
  WorkspaceTester *doCloneEmpty() const override {
    throw std::runtime_error("Cloning of WorkspaceTester is not implemented.");
  }
  std::vector<SpectrumTester> vec;
  size_t spec;
};

//===================================================================================================================
class TableWorkspaceTester : public ITableWorkspace {
public:
  TableWorkspaceTester() {}
  ~TableWorkspaceTester() override {}

  const std::string id() const override { return "TableWorkspaceTester"; }

  size_t getMemorySize() const override {
    throw std::runtime_error("getMemorySize not implemented");
  }

  Column_sptr addColumn(const std::string &, const std::string &) override {
    throw std::runtime_error("addColumn not implemented");
  }

  LogManager_sptr logs() override {
    throw std::runtime_error("logs not implemented");
  }

  LogManager_const_sptr getLogs() const override {
    throw std::runtime_error("getLogs not implemented");
  }

  void removeColumn(const std::string &) override {
    throw std::runtime_error("removeColumn not implemented");
  }

  ITableWorkspace *clone() const {
    throw std::runtime_error("removeColumn not implemented");
  }

  size_t columnCount() const override {
    throw std::runtime_error("columnCount not implemented");
  }

  Column_sptr getColumn(const std::string &) override {
    throw std::runtime_error("getColumn(str) not implemented");
  }

  Column_const_sptr getColumn(const std::string &) const override {
    throw std::runtime_error("getColumn(str) const not implemented");
  }

  Column_sptr getColumn(size_t) override {
    throw std::runtime_error("getColumn(size_t) not implemented");
  }

  Column_const_sptr getColumn(size_t) const override {
    throw std::runtime_error("getColumn(size_t) const not implemented");
  }

  std::vector<std::string> getColumnNames() const override {
    throw std::runtime_error("getColumnNames not implemented");
  }

  size_t rowCount() const override {
    throw std::runtime_error("rowCount not implemented");
  }

  void setRowCount(size_t) override {
    throw std::runtime_error("setRowCount not implemented");
  }

  size_t insertRow(size_t) override {
    throw std::runtime_error("insertRow not implemented");
  }

  void removeRow(size_t) override {
    throw std::runtime_error("removeRow not implemented");
  }

  void find(size_t, size_t &, const size_t &) override {
    throw std::runtime_error("find not implemented");
  }

  void find(double, size_t &, const size_t &) override {
    throw std::runtime_error("find not implemented");
  }

  void find(float, size_t &, const size_t &) override {
    throw std::runtime_error("find not implemented");
  }

  void find(Boolean, size_t &, const size_t &) override {
    throw std::runtime_error("find not implemented");
  }

  void find(std::string, size_t &, const size_t &) override {
    throw std::runtime_error("find not implemented");
  }

  void find(Kernel::V3D, size_t &, const size_t &) override {
    throw std::runtime_error("find not implemented");
  }

private:
  TableWorkspaceTester *doClone() const override {
    throw std::runtime_error(
        "Cloning of TableWorkspaceTester is not implemented.");
  }
  ITableWorkspace *
  doCloneColumns(const std::vector<std::string> &) const override {
    throw std::runtime_error(
        "Cloning of TableWorkspaceTester is not implemented.");
  }
};

//===================================================================================================================
class ColumnTester : public Column {
  size_t size() const override {
    throw std::runtime_error("size not implemented");
  }

  std::type_info &get_type_info() const override {
    throw std::runtime_error("get_type_info not implemented");
  }

  std::type_info &get_pointer_type_info() const override {
    throw std::runtime_error("get_pointer_type_info not implemented");
  }

  void print(size_t, std::ostream &) const override {
    throw std::runtime_error("print not implemented");
  }

  bool isBool() const override {
    throw std::runtime_error("isBool not implemented");
  }

  long int sizeOfData() const override {
    throw std::runtime_error("sizeOfData not implemented");
  }

  Column *clone() const override {
    throw std::runtime_error("clone not implemented");
  }

  double toDouble(size_t) const override {
    throw std::runtime_error("toDouble not implemented");
  }

  void fromDouble(size_t, double) override {
    throw std::runtime_error("fromDouble not implemented");
  }

protected:
  void resize(size_t) override {
    throw std::runtime_error("resize not implemented");
  }
  void insert(size_t) override {
    throw std::runtime_error("insert not implemented");
  }
  void remove(size_t) override {
    throw std::runtime_error("remove not implemented");
  }
  void *void_pointer(size_t) override {
    throw std::runtime_error("void_pointer not implemented");
  }
  const void *void_pointer(size_t) const override {
    throw std::runtime_error("void_pointer const not implemented");
  }
};
#endif /* FAKEOBJECTS_H_ */
