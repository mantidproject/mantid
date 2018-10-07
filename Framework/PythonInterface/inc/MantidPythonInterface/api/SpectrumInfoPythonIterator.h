// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
*/

class SpectrumInfoPythonIterator {
public:
  explicit SpectrumInfoPythonIterator(SpectrumInfo &spectrumInfo)
      : m_begin(spectrumInfo.begin()), m_end(spectrumInfo.end()),
        m_firstOrDone(true) {}

  const SpectrumInfoItem<SpectrumInfo> &next() {
    if (!m_firstOrDone)
      ++m_begin;
    else
      m_firstOrDone = false;
    if (m_begin == m_end) {
      m_firstOrDone = true;
      objects::stop_iteration_error();
    }
    return *m_begin;
  }

private:
  SpectrumInfoIterator<SpectrumInfo> m_begin;
  SpectrumInfoIterator<SpectrumInfo> m_end;
  bool m_firstOrDone;
};

} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINTERFACE_SPECTRUMINFOPYTHONITERATOR_H_ */
