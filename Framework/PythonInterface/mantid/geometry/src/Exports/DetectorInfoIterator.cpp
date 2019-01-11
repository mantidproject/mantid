// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/DetectorInfoIterator.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"

#include <boost/python/class.hpp>
#include <boost/python/module.hpp>

using Mantid::Geometry::DetectorInfo;
using Mantid::Geometry::DetectorInfoIterator;
using namespace boost::python;

// Export DetectorInfoIterator
void export_DetectorInfoIterator() {

  // Export to Python
  class_<DetectorInfoIterator<DetectorInfo>>("DetectorInfoIterator", no_init);
}
