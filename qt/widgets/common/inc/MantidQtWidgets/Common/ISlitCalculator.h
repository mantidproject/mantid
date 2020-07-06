// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include <string>

namespace MantidQt {
namespace MantidWidgets {
class EXPORT_OPT_MANTIDQT_COMMON ISlitCalculator {
public:
  virtual ~ISlitCalculator() = default;
  virtual void setCurrentInstrumentName(std::string instrumentName) = 0;
  virtual void processInstrumentHasBeenChanged() = 0;
  virtual void show() = 0;
};
} // namespace MantidWidgets
} // namespace MantidQt
