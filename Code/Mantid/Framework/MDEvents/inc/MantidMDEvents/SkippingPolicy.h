#ifndef MANTID_MDEVENTS_SKIPPINGPOLICY_H_
#define MANTID_MDEVENTS_SKIPPINGPOLICY_H_

#include "MantidKernel/System.h"
#include "MantidMDEvents/SkippingPolicy.h"
#include "MantidAPI/IMDIterator.h"
#include <boost/scoped_ptr.hpp>

namespace Mantid {
namespace MDEvents {

/** SkippingPolicy : Policy types for skipping in MDiterators.

  @date 2012-03-05

  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

class DLLExport SkippingPolicy {
public:
  virtual bool keepGoing() const = 0;
  virtual ~SkippingPolicy(){};
};

/// Policy that indicates skipping of masked bins.
class DLLExport SkipMaskedBins : public SkippingPolicy {
private:
  Mantid::API::IMDIterator *m_iterator;

public:
  SkipMaskedBins(Mantid::API::IMDIterator *const iterator)
      : m_iterator(iterator) {}
  /**
  Keep going as long as the current iterator bin is masked.
  @return True to keep going.
  */
  bool keepGoing() const { return m_iterator->getIsMasked(); };
  virtual ~SkipMaskedBins() {}
};

/// Policy that indicates no skipping should be applied.
class DLLExport SkipNothing : public SkippingPolicy {
public:
  /**
  Always returns false to cancel skipping.
  @return false to cancel continuation
  */
  bool keepGoing() const { return false; }
  virtual ~SkipNothing() {}
};

typedef boost::scoped_ptr<SkippingPolicy> SkippingPolicy_scptr;

} // namespace MDEvents
} // namespace Mantid

#endif /* MANTID_MDEVENTS_SKIPPINGPOLICY_H_ */
