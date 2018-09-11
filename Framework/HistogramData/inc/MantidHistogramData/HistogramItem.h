#ifndef MANTID_HISTOGRAMDATA_HISTOGRAMITEM_H_
#define MANTID_HISTOGRAMDATA_HISTOGRAMITEM_H_

#include "MantidHistogramData/BinEdges.h"
#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/Points.h"

#include <utility>

namespace Mantid {
namespace HistogramData {

/** HistogramItem

  HistogramItem represents a single index in a Histogram object.

  HistogramItem is the type that is returned when iterating over a
  Histogram using the foreach loop syntax. HistogramItem provides
  efficient access to a single point in the Histogram.

  HistogramItem will only perform conversions between counts and
  frequencies or points and bins when explicitly told to. Code that
  requires only a few values from a large Histogram may find this faster
  than converting the whole X, Y or E value.

  @author Samuel Jackson
  @date 2017

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_HISTOGRAMDATA_DLL HistogramItem {

public:
  double center() const {
    const auto &x = m_histogram.x();
    if (xModeIsPoints()) {
      return x[m_index];
    } else {
      return 0.5 * (x[m_index + 1] + x[m_index]);
    }
  }

  double binWidth() const {
    const auto &x = m_histogram.x();
    if (xModeIsPoints()) {
      auto numPoints = m_histogram.size();
      if (m_index == 0) {
        // first point
        return x[1] - x[0];
      } else if (m_index == numPoints - 1) {
        // last point
        return x[m_index] - x[m_index - 1];
      } else {
        // everything inbetween
        return 0.5 * (x[m_index + 1] - x[m_index - 1]);
      }
    } else {
      return x[m_index + 1] - x[m_index];
    }
  }

  double counts() const {
    const auto &y = m_histogram.y();
    if (yModeIsCounts()) {
      return y[m_index];
    } else {
      return y[m_index] * binWidth();
    }
  }

  double countVariance() const {
    const auto &e = m_histogram.e();
    if (yModeIsCounts()) {
      return e[m_index] * e[m_index];
    } else {
      const auto width = binWidth();
      return e[m_index] * e[m_index] * width * width;
    }
  }

  double countStandardDeviation() const {
    const auto &e = m_histogram.e();
    if (yModeIsCounts()) {
      return e[m_index];
    } else {
      const auto width = binWidth();
      return e[m_index] * width;
    }
  }

  double frequency() const {
    const auto &y = m_histogram.y();
    if (yModeIsCounts()) {
      return y[m_index] / binWidth();
    } else {
      return y[m_index];
    }
  }

  double frequencyVariance() const {
    const auto &e = m_histogram.e();
    if (!yModeIsCounts()) {
      return e[m_index] * e[m_index];
    } else {
      const auto width = binWidth();
      return (e[m_index] * e[m_index]) / (width * width);
    }
  }

  double frequencyStandardDeviation() const {
    const auto &e = m_histogram.e();
    if (!yModeIsCounts()) {
      return e[m_index];
    } else {
      const auto width = binWidth();
      return e[m_index] / width;
    }
  }

private:
  friend class HistogramIterator;

  /// Private constructor, can only be created by HistogramIterator
  HistogramItem(const Histogram &histogram, const size_t index)
      : m_histogram(histogram), m_index(index) {}

  bool xModeIsPoints() const {
    return Histogram::XMode::Points == m_histogram.xMode();
  }

  bool yModeIsCounts() const {
    return Histogram::YMode::Counts == m_histogram.yMode();
  }
  // Deleted assignment operator as a HistogramItem is not copyable
  HistogramItem operator=(const HistogramItem &) = delete;

  const Histogram &m_histogram;
  size_t m_index;
};

} // namespace HistogramData
} // namespace Mantid

#endif
