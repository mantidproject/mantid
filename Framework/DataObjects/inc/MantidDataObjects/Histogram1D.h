#ifndef MANTID_DATAOBJECTS_HISTOGRAM1D_H_
#define MANTID_DATAOBJECTS_HISTOGRAM1D_H_

#include "MantidAPI/ISpectrum.h"
#include "MantidKernel/System.h"
#include "MantidKernel/cow_ptr.h"

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
  Histogram1D(HistogramData::Histogram::XMode xmode,
              HistogramData::Histogram::YMode ymode);

  Histogram1D(const Histogram1D &) = default;
  Histogram1D(Histogram1D &&) = default;
  Histogram1D(const ISpectrum &other);

  Histogram1D &operator=(const Histogram1D &) = default;
  Histogram1D &operator=(Histogram1D &&) = default;
  Histogram1D &operator=(const ISpectrum &rhs);

  void copyDataFrom(const ISpectrum &source) override;

  void setX(const Kernel::cow_ptr<HistogramData::HistogramX> &X) override;
  MantidVec &dataX() override;
  const MantidVec &dataX() const override;
  const MantidVec &readX() const override;
  Kernel::cow_ptr<HistogramData::HistogramX> ptrX() const override;

  MantidVec &dataDx() override;
  const MantidVec &dataDx() const override;
  const MantidVec &readDx() const override;

  /// Zero the data (Y&E) in this spectrum
  void clearData() override;

  /// Deprecated, use y() instead. Returns the y data const
  const MantidVec &dataY() const override { return m_histogram.dataY(); }
  /// Deprecated, use e() instead. Returns the error data const
  const MantidVec &dataE() const override { return m_histogram.dataE(); }

  /// Deprecated, use mutableY() instead. Returns the y data
  MantidVec &dataY() override { return m_histogram.dataY(); }
  /// Deprecated, use mutableE() instead. Returns the error data
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

private:
  using ISpectrum::copyDataInto;
  void copyDataInto(Histogram1D &sink) const override;

  void checkAndSanitizeHistogram(HistogramData::Histogram &histogram) override;
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
