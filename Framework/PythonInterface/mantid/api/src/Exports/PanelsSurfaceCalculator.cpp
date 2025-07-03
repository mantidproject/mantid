// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/PanelsSurfaceCalculator.h"
#include <boost/python/class.hpp>

void export_PanelsSurfaceCalculator() {
  using namespace boost::python;
  using Mantid::API::PanelsSurfaceCalculator;

  class_<PanelsSurfaceCalculator, boost::noncopyable>("PanelsSurfaceCalculator", no_init);
}
