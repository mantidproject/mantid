// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/ISpectrum.h"
#include "MantidDataObjects/DllConfig.h"
#include "MantidKernel/cow_ptr.h"

namespace Mantid {
namespace DataObjects {
/**
  1D histogram implementation.
*/
class MANTID_DATAOBJECTS_DLL Histogram1D : public Mantid::API::ISpectrum {
private:
  /// Histogram object holding the histogram data.
  HistogramData::Histogram m_histogram;

public:
  Histogram1D(HistogramData::Histogram::XMode xmode, HistogramData::Histogram::YMode ymode);

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

  virtual std::size_t size() const { return m_histogram.readY().size(); } ///< get pseudo size

  /// Checks for errors
  bool isError() const { return readE().empty(); }

  /// Gets the memory size of the histogram
  size_t getMemorySize() const override {
    return ((readX().size() + readY().size() + readE().size()) * sizeof(double));
  }

private:
  using ISpectrum::copyDataInto;
  void copyDataInto(Histogram1D &sink) const override;

  void checkAndSanitizeHistogram(HistogramData::Histogram &histogram) override;
  const HistogramData::Histogram &histogramRef() const override { return m_histogram; }
  HistogramData::Histogram &mutableHistogramRef() override { return m_histogram; }
};

} // namespace DataObjects
} // Namespace Mantid
