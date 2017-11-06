
#ifndef MANTID_HISTOGRAMDATA_HISTOGRAMITERATOR_H_
#define MANTID_HISTOGRAMDATA_HISTOGRAMITERATOR_H_

#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/HistogramItem.h"
#include "MantidKernel/make_unique.h"

#include <boost/iterator/iterator_facade.hpp>
#include <memory>

namespace Mantid {
namespace HistogramData {

/** HistogramIterator

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
    : public boost::iterator_facade<HistogramIterator, HistogramItem, boost::bidirectional_traversal_tag> {

public:
    HistogramIterator(const Histogram &histogram, const size_t index = 0)
        : m_histogram(histogram), m_index(index), 
        m_currentItem(Kernel::make_unique<HistogramItem>(histogram, index)) {};

private:
    friend class boost::iterator_core_access;

    void increment();
    bool equal(const HistogramIterator &other) const;
    HistogramItem& dereference() const;
    void decrement();
    void advance(uint64_t delta);
    uint64_t distance_to(const HistogramIterator &other) const;

    const Histogram& m_histogram;
    size_t m_index;
    std::unique_ptr<HistogramItem> m_currentItem;
};

}
}

#endif

