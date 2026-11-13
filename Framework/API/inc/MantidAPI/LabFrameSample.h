// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/Sample.h"
#include "MantidGeometry/Objects/IObject.h"

/* The sample as an experiment's goniometer places it in the lab frame, for algorithms that combine
 * it with lab-frame beams, detectors or gauge volumes. Built on Geometry::getLabFrameShape.
 *
 * A sample that sits in an environment is left as defined, with a warning: the environment is not
 * currently goniometer-rotated, so turning the sample alone would push it through the container's walls.
 * SetSample declines to bake the goniometer in for the same reason.
 */

namespace Mantid::API {
class ExperimentInfo;

/// The sample shape in the lab frame: the sample's own shape, uncopied, when there is nothing to rotate.
MANTID_API_DLL Geometry::IObject_sptr labFrameSampleShape(const ExperimentInfo &experiment);

/// A copy of the sample carrying labFrameSampleShape, for code that takes a whole Sample.
MANTID_API_DLL Sample labFrameSample(const ExperimentInfo &experiment);

} // namespace Mantid::API
