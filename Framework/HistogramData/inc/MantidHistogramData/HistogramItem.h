#ifndef MANTID_HISTOGRAMDATA_HISTOGRAMITEM_H_
#define MANTID_HISTOGRAMDATA_HISTOGRAMITEM_H_

#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/BinEdges.h"
#include "MantidHistogramData/Points.h"
#include <utility>

namespace Mantid {
namespace HistogramData {

// forward declare Histogram    
class Histogram;

/** HistogramItem

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
  HistogramItem(const Histogram &histogram, const size_t index);

  double counts() const;
  double countVariance() const;
  double countStandardDeviation() const;
  double frequency() const;
  double frequencyVariance() const;
  double frequencyStandardDeviation() const;
  double center() const;
  double width() const;
  const BinEdges binEdges() const;
  const Points point() const;

private:
  const Histogram &histogramRef() const;
  // Deleted copy & assignment operators as a HistogramItem is not copyable
  HistogramItem(const HistogramItem &) = delete;
  HistogramItem operator=(const HistogramItem &) = delete;

  const Histogram &m_histogram;
  const size_t m_index;
};

} // namespace HistogramData
} // namespace Mantid

#endif
