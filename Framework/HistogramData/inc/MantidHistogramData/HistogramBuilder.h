// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/Histogram.h"

namespace Mantid {
namespace HistogramData {

/** HistogramBuilder is a helper for constructing Histograms based on "legacy
  style" information such as x-length, y-length, and a "distribution" flag. If
  the actual types of x and y (such as BinEdges and Counts) are known the
  constructor of Histogram should be used instead of the builder.

  @author Simon Heybrock
  @date 2017
*/
class MANTID_HISTOGRAMDATA_DLL HistogramBuilder {
public:
  /// Sets X information. Can be a length or actual X data.
  template <typename... T> void setX(T &&...data) { m_x = Kernel::make_cow<HistogramX>(std::forward<T>(data)...); }
  /// Sets Y information. Can be a length or actual Y data.
  template <typename... T> void setY(T &&...data) { m_y = Kernel::make_cow<HistogramY>(std::forward<T>(data)...); }
  /// Sets E information. Can be a length or actual E data.
  template <typename... T> void setE(T &&...data) { m_e = Kernel::make_cow<HistogramE>(std::forward<T>(data)...); }
  /// Sets Dx information. Can be a length or actual Dx data.
  template <typename... T> void setDx(T &&...data) { d_x = Kernel::make_cow<HistogramDx>(std::forward<T>(data)...); }
  void setDistribution(bool isDistribution);

  Histogram build() const;

private:
  bool m_isDistribution{false};
  Kernel::cow_ptr<HistogramX> m_x{nullptr};
  Kernel::cow_ptr<HistogramY> m_y{nullptr};
  Kernel::cow_ptr<HistogramE> m_e{nullptr};
  Kernel::cow_ptr<HistogramDx> d_x{nullptr};
};

} // namespace HistogramData
} // namespace Mantid
