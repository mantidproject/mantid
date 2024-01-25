// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/NoShape.h"
#include <boost/python/class.hpp>

using namespace boost::python;
using namespace Mantid::DataObjects;

void export_NoShape() { class_<NoShape, bases<Mantid::Geometry::PeakShape>, boost::noncopyable>("NoShape"); }
