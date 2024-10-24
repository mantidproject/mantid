// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidReflectometry/ConvertToReflectometryQ.h"
#include "MantidReflectometry/DllConfig.h"

namespace Mantid {
namespace Reflectometry {

class MANTID_REFLECTOMETRY_DLL ConvertToReflectometryQ2 : public ConvertToReflectometryQ {
public:
  // All we need to do in this v2 class is return the version number. This is passed through to ReflectometryTransform,
  // which does the work.
  int version() const override;
};
} // namespace Reflectometry
} // namespace Mantid
