#ifndef MANTID_HISTOGRAMDATA_HISTOGRAMITEM_H_
#define MANTID_HISTOGRAMDATA_HISTOGRAMITEM_H_

#include "MantidHistogramData/BinEdges.h"
#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/Points.h"

#include <utility>

namespace Mantid {
namespace HistogramData {

// forward declare Histogram
class Histogram;

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
  double counts() const;
  double countVariance() const;
  double countStandardDeviation() const;
  double frequency() const;
  double frequencyVariance() const;
  double frequencyStandardDeviation() const;
  double width() const;
  double center() const;

  void incrementIndex();
  void advance(int64_t delta);

  void decrementIndex() {
    if (m_index > 0) {
      --m_index;
    }
  }

  size_t getIndex() const { return m_index; }

  void setIndex(const size_t index) { m_index = index; }

private:
  friend class HistogramIterator;
  /// Private constructor, can only be created by HistogramIterator
  HistogramItem(const Histogram &histogram, const size_t index);
  /// Get a refernce to the histogram
  const Histogram &histogramRef() const;
  /// Check if is points or bins
  bool xModeIsPoints() const;
  /// Check if is counts or frequencies
  bool yModeIsCounts() const;

  // Deleted assignment operator as a HistogramItem is not copyable
  HistogramItem operator=(const HistogramItem &) = delete;

  const Histogram &m_histogram;
  size_t m_index;
};

} // namespace HistogramData
} // namespace Mantid

#endif
