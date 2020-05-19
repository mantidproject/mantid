// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include <tuple>

/**
 * Creates an object containing information about converting display coordinates
 *to data coordinates within a workspace
 **/

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_COMMON CoordinateConversion {
public:
  virtual std::tuple<double, double>
  toDataCoord(const double xDisplayCoord, const double yDisplayCoord) const = 0;
};
} // namespace MantidWidgets
} // namespace MantidQt
