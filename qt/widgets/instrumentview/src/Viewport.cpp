// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/InstrumentView/Viewport.h"
#include "MantidGeometry/Rendering/OpenGL_Headers.h"
#include "MantidKernel/V3D.h"

#include "MantidQtWidgets/Common/TSVSerialiser.h"
#include <QtGlobal>

#include "MantidQtWidgets/InstrumentView/OpenGLError.h"
#include <cmath>
#include <limits>

namespace MantidQt::MantidWidgets {

/**
 * Initialize with defaults.
 * @param dimensions Viewport width/height
 */
Viewport::Viewport(QSize dimensions)
    : m_dimensions(dimensions), m_left(-1), m_right(1), m_bottom(-1), m_top(1), m_near(-1), m_far(1), m_zmin(-1),
      m_zmax(1), m_leftOrig(m_left), m_rightOrig(m_right), m_bottomOrig(m_bottom), m_topOrig(m_top), m_zminOrig(m_zmin),
      m_zmaxOrig(m_zmax), m_rotationspeed(180.0 / M_PI), m_zoomFactor(1.0), m_xTrans(0.0), m_yTrans(0.0),
      m_zTrans(0.0) {
  m_quaternion.GLMatrix(&m_rotationmatrix[0]);
}

/**
 * Resize the viewport = size of the displaying widget.
 * @param dimensions Viewport width/height in device pixels
 */
void Viewport::resize(QSize dimensions) { m_dimensions = dimensions; }

/**
 * Get the size of the viewport in logical pixels (size of the displaying
 * widget).
 */
QSize Viewport::dimensions() const { return m_dimensions; }

/**
 * This will set the projection. The parameters describe the dimensions of a
 *scene
 * which has to be fully visible in this viewport by default. These don't set
 * the actual projection sizes because they have to be adjusted for the aspect
 * ratio of the displaying widget. The actual projection dimensions can be
 * retrieved by calling getInstantProjection() method.
 *
 * @param xmin :: left side of the scene
 * @param xmax :: right side of the scene
 * @param ymin :: bottom side of the scene
 * @param ymax :: top side of the scene
 * @param zmin :: near side of the scene
 * @param zmax :: far side of the scene
 */
void Viewport::setProjection(double xmin, double xmax, double ymin, double ymax, double zmin, double zmax) {
  m_left = xmin;
  m_right = xmax;
  if (m_left > m_right)
    std::swap(m_left, m_right);
  m_bottom = ymin;
  m_top = ymax;
  if (m_bottom > m_top)
    std::swap(m_bottom, m_top);
  m_near = zmin;
  m_far = zmax;
  // save the current projection bounds to reuse on view changes
  m_leftOrig = m_left;
  m_rightOrig = m_right;
  m_topOrig = m_top;
  m_bottomOrig = m_bottom;
}

/**
 * Convenience overload.
 *
 * @param minBounds :: Near-bottom-left corner of the scene.
 * @param maxBounds :: Far-top-right corner of the scene.
 */
void Viewport::setProjection(const Mantid::Kernel::V3D &minBounds, const Mantid::Kernel::V3D &maxBounds) {
  double radius = minBounds.norm();
  double tmp = maxBounds.norm();
  if (tmp > radius)
    radius = tmp;

  // save the Z value of the bounding box to use when rotating views
  m_zmin = minBounds.Z();
  m_zmax = maxBounds.Z();
  m_zminOrig = m_zmin;
  m_zmaxOrig = m_zmax;

  setProjection(minBounds.X(), maxBounds.X(), minBounds.Y(), maxBounds.Y(), -radius, radius);
}

/**
 * Sets the near and far clipping plane based on the size of given points
 *
 * @param minBounds :: Near-bottom-left corner of the scene.
 * @param maxBounds :: Far-top-right corner of the scene.
 */
void Viewport::setProjectionZPlane(const Mantid::Kernel::V3D &minBounds, const Mantid::Kernel::V3D &maxBounds) {
  double radius = minBounds.norm();
  double tmp = maxBounds.norm();
  if (tmp > radius)
    radius = tmp;

  m_near = -radius;
  m_far = radius;
}

/**
 * Return XY plane bounds corrected for the aspect ratio.
 */
void Viewport::correctForAspectRatioAndZoom(double &xmin, double &xmax, double &ymin, double &ymax, double &zmin,
                                            double &zmax) const {
  xmin = m_left;
  xmax = m_right;
  ymin = m_bottom;
  ymax = m_top;
  // check if the scene is going to be stretched anlong x or y axes
  // and correct the extent to make it loook normal
  double xSize = m_right - m_left;
  double ySize = m_top - m_bottom;
  double r = ySize * m_dimensions.width() / (xSize * m_dimensions.height());
  if (r < 1.0) {
    // ySize is too small
    ySize /= r;
    ymin = (m_bottom + m_top - ySize) / 2;
    ymax = ymin + ySize;
  } else {
    // xSize is too small
    xSize *= r;
    xmin = (m_left + m_right - xSize) / 2;
    xmax = xmin + xSize;
  }
  zmin = m_near * m_zoomFactor;
  zmax = m_far * m_zoomFactor;
}

/**
 * Get the projection bounds.
 * @param xmin :: left side of the Ortho projection
 * @param xmax :: right side of the Ortho projection
 * @param ymin :: bottom side of the Ortho projection
 * @param ymax :: top side of the Ortho projection
 * @param zmin :: near side of the Ortho Projection
 * @param zmax :: far side of the Ortho Projection
 */
void Viewport::getInstantProjection(double &xmin, double &xmax, double &ymin, double &ymax, double &zmin,
                                    double &zmax) const {
  correctForAspectRatioAndZoom(xmin, xmax, ymin, ymax, zmin, zmax);
}

void Viewport::setTranslation(double xval, double yval) {
  m_xTrans = xval;
  m_yTrans = yval;
}

/**
 * Issue the OpenGL commands that define the viewport and projection.
 */
void Viewport::applyProjection() const {
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  OpenGLError::check("GLViewport::issueGL()");

  double xmin, xmax, ymin, ymax, zmin, zmax;
  correctForAspectRatioAndZoom(xmin, xmax, ymin, ymax, zmin, zmax);

  glOrtho(xmin, xmax, ymin, ymax, zmin, zmax);
  if (OpenGLError::hasError("GLViewport::issueGL()")) {
    OpenGLError::log() << "Arguments to glOrtho:\n";
    OpenGLError::log() << xmin << ' ' << xmax << '\n' << ymin << ' ' << ymax << '\n' << zmin << ' ' << zmax << "\n\n";
  }
  // Reset the rendering options just in case
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

/**
 * A point on the screen is projected onto a sphere with center at the rotation
 * point
 * @param a :: The x screen coordinate in pixels
 * @param b :: The y screen coordinate in pixels
 * @param point :: The projection point on the sphere in model coordinates
 */
Mantid::Kernel::V3D Viewport::projectOnSphere(int a, int b) const {
  // z initiaised to zero if out of the sphere
  double z = 0;
  auto x = static_cast<double>((2.0 * a - m_dimensions.width()) / m_dimensions.width());
  auto y = static_cast<double>((m_dimensions.height() - 2.0 * b) / m_dimensions.height());
  double norm = x * x + y * y;
  if (norm > 1.0) // The point is inside the sphere
  {
    norm = sqrt(norm);
    x /= norm;
    y /= norm;
  } else // The point is outside the sphere, so project to nearest point on
         // circle
    z = sqrt(1.0 - norm);
  // Set-up point
  return Mantid::Kernel::V3D(x, y, z);
}

/**
 * Apply the transformation to the scene: translation, rotation and zooming.
 */
void Viewport::applyRotation() const {
  // Translate
  glTranslated(m_xTrans, m_yTrans, m_zTrans);
  // Rotate with respect to the centre
  glMultMatrixd(m_rotationmatrix);
  // Zoom
  glScaled(m_zoomFactor, m_zoomFactor, m_zoomFactor);

  OpenGLError::check("GLTrackball::IssueRotation()");
}

/**
 * Clear all transforamtions (rotation, translation. scaling)
 */
void Viewport::reset() {
  m_quaternion.init();
  m_quaternion.GLMatrix(&m_rotationmatrix[0]);
  m_xTrans = 0.0;
  m_yTrans = 0.0;
  m_zoomFactor = 1.0;
}

/**
 * Rotate the scene such that its X axis is perpendicular to the screen and
 * points towards the viewer.
 */
void Viewport::setViewToXPositive() {
  reset();
  Mantid::Kernel::Quat tempy(Mantid::Kernel::V3D(0.0, 0.0, 1.0), Mantid::Kernel::V3D(-1.0, 0.0, 0.0));
  m_quaternion = tempy;
  m_quaternion.GLMatrix(&m_rotationmatrix[0]);
  adjustProjection();
}

/**
 * Rotate the scene such that its Y axis is perpendicular to the screen and
 * points towards the viewer.
 */
void Viewport::setViewToYPositive() {
  reset();
  Mantid::Kernel::Quat tempy(Mantid::Kernel::V3D(0.0, 0.0, 1.0), Mantid::Kernel::V3D(0.0, -1.0, 0.0));
  m_quaternion = tempy;
  m_quaternion.GLMatrix(&m_rotationmatrix[0]);
  adjustProjection();
}

/**
 * Rotate the scene such that its Z axis is perpendicular to the screen and
 * points towards the viewer.
 */
void Viewport::setViewToZPositive() {
  reset();
  m_quaternion.init();
  m_quaternion.GLMatrix(&m_rotationmatrix[0]);
  adjustProjection();
}

/**
 * Rotate the scene such that its X axis is perpendicular to the screen and
 * points away from the viewer.
 */
void Viewport::setViewToXNegative() {
  reset();
  Mantid::Kernel::Quat tempy(Mantid::Kernel::V3D(0.0, 0.0, 1.0), Mantid::Kernel::V3D(1.0, 0.0, 0.0));
  m_quaternion = tempy;
  m_quaternion.GLMatrix(&m_rotationmatrix[0]);
  adjustProjection();
}

/**
 * Rotate the scene such that its Y axis is perpendicular to the screen and
 * points away from the viewer.
 */
void Viewport::setViewToYNegative() {
  reset();
  Mantid::Kernel::Quat tempy(Mantid::Kernel::V3D(0.0, 0.0, 1.0), Mantid::Kernel::V3D(0.0, 1.0, 0.0));
  m_quaternion = tempy;
  m_quaternion.GLMatrix(&m_rotationmatrix[0]);
  adjustProjection();
}

/**
 * Rotate the scene such that its Z axis is perpendicular to the screen and
 * points away from the viewer.
 */
void Viewport::setViewToZNegative() {
  reset();
  Mantid::Kernel::Quat tempy(180.0, Mantid::Kernel::V3D(0.0, 1.0, 0.0));
  m_quaternion = tempy;
  m_quaternion.GLMatrix(&m_rotationmatrix[0]);
  adjustProjection();
}

/**
 * Adjusts the current projection coordinates based on a any prevoius calls
 * to change the view. Note that this does not issue any drawing commands
 * it sets up the internal matrices for the next call to applyProjection
 * by the view code.
 */
void Viewport::adjustProjection() {
  // reset the projection bounds to the original values
  m_left = m_leftOrig;
  m_right = m_rightOrig;
  m_top = m_topOrig;
  m_bottom = m_bottomOrig;
  m_zmin = m_zminOrig;
  m_zmax = m_zmaxOrig;

  // rotate the original projection based on the new quaternion
  m_quaternion.rotateBB(m_left, m_bottom, m_zmin, m_right, m_top, m_zmax);
}

/**
 * Set a new rotation.
 * @param rot :: Rotattion as a quaternion.
 */
void Viewport::setRotation(const Mantid::Kernel::Quat &rot) {
  m_quaternion = rot;
  m_quaternion.GLMatrix(&m_rotationmatrix[0]);
}

/**
 * Init zooming at a point on the screen. The user starts zooming by clicking
 * (middle)
 * mouse button then drags holding the button.
 * @param a :: The x mouse coordinate
 * @param b :: The y mouse coordinate
 */
void Viewport::initZoomFrom(int a, int b) {
  if (a <= 0 || b <= 0)
    return;
  if (a >= m_dimensions.width() || b >= m_dimensions.height())
    return;
  double z = 0;
  auto x = static_cast<double>(m_dimensions.width() - a);
  auto y = static_cast<double>(b - m_dimensions.height());
  m_lastpoint(x, y, z);
}

/**
 * Calculate the zoom factor when the user releases the mouse button at a point
 * on the screen.
 * @param a :: The x mouse coordinate
 * @param b :: The y mouse coordinate
 */
void Viewport::generateZoomTo(int a, int b) {
  if (a >= m_dimensions.width() || b >= m_dimensions.height() || a <= 0 || b <= 0)
    return;
  auto y = static_cast<double>(b - m_dimensions.height());
  if (y == 0)
    y = m_lastpoint[1];
  double diff = m_lastpoint[1] / y;
  m_zoomFactor /= diff;
}

/**
 * Generate zooming factor using mouse wheel
 * @param a :: The x mouse coordinate
 * @param b :: The y mouse coordinate
 * @param d :: The mouse wheel delta
 */
void Viewport::wheelZoom(int a, int b, int d) {
  // OpenGL works with floats. Set a limit to the zoom factor
  // based on the epsilon for floats
  const double zoomLimit = std::numeric_limits<float>::epsilon() * 1000;
  Mantid::Kernel::V3D point = generateTranslationPoint(a, b);
  double diff = 1.0 + static_cast<double>(d) / 600;
  double newZoomFactor = m_zoomFactor * diff;
  if (newZoomFactor < zoomLimit || 1.0 / newZoomFactor < zoomLimit)
    return;
  // set new zoom factor
  m_zoomFactor = newZoomFactor;
  // update translation vector to keep
  Mantid::Kernel::V3D T(m_xTrans, m_yTrans, 0.0);
  T = point - (point - T) * diff;
  m_xTrans = T.X();
  m_yTrans = T.Y();
}

/**
 * Set zooming factor.
 * @param zoom :: A new zooming factor.
 */
void Viewport::setZoom(double zoom) {
  if (zoom > 0.0) {
    m_zoomFactor = zoom;
  }
}

/**
 * Start a trackball rotation from here.
 * @param a :: The x mouse coordinate
 * @param b :: The y mouse coordinate
 */
void Viewport::initRotationFrom(int a, int b) { m_lastpoint = projectOnSphere(a, b); }

/**
 * Generate the rotation matrix to rotate to this point.
 * @param a :: The x mouse coordinate
 * @param b :: The y mouse coordinate
 */
void Viewport::generateRotationTo(int a, int b) {
  Mantid::Kernel::V3D newpoint = projectOnSphere(a, b);
  // Angle is given in degrees as the dot product of the two vectors
  double angle = m_rotationspeed * newpoint.angle(m_lastpoint);
  const auto diff = m_lastpoint.cross_prod(newpoint);
  // Create a quaternion from the angle and vector direction
  Mantid::Kernel::Quat temp(angle, diff);
  // Left multiply
  temp *= m_quaternion;
  // Assignment of _quaternion
  m_quaternion(temp);
  // Get the corresponding OpenGL rotation matrix
  m_quaternion.GLMatrix(&m_rotationmatrix[0]);
}

/**
 * Initialize scene translation at a point on the screen
 * @param a :: The x mouse coordinate
 * @param b :: The y mouse coordinate
 */
void Viewport::initTranslateFrom(int a, int b) { m_lastpoint = generateTranslationPoint(a, b); }

/**
 * Generate scene translation such that a point of the last initTranslateFrom
 * moved to the new position pointed by the mouse.
 * @param a :: The x mouse coordinate
 * @param b :: The y mouse coordinate
 */
void Viewport::generateTranslationTo(int a, int b) {
  Mantid::Kernel::V3D newpoint = generateTranslationPoint(a, b);
  // This is now the difference
  newpoint -= m_lastpoint;
  m_xTrans += newpoint[0];
  m_yTrans += newpoint[1];
}

/**
 * Find coordinates of a point on z = 0 plane under the mouse.
 * @param a :: The x mouse coordinate
 * @param b :: The y mouse coordinate
 * @param point :: Return the result through this reference.
 */
Mantid::Kernel::V3D Viewport::generateTranslationPoint(int a, int b) const {
  double x, y, z = 0.0;
  double xmin, xmax, ymin, ymax, zmin, zmax;
  correctForAspectRatioAndZoom(xmin, xmax, ymin, ymax, zmin, zmax);
  x = static_cast<double>((xmin + ((xmax - xmin) * ((double)a / (double)m_dimensions.width()))));
  y = static_cast<double>((ymin + ((ymax - ymin) * (m_dimensions.height() - b) / m_dimensions.height())));
  return Mantid::Kernel::V3D(x, y, z);
}

/**
 * Apply the transformation to a vector.
 * @param pos :: A position vector to transform (in and out).
 */
void Viewport::transform(Mantid::Kernel::V3D &pos) const {
  pos *= m_zoomFactor;
  m_quaternion.rotate(pos);
  pos += Mantid::Kernel::V3D(m_xTrans, m_yTrans, 0.0);
}

void Viewport::loadFromProject(const std::string &lines) {
  Q_UNUSED(lines);
  throw std::runtime_error("Viewport::loadFromProject not implemented for Qt >= 5");
}

std::string Viewport::saveToProject() const {
  // WARNING: Q1DWeighted heavily depends on the format of this function's
  // output (via "Save Shapes to table" in the instrument viewer draw tab).
  // Modify with great caution.
  API::TSVSerialiser tsv;
  tsv.writeLine("Translation") << m_xTrans << m_yTrans;
  tsv.writeLine("Zoom") << m_zoomFactor;

  tsv.writeLine("Rotation");
  for (int i = 0; i < 4; ++i) {
    tsv << m_quaternion[i];
  }

  return tsv.outputLines();
}
} // namespace MantidQt::MantidWidgets
