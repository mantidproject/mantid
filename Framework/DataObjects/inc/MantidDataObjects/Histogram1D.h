#ifndef MANTID_DATAOBJECTS_HISTOGRAM1D_H_
#define MANTID_DATAOBJECTS_HISTOGRAM1D_H_

#include "MantidAPI/ISpectrum.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace DataObjects {
/**
  1D histogram implementation.

  Copyright &copy; 2007-2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>.
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport Histogram1D : public Mantid::API::ISpectrum {
private:
  /// Histogram object holding the histogram data. Currently only X and Dx.
  HistogramData::Histogram m_histogram;

protected:
  MantidVecPtr refY; ///< RefCounted Y
  MantidVecPtr refE; ///< RefCounted Error

public:
  Histogram1D(HistogramData::Histogram::XMode mode)
      : API::ISpectrum(), m_histogram(mode) {}

  void setX(const Kernel::cow_ptr<HistogramData::HistogramX> &X) override;
  MantidVec &dataX() override;
  MantidVec &dataDx() override;
  const MantidVec &dataX() const override;
  const MantidVec &dataDx() const override;
  const MantidVec &readX() const override;
  const MantidVec &readDx() const override;
  Kernel::cow_ptr<HistogramData::HistogramX> ptrX() const override;

  /// Sets the data.
  void setData(const MantidVec &Y) override { refY.access() = Y; };
  /// Sets the data and errors
  void setData(const MantidVec &Y, const MantidVec &E) override {
    refY.access() = Y;
    refE.access() = E;
  }

  /// Sets the data.
  void setData(const MantidVecPtr &Y) override { refY = Y; }
  /// Sets the data and errors
  void setData(const MantidVecPtr &Y, const MantidVecPtr &E) override {
    refY = Y;
    refE = E;
  }

  /// Sets the data.
  void setData(const MantidVecPtr::ptr_type &Y) override { refY = Y; }
  /// Sets the data and errors
  void setData(const MantidVecPtr::ptr_type &Y,
               const MantidVecPtr::ptr_type &E) override {
    refY = Y;
    refE = E;
  }

  /// Zero the data (Y&E) in this spectrum
  void clearData() override;

  // Get the array data
  /// Returns the y data const
  const MantidVec &dataY() const override { return *refY; }
  /// Returns the error data const
  const MantidVec &dataE() const override { return *refE; }

  /// Returns the y data
  MantidVec &dataY() override { return refY.access(); }
  /// Returns the error data
  MantidVec &dataE() override { return refE.access(); }

  virtual std::size_t size() const { return refY->size(); } ///< get pseudo size

  /// Checks for errors
  bool isError() const { return refE->empty(); }

  /// Gets the memory size of the histogram
  size_t getMemorySize() const override {
    return ((readX().size() + refY->size() + refE->size()) * sizeof(double));
  }

  void setHistogram(const HistogramData::Histogram &other) {
    mutableHistogramRef() = other;
  }

private:
  const HistogramData::Histogram &histogramRef() const override {
    return m_histogram;
  }
  HistogramData::Histogram &mutableHistogramRef() override {
    return m_histogram;
  }
};

} // namespace DataObjects
} // Namespace Mantid
#endif /*MANTID_DATAOBJECTS_HISTOGRAM1D_H_*/
