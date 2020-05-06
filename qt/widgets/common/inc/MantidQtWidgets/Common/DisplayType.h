// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"

/**
 * Creates an object containing information about converting display coordinates
 *to data coordinates within a workspace
 **/

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_COMMON DisplayType {
public:
  DisplayType(){};
  ~DisplayType(){};

  virtual void convertToDataCoord(const double xDisplayCoord,
                                  const double yDisplayCoord,
                                  double &xDataCoord, double &yDataCoord) {
    xDataCoord = xDisplayCoord;
    yDataCoord = yDisplayCoord;
  };
};
} // namespace MantidWidgets
} // namespace MantidQt
