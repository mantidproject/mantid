// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/LabFrameSample.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/Run.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidGeometry/Objects/ShapeRotation.h"
#include "MantidKernel/Logger.h"

namespace Mantid::API {

namespace {
Kernel::Logger g_log("LabFrameSample");
} // namespace

Geometry::IObject_sptr labFrameSampleShape(const ExperimentInfo &experiment) {
  const auto &sample = experiment.sample();
  const auto &goniometerR = experiment.run().getGoniometer().getR();
  if (Geometry::outstandingGoniometerRotation(sample.getShape(), goniometerR) == Kernel::Matrix<double>(3, 3, true)) {
    return sample.getShapePtr();
  }
  if (sample.hasEnvironment()) {
    g_log.warning("The sample environment is not currently rotated by the goniometer, so the goniometer "
                  "rotation is not applied to the sample in it either. The sample is used as defined.");
    return sample.getShapePtr();
  }
  return Geometry::getLabFrameShape(sample.getShape(), goniometerR);
}

Sample labFrameSample(const ExperimentInfo &experiment) {
  Sample sample(experiment.sample());
  sample.setShape(labFrameSampleShape(experiment));
  return sample;
}

} // namespace Mantid::API
