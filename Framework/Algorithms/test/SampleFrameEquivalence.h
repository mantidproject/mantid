// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

/** Helpers for asserting that the two ways of orienting a sample agree.
 *
 * A workspace looks the same from the run whether its shape is in its own frame (SetGoniometer
 * alone) or already baked into the lab frame (CopySample). They describe the same experiment, so
 * any algorithm consuming the shape must answer the same for both: one that ignores the goniometer
 * uses the unrotated shape, one that applies it unconditionally turns the baked shape twice.
 *
 * The lab-frame shape is baked with a bare <goniometer> tag, read as a full bake by ShapeFactory's
 * legacy fallback - deliberately independent of Geometry::getLabFrameShape, so these tests cannot
 * pass by agreeing with the code they check.
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
#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"

#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

namespace SampleFrameEquivalence {

/// Rotation about x, which tilts a shape standing along y - a cylinder, say - towards the beam.
inline Mantid::Kernel::Matrix<double> rotationX(const double degrees) {
  return Mantid::Kernel::Matrix<double>(Mantid::Kernel::Quat(degrees, Mantid::Kernel::V3D(1, 0, 0)).getRotation());
}

/// Rotation about y, which tilts a plate lying in the xy plane out of the beam's path along z.
inline Mantid::Kernel::Matrix<double> rotationY(const double degrees) {
  return Mantid::Kernel::Matrix<double>(Mantid::Kernel::Quat(degrees, Mantid::Kernel::V3D(0, 1, 0)).getRotation());
}

/// No rotation - the control these tests compare against, to show the rotation mattered.
inline Mantid::Kernel::Matrix<double> unrotated() { return Mantid::Kernel::Matrix<double>(3, 3, true); }

/// Serialise a matrix into the attribute form the goniometer tag uses. Full precision, deliberately:
/// ShapeFactory's own six decimal places move an attenuation in the seventh significant figure, so
/// the two routes would disagree for reasons unrelated to the code under test.
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
/// along z crosses - an orientation a sphere or axis-aligned cube would hide.
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

/// Put the shape on the workspace in one frame or the other, with the same rotation on the run
/// either way. Pass a shapeXML other than the default plate where the algorithm constrains the
/// shape - a cylinder for one that dices in cylindrical coordinates, or a box enclosing a gauge.
inline void setSample(Mantid::API::ExperimentInfo &expt, const Mantid::Kernel::Matrix<double> &rotation,
                      const bool baked, const Mantid::Kernel::Material &material = vanadium(),
                      const std::string &shapeXML = plateXML()) {
  auto shape = Mantid::Geometry::ShapeFactory().createShape(baked ? shapeXML + goniometerTag(rotation) : shapeXML);
  shape->setMaterial(material);
  expt.mutableSample().setShape(shape);
  expt.mutableRun().mutableGoniometer().setR(rotation);
}

} // namespace SampleFrameEquivalence
