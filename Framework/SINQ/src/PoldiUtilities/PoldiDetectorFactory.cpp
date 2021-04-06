// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidSINQ/PoldiUtilities/PoldiDetectorFactory.h"

#include "MantidSINQ/PoldiUtilities/PoldiHeliumDetector.h"

namespace Mantid {
namespace Poldi {
using namespace boost::gregorian;

PoldiDetectorFactory::PoldiDetectorFactory() : m_newDetectorDate(from_string(std::string("2016/01/01"))) {}

PoldiAbstractDetector *PoldiDetectorFactory::createDetector(std::string detectorType) {
  UNUSED_ARG(detectorType);

  return new PoldiHeliumDetector();
}

PoldiAbstractDetector *PoldiDetectorFactory::createDetector(date experimentDate) {
  if (experimentDate < m_newDetectorDate) {
    return new PoldiHeliumDetector();
  }

  return nullptr;
}

} // namespace Poldi
} // namespace Mantid
