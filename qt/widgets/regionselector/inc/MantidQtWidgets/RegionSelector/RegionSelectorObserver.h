// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/RegionSelector/DllConfig.h"

namespace MantidQt::Widgets {
class MANTID_REGIONSELECTOR_DLL RegionSelectorObserver {
public:
  virtual ~RegionSelectorObserver() = default;
  virtual void notifyRegionChanged() = 0;
};
} // namespace MantidQt::Widgets