//
// Created by jbq on 4/20/21.
//
// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/SpecialCoordinateSystem.h"

#include <boost/python/enum.hpp>

void export_SpecialCoordinateSystem() {
  boost::python::enum_<Mantid::Kernel::SpecialCoordinateSystem>("SpecialCoordinateSystem")
      .value("NONE", Mantid::Kernel::None)
      .value("QLab", Mantid::Kernel::QLab)
      .value("QSample", Mantid::Kernel::QSample)
      .value("HKL", Mantid::Kernel::HKL);
}
