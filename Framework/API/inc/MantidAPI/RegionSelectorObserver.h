// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"

namespace Mantid::API {
class MANTID_API_DLL RegionSelectorObserver {
public:
  virtual ~RegionSelectorObserver();
  virtual void notifyRegionChanged() = 0;
};
} // namespace Mantid::API
