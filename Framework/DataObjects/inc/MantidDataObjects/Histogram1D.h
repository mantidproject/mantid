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
  /// Histogram object holding the histogram data.
  HistogramData::Histogram m_histogram;

public:
  Histogram1D(HistogramData::Histogram::XMode mode)
      : API::ISpectrum(), m_histogram(mode) {}

  void setX(const Kernel::cow_ptr<HistogramData::HistogramX> &X) override;
  MantidVec &dataX() override;
  const MantidVec &dataX() const override;
  const MantidVec &readX() const override;
  Kernel::cow_ptr<HistogramData::HistogramX> ptrX() const override;

  /// Sets the data.
  void setData(const MantidVec &Y) override { m_histogram.dataY() = Y; };
  /// Sets the data and errors
  void setData(const MantidVec &Y, const MantidVec &E) override {
    m_histogram.dataY() = Y;
    m_histogram.dataE() = E;
  }

  /// Sets the data.
  void setData(const Kernel::cow_ptr<HistogramData::HistogramY> &Y) override {
    m_histogram.setY(Y);
  }
  /// Sets the data and errors
  void setData(const Kernel::cow_ptr<HistogramData::HistogramY> &Y,
               const Kernel::cow_ptr<HistogramData::HistogramE> &E) override {
    m_histogram.setY(Y);
    m_histogram.setE(E);
  }

  /// Zero the data (Y&E) in this spectrum
  void clearData() override;

  // Get the array data
  /// Returns the y data const
  const MantidVec &dataY() const override { return m_histogram.dataY(); }
  /// Returns the error data const
  const MantidVec &dataE() const override { return m_histogram.dataE(); }

  /// Returns the y data
  MantidVec &dataY() override { return m_histogram.dataY(); }
  /// Returns the error data
  MantidVec &dataE() override { return m_histogram.dataE(); }

  virtual std::size_t size() const {
    return m_histogram.readY().size();
  } ///< get pseudo size

  /// Checks for errors
  bool isError() const { return readE().empty(); }

  /// Gets the memory size of the histogram
  size_t getMemorySize() const override {
    return ((readX().size() + readY().size() + readE().size()) *
            sizeof(double));
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
