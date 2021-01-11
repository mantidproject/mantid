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

enum EXPORT_OPT_MANTIDQT_COMMON FittingMode {
  SIMULTANEOUS = 0,
  SEQUENTIAL = 1,
  SIMULTANEOUS_SEQUENTIAL = 2
};

}
} // namespace MantidQt
