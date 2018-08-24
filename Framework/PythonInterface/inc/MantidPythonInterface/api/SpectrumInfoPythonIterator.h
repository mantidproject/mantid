#ifndef MANTID_PYTHONINTERFACE_SPECTRUMINFOPYTHONITERATOR_H_
#define MANTID_PYTHONINTERFACE_SPECTRUMINFOPYTHONITERATOR_H_

#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/SpectrumInfoItem.h"
#include "MantidAPI/SpectrumInfoIterator.h"

#include <boost/python/iterator.hpp>

using Mantid::API::SpectrumInfo;
using Mantid::API::SpectrumInfoItem;
using Mantid::API::SpectrumInfoIterator;
using namespace boost::python;

namespace Mantid {
namespace PythonInterface {

/** SpectrumInfoPythonIterator

SpectrumInfoPythonIterator is used to expose SpectrumInfoIterator to the Python
side. From Python the user will be able to use more pythonic loop syntax to
access data such as:
- isMonitor()
- isMaksed()
- twoTheta()
- signedTwoTheta()
- l2()
- hasUniqueDetector()
- spectrumDefinition()
- position()
without the need for indexes.

@author Bhuvan Bezawada, STFC
@date 2018

Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

class SpectrumInfoPythonIterator {
public:
  explicit SpectrumInfoPythonIterator(const SpectrumInfo &spectrumInfo)
      : m_begin(spectrumInfo.begin()), m_end(spectrumInfo.end()),
        m_firstOrDone(true) {}

  const SpectrumInfoItem &next() {
    if (!m_firstOrDone) {
      ++m_begin;
    } else {
      m_firstOrDone = false;
      objects::stop_iteration_error();
    }
    return *m_begin;
  }

private:
  SpectrumInfoIterator m_begin;
  SpectrumInfoIterator m_end;
  bool m_firstOrDone;
};

} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINTERFACE_SPECTRUMINFOPYTHONITERATOR_H_ */
