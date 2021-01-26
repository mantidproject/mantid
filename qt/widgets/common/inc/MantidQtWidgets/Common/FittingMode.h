// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"

namespace MantidQt {
namespace MantidWidgets {

/*!
 * FittingMode can be used to specify what mode you are using for a fit. It is
 * also used by the FitPropertyBrowser to either allow the selection of
 * sequential fitting (SEQUENTIAL), simultaneous fitting (SIMULTANEOUS) or both
 * types (SIMULTANEOUS_SEQUENTIAL).
 */
enum EXPORT_OPT_MANTIDQT_COMMON FittingMode {
  SIMULTANEOUS = 0,
  SEQUENTIAL = 1,
  SIMULTANEOUS_SEQUENTIAL = 2
};

} // namespace MantidWidgets
} // namespace MantidQt
