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
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidGeometry/Objects/ShapeRotation.h"

namespace Mantid::API {

Geometry::IObject_sptr labFrameSampleShape(const ExperimentInfo &experiment) {
  const auto &sample = experiment.sample();
  const auto &goniometerR = experiment.run().getGoniometer().getR();
  if (Geometry::outstandingGoniometerRotation(sample.getShape(), goniometerR) == Kernel::Matrix<double>(3, 3, true)) {
    return sample.getShapePtr();
  }
  return Geometry::getLabFrameShape(sample.getShape(), goniometerR);
}

Sample labFrameSample(const ExperimentInfo &experiment) {
  Sample sample(experiment.sample());
  sample.setShape(labFrameSampleShape(experiment));
  if (sample.hasEnvironment()) {
    const auto &environment = sample.getEnvironment();
    const auto &goniometerR = experiment.run().getGoniometer().getR();
    // An own-frame environment is already where an identity goniometer puts it; a view may not be.
    if (goniometerR != Kernel::Matrix<double>(3, 3, true) || &environment.ownFrame() != &environment)
      sample.setEnvironment(environment.inLabFrame(goniometerR));
  }
  return sample;
}

} // namespace Mantid::API
