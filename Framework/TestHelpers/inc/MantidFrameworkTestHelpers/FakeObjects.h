// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
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
#pragma once

/*
 * FakeObjects.h: Fake Tester objects for APITest
 *
 *  Created on: Jul 5, 2011
 *      Author: Janik Zikovsky
 */

#include <algorithm>
#include <fstream>
#include <map>
#include <string>

#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/ISpectrum.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/RefAxis.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include "MantidKernel/cow_ptr.h"

using namespace Mantid::API;
using namespace Mantid::Geometry;
using Mantid::coord_t;
using Mantid::detid_t;
using Mantid::MantidVec;
using Mantid::signal_t;
using Mantid::specnum_t;
using Mantid::Kernel::SpecialCoordinateSystem;

//===================================================================================================================
/** Helper class that implements ISpectrum */
class SpectrumTester : public ISpectrum {
public:
  SpectrumTester(Mantid::HistogramData::Histogram::XMode xmode, Mantid::HistogramData::Histogram::YMode ymode)
      : ISpectrum(), m_histogram(xmode, ymode) {
    m_histogram.setCounts(0);
    m_histogram.setCountStandardDeviations(0);
  }
  SpectrumTester(const specnum_t specNo, Mantid::HistogramData::Histogram::XMode xmode,
                 Mantid::HistogramData::Histogram::YMode ymode)
      : ISpectrum(specNo), m_histogram(xmode, ymode) {
    m_histogram.setCounts(0);
    m_histogram.setCountStandardDeviations(0);
  }

  void copyDataFrom(const ISpectrum &other) override { other.copyDataInto(*this); }

  void setX(const Mantid::Kernel::cow_ptr<Mantid::HistogramData::HistogramX> &X) override { m_histogram.setX(X); }
  MantidVec &dataX() override { return m_histogram.dataX(); }
  const MantidVec &dataX() const override { return m_histogram.dataX(); }
  const MantidVec &readX() const override { return m_histogram.readX(); }
  Mantid::Kernel::cow_ptr<Mantid::HistogramData::HistogramX> ptrX() const override { return m_histogram.ptrX(); }

  MantidVec &dataDx() override { return m_histogram.dataDx(); }
  const MantidVec &dataDx() const override { return m_histogram.dataDx(); }
  const MantidVec &readDx() const override { return m_histogram.readDx(); }

  MantidVec &dataY() override { return m_histogram.dataY(); }
  MantidVec &dataE() override { return m_histogram.dataE(); }

  const MantidVec &dataY() const override { return m_histogram.dataY(); }
  const MantidVec &dataE() const override { return m_histogram.dataE(); }

  size_t getMemorySize() const override { return readY().size() * sizeof(double) * 2; }

  /// Mask the spectrum to this value
  void clearData() override {
    // Assign the value to the data and error arrays
    MantidVec &yValues = this->dataY();
    std::fill(yValues.begin(), yValues.end(), 0.0);
    MantidVec &eValues = this->dataE();
    std::fill(eValues.begin(), eValues.end(), 0.0);
  }

protected:
  Mantid::HistogramData::Histogram m_histogram;

private:
  using ISpectrum::copyDataInto;
  void copyDataInto(SpectrumTester &other) const override { other.m_histogram = m_histogram; }

  const Mantid::HistogramData::Histogram &histogramRef() const override { return m_histogram; }
  Mantid::HistogramData::Histogram &mutableHistogramRef() override { return m_histogram; }
};

//===================================================================================================================

class FakeWorkspace : public Mantid::API::Workspace {
public:
  const std::string id() const override { return "FakeWorkspace"; }
  Workspace *doClone() const override { return new FakeWorkspace(*this); }
  Workspace *doCloneEmpty() const override { return new FakeWorkspace; }
  const std::string toString() const override { return "FakeWorkspace instance"; }
  size_t getMemorySize() const override { return 0; }
};

//===================================================================================================================
class AxeslessWorkspaceTester : public MatrixWorkspace {
public:
  AxeslessWorkspaceTester() : MatrixWorkspace(), m_spec(0) {}

  bool isRaggedWorkspace() const override {
    if (m_vec.empty()) {
      throw std::runtime_error("Vector data is empty, cannot check for ragged workspace.");
    } else {
      const auto numberOfBins = m_vec[0].dataY().size();
      return std::any_of(m_vec.cbegin(), m_vec.cend(),
                         [&numberOfBins](const auto &spectrum) { return numberOfBins != spectrum.dataY().size(); });
    }
  }

  // Empty overrides of virtual methods
  size_t getNumberHistograms() const override { return m_spec; }
  const std::string id() const override { return "AxeslessWorkspaceTester"; }
  size_t size() const override {
    return std::accumulate(m_vec.cbegin(), m_vec.cend(), static_cast<size_t>(0),
                           [](size_t total, const SpectrumTester &i) { return total + i.dataY().size(); });
  }
  size_t blocksize() const override {
    if (m_vec.empty()) {
      return 0;
    } else {
      size_t numY = m_vec[0].dataY().size();
      if (std::any_of(m_vec.cbegin(), m_vec.cend(), [numY](auto it) { return it.dataY().size() != numY; })) {
        throw std::logic_error("non-constant number of bins");
      }
      return numY;
    }
  }

  std::size_t getNumberBins(const std::size_t &index) const override {
    if (index > m_vec.size())
      return 0;
    return m_vec[index].dataY().size();
  }

  std::size_t getMaxNumberBins() const override {
    if (m_vec.empty()) {
      return 0;
    } else {
      const auto iter =
          std::max_element(m_vec.cbegin(), m_vec.cend(), [](const SpectrumTester &s1, const SpectrumTester &s2) {
            return s1.dataY().size() < s2.dataY().size();
          });
      return iter->dataY().size();
    }
  }

  ISpectrum &getSpectrum(const size_t index) override {
    invalidateCommonBinsFlag();
    m_vec[index].setMatrixWorkspace(this, index);
    return m_vec[index];
  }
  const ISpectrum &getSpectrum(const size_t index) const override { return m_vec[index]; }
  void generateHistogram(const std::size_t, const MantidVec &, MantidVec &, MantidVec &, bool) const override {}
  Mantid::Kernel::SpecialCoordinateSystem getSpecialCoordinateSystem() const override { return Mantid::Kernel::None; }

protected:
  void init(const size_t &numspec, const size_t &j, const size_t &k) override {
    m_spec = numspec;
    m_vec.resize(m_spec, SpectrumTester(Mantid::HistogramData::getHistogramXMode(j, k),
                                        Mantid::HistogramData::Histogram::YMode::Counts));
    for (size_t i = 0; i < m_spec; i++) {
      m_vec[i].setMatrixWorkspace(this, i);
      m_vec[i].dataX().resize(j, 1.0);
      m_vec[i].dataY().resize(k, 1.0);
      m_vec[i].dataE().resize(k, 1.0);
      m_vec[i].addDetectorID(detid_t(i));
      m_vec[i].setSpectrumNo(specnum_t(i + 1));
    }
  }
  void init(const Mantid::HistogramData::Histogram &histogram) override {
    m_spec = numberOfDetectorGroups();
    m_vec.resize(m_spec, SpectrumTester(histogram.xMode(), histogram.yMode()));
    for (size_t i = 0; i < m_spec; i++) {
      m_vec[i].setHistogram(histogram);
      m_vec[i].addDetectorID(detid_t(i));
      m_vec[i].setSpectrumNo(specnum_t(i + 1));
    }
  }

  AxeslessWorkspaceTester *doClone() const override { return new AxeslessWorkspaceTester(*this); }
  AxeslessWorkspaceTester *doCloneEmpty() const override {
    throw std::runtime_error("Cloning of AxeslessWorkspaceTester is not implemented.");
  }

private:
  std::vector<SpectrumTester> m_vec;
  size_t m_spec;
  ISpectrum &getSpectrumWithoutInvalidation(const size_t index) override {
    m_vec[index].setMatrixWorkspace(this, index);
    return m_vec[index];
  }
};

class WorkspaceTester : public AxeslessWorkspaceTester {
public:
  WorkspaceTester() : AxeslessWorkspaceTester() {}

  const std::string id() const override { return "WorkspaceTester"; }

  /// Returns a clone of the workspace
  std::unique_ptr<WorkspaceTester> clone() const { return std::unique_ptr<WorkspaceTester>(doClone()); }

  /// Returns a default-initialized clone of the workspace
  std::unique_ptr<WorkspaceTester> cloneEmpty() const { return std::unique_ptr<WorkspaceTester>(doCloneEmpty()); }

protected:
  void init(const size_t &numspec, const size_t &j, const size_t &k) override {
    AxeslessWorkspaceTester::init(numspec, j, k);

    // Put an 'empty' axis in to test the getAxis method
    m_axes.resize(2);
    m_axes[0] = std::make_unique<Mantid::API::RefAxis>(this);
    m_axes[1] = std::make_unique<Mantid::API::SpectraAxis>(this);
  }
  void init(const Mantid::HistogramData::Histogram &histogram) override {
    AxeslessWorkspaceTester::init(histogram);

    // Put an 'empty' axis in to test the getAxis method
    m_axes.resize(2);
    m_axes[0] = std::make_unique<Mantid::API::RefAxis>(this);
    m_axes[1] = std::make_unique<Mantid::API::SpectraAxis>(this);
  }

private:
  WorkspaceTester *doClone() const override { return new WorkspaceTester(*this); }
  WorkspaceTester *doCloneEmpty() const override { return new WorkspaceTester(); }
};

//===================================================================================================================
class TableWorkspaceTester : public ITableWorkspace {
public:
  /// Returns a clone of the workspace
  std::unique_ptr<TableWorkspaceTester> clone() const { return std::unique_ptr<TableWorkspaceTester>(doClone()); }

  /// Returns a default-initialized clone of the workspace
  std::unique_ptr<TableWorkspaceTester> cloneEmpty() const {
    return std::unique_ptr<TableWorkspaceTester>(doCloneEmpty());
  }

  const std::string id() const override { return "TableWorkspaceTester"; }

  size_t getMemorySize() const override { throw std::runtime_error("getMemorySize not implemented"); }

  Column_sptr addColumn(const std::string &, const std::string &) override {
    throw std::runtime_error("addColumn not implemented");
  }

  LogManager_sptr logs() override { throw std::runtime_error("logs not implemented"); }

  LogManager_const_sptr getLogs() const override { throw std::runtime_error("getLogs not implemented"); }

  void removeColumn(const std::string &) override { throw std::runtime_error("removeColumn not implemented"); }

  size_t columnCount() const override { throw std::runtime_error("columnCount not implemented"); }

  Column_sptr getColumn(const std::string &) override { throw std::runtime_error("getColumn(str) not implemented"); }

  Column_const_sptr getColumn(const std::string &) const override {
    throw std::runtime_error("getColumn(str) const not implemented");
  }

  Column_sptr getColumn(size_t) override { throw std::runtime_error("getColumn(size_t) not implemented"); }

  Column_const_sptr getColumn(size_t) const override {
    throw std::runtime_error("getColumn(size_t) const not implemented");
  }

  std::vector<std::string> getColumnNames() const override {
    throw std::runtime_error("getColumnNames not implemented");
  }

  size_t rowCount() const override { throw std::runtime_error("rowCount not implemented"); }

  void setRowCount(size_t) override { throw std::runtime_error("setRowCount not implemented"); }

  size_t insertRow(size_t) override { throw std::runtime_error("insertRow not implemented"); }

  void removeRow(size_t) override { throw std::runtime_error("removeRow not implemented"); }

  void find(size_t, size_t &, size_t) override { throw std::runtime_error("find not implemented"); }

  void find(double, size_t &, size_t) override { throw std::runtime_error("find not implemented"); }

  void find(float, size_t &, size_t) override { throw std::runtime_error("find not implemented"); }

  void find(Boolean, size_t &, size_t) override { throw std::runtime_error("find not implemented"); }

  void find(const std::string &, size_t &, size_t) override { throw std::runtime_error("find not implemented"); }

  void find(const Mantid::Kernel::V3D &, size_t &, size_t) override {
    throw std::runtime_error("find not implemented");
  }

private:
  TableWorkspaceTester *doClone() const override {
    throw std::runtime_error("Cloning of TableWorkspaceTester is not implemented.");
  }
  TableWorkspaceTester *doCloneEmpty() const override {
    throw std::runtime_error("Cloning of TableWorkspaceTester is not implemented.");
  }
  ITableWorkspace *doCloneColumns(const std::vector<std::string> &) const override {
    throw std::runtime_error("Cloning of TableWorkspaceTester is not implemented.");
  }
};

//===================================================================================================================
class ColumnTester : public Column {
  size_t size() const override { throw std::runtime_error("size not implemented"); }

  std::type_info &get_type_info() const override { throw std::runtime_error("get_type_info not implemented"); }

  std::type_info &get_pointer_type_info() const override {
    throw std::runtime_error("get_pointer_type_info not implemented");
  }

  void print(size_t, std::ostream &) const override { throw std::runtime_error("print not implemented"); }

  bool isBool() const override { throw std::runtime_error("isBool not implemented"); }

  bool isNumber() const override { throw std::runtime_error("isNumber not implemented"); }

  long int sizeOfData() const override { throw std::runtime_error("sizeOfData not implemented"); }

  Column *clone() const override { throw std::runtime_error("clone not implemented"); }

  double toDouble(size_t) const override { throw std::runtime_error("toDouble not implemented"); }

  void fromDouble(size_t, double) override { throw std::runtime_error("fromDouble not implemented"); }

protected:
  void resize(size_t) override { throw std::runtime_error("resize not implemented"); }
  void insert(size_t) override { throw std::runtime_error("insert not implemented"); }
  void remove(size_t) override { throw std::runtime_error("remove not implemented"); }
  void *void_pointer(size_t) override { throw std::runtime_error("void_pointer not implemented"); }
  const void *void_pointer(size_t) const override { throw std::runtime_error("void_pointer const not implemented"); }
};

//===================================================================================================================
class MDHistoWorkspaceTester : public IMDHistoWorkspace {

public:
  uint64_t getNPoints() const override { throw std::runtime_error("Not Implemented"); }
  uint64_t getNEvents() const override { throw std::runtime_error("Not Implemented"); }

  std::vector<std::unique_ptr<IMDIterator>>
  createIterators(size_t suggestedNumCores = 1,
                  Mantid::Geometry::MDImplicitFunction *function = nullptr) const override {
    UNUSED_ARG(suggestedNumCores)
    UNUSED_ARG(function)
    throw std::runtime_error("Not Implemented");
  }

  signal_t getSignalAtCoord(const coord_t *coords, const Mantid::API::MDNormalization &normalization) const override {
    UNUSED_ARG(coords);
    UNUSED_ARG(normalization);
    throw std::runtime_error("Not Implemented");
  }

  signal_t getSignalWithMaskAtCoord(const coord_t *coords,
                                    const Mantid::API::MDNormalization &normalization) const override {
    UNUSED_ARG(coords);
    UNUSED_ARG(normalization);
    throw std::runtime_error("Not Implemented");
  }

  void setMDMasking(std::unique_ptr<Mantid::Geometry::MDImplicitFunction> maskingRegion) override {
    UNUSED_ARG(maskingRegion);
    throw std::runtime_error("Not Implemented");
  }

  void clearMDMasking() override { throw std::runtime_error("Not Implemented"); }

  SpecialCoordinateSystem getSpecialCoordinateSystem() const override { throw std::runtime_error("Not Implemented"); }

  coord_t getInverseVolume() const override { throw std::runtime_error("Not Implemented"); }

  const signal_t *getSignalArray() const override { throw std::runtime_error("Not Implemented"); }

  const signal_t *getErrorSquaredArray() const override { throw std::runtime_error("Not Implemented"); }

  const signal_t *getNumEventsArray() const override { throw std::runtime_error("Not Implemented"); }

  signal_t *mutableSignalArray() override { throw std::runtime_error("Not Implemented"); }

  signal_t *mutableErrorSquaredArray() override { throw std::runtime_error("Not Implemented"); }

  signal_t *mutableNumEventsArray() override { throw std::runtime_error("Not Implemented"); }

  void setTo(signal_t signal, signal_t errorSquared, signal_t numEvents) override {
    UNUSED_ARG(signal);
    UNUSED_ARG(errorSquared);
    UNUSED_ARG(numEvents);
    throw std::runtime_error("Not Implemented");
  }

  Mantid::Kernel::VMD getCenter(size_t linearIndex) const override {
    UNUSED_ARG(linearIndex);
    throw std::runtime_error("Not Implemented");
  }

  void setSignalAt(size_t index, signal_t value) override {
    UNUSED_ARG(index)
    UNUSED_ARG(value)
    throw std::runtime_error("Not Implemented");
  }

  void setErrorSquaredAt(size_t index, signal_t value) override {
    UNUSED_ARG(index)
    UNUSED_ARG(value)
    throw std::runtime_error("Not Implemented");
  }

  signal_t getErrorAt(size_t index) const override {
    UNUSED_ARG(index)
    throw std::runtime_error("Not Implemented");
  }

  signal_t getErrorAt(size_t index1, size_t index2) const override {
    UNUSED_ARG(index1)
    UNUSED_ARG(index2)
    throw std::runtime_error("Not Implemented");
  }

  signal_t getErrorAt(size_t index1, size_t index2, size_t index3) const override {
    UNUSED_ARG(index1)
    UNUSED_ARG(index2)
    UNUSED_ARG(index3)
    throw std::runtime_error("Not Implemented");
  }

  signal_t getErrorAt(size_t index1, size_t index2, size_t index3, size_t index4) const override {
    UNUSED_ARG(index1)
    UNUSED_ARG(index2)
    UNUSED_ARG(index3)
    UNUSED_ARG(index4)
    throw std::runtime_error("Not Implemented");
  }

  signal_t getSignalAt(size_t index) const override {
    UNUSED_ARG(index)
    throw std::runtime_error("Not Implemented");
  }

  signal_t getSignalAt(size_t index1, size_t index2) const override {
    UNUSED_ARG(index1)
    UNUSED_ARG(index2)
    throw std::runtime_error("Not Implemented");
  }

  signal_t getSignalAt(size_t index1, size_t index2, size_t index3) const override {
    UNUSED_ARG(index1)
    UNUSED_ARG(index2)
    UNUSED_ARG(index3)
    throw std::runtime_error("Not Implemented");
  }

  signal_t getSignalAt(size_t index1, size_t index2, size_t index3, size_t index4) const override {
    UNUSED_ARG(index1)
    UNUSED_ARG(index2)
    UNUSED_ARG(index3)
    UNUSED_ARG(index4)
    throw std::runtime_error("Not Implemented");
  }

  signal_t getSignalNormalizedAt(size_t index) const override {
    UNUSED_ARG(index)
    throw std::runtime_error("Not Implemented");
  }

  signal_t getSignalNormalizedAt(size_t index1, size_t index2) const override {
    UNUSED_ARG(index1)
    UNUSED_ARG(index2)
    throw std::runtime_error("Not Implemented");
  }

  signal_t getSignalNormalizedAt(size_t index1, size_t index2, size_t index3) const override {
    UNUSED_ARG(index1)
    UNUSED_ARG(index2)
    UNUSED_ARG(index3)
    throw std::runtime_error("Not Implemented");
  }

  signal_t getSignalNormalizedAt(size_t index1, size_t index2, size_t index3, size_t index4) const override {
    UNUSED_ARG(index1)
    UNUSED_ARG(index2)
    UNUSED_ARG(index3)
    UNUSED_ARG(index4)
    throw std::runtime_error("Not Implemented");
  }

  signal_t getErrorNormalizedAt(size_t index) const override {
    UNUSED_ARG(index)
    throw std::runtime_error("Not Implemented");
  }

  signal_t getErrorNormalizedAt(size_t index1, size_t index2) const override {
    UNUSED_ARG(index1)
    UNUSED_ARG(index2)
    throw std::runtime_error("Not Implemented");
  }

  signal_t getErrorNormalizedAt(size_t index1, size_t index2, size_t index3) const override {
    UNUSED_ARG(index1)
    UNUSED_ARG(index2)
    UNUSED_ARG(index3)
    throw std::runtime_error("Not Implemented");
  }

  signal_t getErrorNormalizedAt(size_t index1, size_t index2, size_t index3, size_t index4) const override {
    UNUSED_ARG(index1)
    UNUSED_ARG(index2)
    UNUSED_ARG(index3)
    UNUSED_ARG(index4)
    throw std::runtime_error("Not Implemented");
  }

  signal_t &errorSquaredAt(size_t index) override {
    UNUSED_ARG(index)
    throw std::runtime_error("Not Implemented");
  }

  signal_t &signalAt(size_t index) override {
    UNUSED_ARG(index)
    throw std::runtime_error("Not Implemented");
  }

  size_t getLinearIndex(size_t index1, size_t index2) const override {
    UNUSED_ARG(index1)
    UNUSED_ARG(index2)
    throw std::runtime_error("Not Implemented");
  }

  size_t getLinearIndex(size_t index1, size_t index2, size_t index3) const override {
    UNUSED_ARG(index1)
    UNUSED_ARG(index2)
    UNUSED_ARG(index3)
    throw std::runtime_error("Not Implemented");
  }

  size_t getLinearIndex(size_t index1, size_t index2, size_t index3, size_t index4) const override {
    UNUSED_ARG(index1)
    UNUSED_ARG(index2)
    UNUSED_ARG(index3)
    UNUSED_ARG(index4)
    throw std::runtime_error("Not Implemented");
  }

  LinePlot getLineData(const Mantid::Kernel::VMD &start, const Mantid::Kernel::VMD &end,
                       Mantid::API::MDNormalization normalize) const override {
    UNUSED_ARG(start)
    UNUSED_ARG(end)
    UNUSED_ARG(normalize)
    throw std::runtime_error("Not Implemented");
  }

  double &operator[](const size_t &index) override {
    UNUSED_ARG(index)
    throw std::runtime_error("Not Implemented");
  }

  void setCoordinateSystem(const SpecialCoordinateSystem coordinateSystem) override {
    UNUSED_ARG(coordinateSystem)
    throw std::runtime_error("Not Implemented");
  }

  void setDisplayNormalization(const Mantid::API::MDNormalization &preferredNormalization) override {
    UNUSED_ARG(preferredNormalization);
    throw std::runtime_error("Not Implemented");
  }

  size_t getMemorySize() const override { throw std::runtime_error("Not Implemented"); }

  const std::string id() const override { throw std::runtime_error("Not Implemented"); }
  const std::string &getName() const override { throw std::runtime_error("Not Implemented"); }
  bool threadSafe() const override { throw std::runtime_error("Not Implemented"); }
  const std::string toString() const override { throw std::runtime_error("Not Implemented"); }
  MDHistoWorkspaceTester(const MDHistoDimension_sptr &dimX, const MDHistoDimension_sptr &dimY,
                         const MDHistoDimension_sptr &dimZ) {
    std::vector<IMDDimension_sptr> dimensions{dimX, dimY, dimZ};
    initGeometry(dimensions);
  }

private:
  IMDHistoWorkspace *doClone() const override { throw std::runtime_error("Not Implemented"); }
  IMDHistoWorkspace *doCloneEmpty() const override { throw std::runtime_error("Not Implemented"); }
};

class VariableBinThrowingTester : public AxeslessWorkspaceTester {
  size_t blocksize() const override {
    if (getSpectrum(0).dataY().size() == getSpectrum(1).dataY().size())
      return getSpectrum(0).dataY().size();
    else
      throw std::length_error("Mismatched bins sizes");

    return 0;
  }
};
