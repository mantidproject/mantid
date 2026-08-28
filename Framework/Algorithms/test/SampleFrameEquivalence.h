// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

/** Helpers for asserting that the two ways of orienting a sample agree.
 *
 * A workspace can carry its sample shape in either frame and look identical from the run:
 *
 *   own frame - the shape is as defined and the goniometer on the run says where it points.
 *               This is what SetGoniometer alone leaves behind.
 *   lab frame - the rotation has already been baked into the shape definition, and the run
 *               carries the same goniometer. This is what CopySample leaves behind.
 *
 * They describe the same experiment, so any algorithm that consumes the sample shape must give the
 * same answer for both. An algorithm that ignores the goniometer silently uses the unrotated shape
 * and the two disagree; one that applies it unconditionally rotates the baked shape twice and they
 * disagree the other way.
 *
 * The lab-frame shape here is baked with a bare <goniometer> tag, which ShapeFactory reads as a full
 * bake through its legacy fallback. That is deliberately independent of Geometry::getLabFrameShape,
 * so these tests cannot pass by agreeing with the code they are checking.
 */

#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/NeutronAtom.h"

#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

namespace SampleFrameEquivalence {

/// Rotation about x, which tilts a shape standing along y - a cylinder, say - towards the beam.
inline Mantid::Kernel::Matrix<double> rotationX(const double degrees) {
  constexpr double pi = 3.14159265358979323846;
  const double c = std::cos(degrees * pi / 180.0);
  const double s = std::sin(degrees * pi / 180.0);
  Mantid::Kernel::Matrix<double> rotation(3, 3);
  rotation[0][0] = 1.0;
  rotation[0][1] = 0.0;
  rotation[0][2] = 0.0;
  rotation[1][0] = 0.0;
  rotation[1][1] = c;
  rotation[1][2] = -s;
  rotation[2][0] = 0.0;
  rotation[2][1] = s;
  rotation[2][2] = c;
  return rotation;
}

/// Rotation about y, which tilts a plate lying in the xy plane out of the beam's path along z.
inline Mantid::Kernel::Matrix<double> rotationY(const double degrees) {
  constexpr double pi = 3.14159265358979323846;
  const double c = std::cos(degrees * pi / 180.0);
  const double s = std::sin(degrees * pi / 180.0);
  Mantid::Kernel::Matrix<double> rotation(3, 3);
  rotation[0][0] = c;
  rotation[0][1] = 0.0;
  rotation[0][2] = s;
  rotation[1][0] = 0.0;
  rotation[1][1] = 1.0;
  rotation[1][2] = 0.0;
  rotation[2][0] = -s;
  rotation[2][1] = 0.0;
  rotation[2][2] = c;
  return rotation;
}

/// Serialise a matrix into the attribute form the goniometer tag uses.
///
/// Full precision, deliberately: std::to_string rounds to six decimal places, which perturbs the
/// rotation enough to move an attenuation in the seventh significant figure and would make the two
/// routes disagree for reasons that have nothing to do with the code under test.
inline std::string goniometerTag(const Mantid::Kernel::Matrix<double> &matrix) {
  std::ostringstream tag;
  tag << std::setprecision(std::numeric_limits<double>::max_digits10) << "<goniometer";
  for (size_t i = 0; i < 3; ++i) {
    for (size_t j = 0; j < 3; ++j) {
      tag << " a" << (i + 1) << (j + 1) << "=\"" << matrix[i][j] << "\"";
    }
  }
  tag << "/>";
  return tag.str();
}

/// A plate much wider than it is thick, so rotating it about y changes how much material a beam
/// travelling along z has to cross. A sphere or an axis-aligned cube would not - the whole point is
/// a shape whose orientation is visible in the answer.
inline std::string plateXML(const std::string &extraTags = "") {
  return "<cuboid id=\"plate\">"
         "<left-front-bottom-point x=\"-0.02\" y=\"-0.02\" z=\"-0.002\"/>"
         "<left-front-top-point x=\"-0.02\" y=\"0.02\" z=\"-0.002\"/>"
         "<left-back-bottom-point x=\"-0.02\" y=\"-0.02\" z=\"0.002\"/>"
         "<right-front-bottom-point x=\"0.02\" y=\"-0.02\" z=\"-0.002\"/>"
         "</cuboid>" +
         extraTags;
}

inline Mantid::Kernel::Material vanadium() {
  return Mantid::Kernel::Material("Vanadium", Mantid::PhysicalConstants::getNeutronAtom(23, 0), 0.072);
}

/// Put the shape on the workspace in its own frame, with the rotation held only on the run.
///
/// shapeXML defaults to the plate, which suits an algorithm that traces the beam through the whole
/// sample. Pass something else where the algorithm constrains the shape - a cylinder for one that
/// dices in cylindrical coordinates, or a box large enough to enclose a gauge volume.
inline void setSampleInOwnFrame(Mantid::API::ExperimentInfo &expt, const Mantid::Kernel::Matrix<double> &rotation,
                                const Mantid::Kernel::Material &material = vanadium(),
                                const std::string &shapeXML = plateXML()) {
  auto shape = Mantid::Geometry::ShapeFactory().createShape(shapeXML);
  shape->setMaterial(material);
  expt.mutableSample().setShape(shape);
  expt.mutableRun().mutableGoniometer().setR(rotation);
}

/// Put the same shape on the workspace already rotated into the lab frame, with the same rotation
/// on the run - the state CopySample leaves behind.
inline void setSampleInLabFrame(Mantid::API::ExperimentInfo &expt, const Mantid::Kernel::Matrix<double> &rotation,
                                const Mantid::Kernel::Material &material = vanadium(),
                                const std::string &shapeXML = plateXML()) {
  auto shape = Mantid::Geometry::ShapeFactory().createShape(shapeXML + goniometerTag(rotation));
  shape->setMaterial(material);
  expt.mutableSample().setShape(shape);
  expt.mutableRun().mutableGoniometer().setR(rotation);
}

} // namespace SampleFrameEquivalence
