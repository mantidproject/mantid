// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Crystal/PeakTransform.h"
#include "MantidKernel/System.h"
#include <memory>

namespace Mantid {
namespace Geometry {
/**
@class PeakTransformFactory
Abstract type defining Factory Method interface for generating PeakTransforms
*/
class DLLExport PeakTransformFactory {
public:
  virtual PeakTransform_sptr createDefaultTransform() const = 0;
  virtual PeakTransform_sptr createTransform(const std::string &xPlotLabel, const std::string &yPlotLabel) const = 0;
  virtual ~PeakTransformFactory() = default;
};

/// Factory Shared Pointer typedef.
using PeakTransformFactory_sptr = std::shared_ptr<PeakTransformFactory>;
} // namespace Geometry
} // namespace Mantid
