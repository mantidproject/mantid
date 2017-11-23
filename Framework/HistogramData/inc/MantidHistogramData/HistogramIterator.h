
#ifndef MANTID_HISTOGRAMDATA_HISTOGRAMITERATOR_H_
#define MANTID_HISTOGRAMDATA_HISTOGRAMITERATOR_H_

#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/HistogramItem.h"

#include <boost/iterator/iterator_facade.hpp>
#include <memory>

namespace Mantid {
namespace HistogramData {

class Histogram;

/** HistogramIterator

  HistogramIterator implements an the iterator interface for HistogramData.
  At each position the iterator will point to an instance of a HistogramItem.
  This item provides direct access to the values at a particular index.

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
class MANTID_HISTOGRAMDATA_DLL HistogramIterator
    : public boost::iterator_facade<HistogramIterator, const HistogramItem &,
                                    boost::bidirectional_traversal_tag> {

public:
  HistogramIterator(const Histogram &histogram, const size_t index)
      : m_item(histogram, index){};

private:
  friend class boost::iterator_core_access;

  void increment() { m_item.incrementIndex(); }

  bool equal(const HistogramIterator &other) const {
    return m_item.getIndex() == other.m_item.getIndex();
  }

  const HistogramItem &dereference() const { return m_item; }

  void decrement() { m_item.decrementIndex(); }

  void advance(int64_t delta) { m_item.advance(delta); }

  uint64_t distance_to(const HistogramIterator &other) const {
    return static_cast<uint64_t>(other.m_item.getIndex()) -
           static_cast<uint64_t>(m_item.getIndex());
  }

  HistogramItem m_item;
};

} // namespace HistogramData
} // namespace Mantid

#endif
