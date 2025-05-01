// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"
#include <QSize>

namespace MantidQt {
namespace MantidWidgets {

/**
\class  Viewport

Viewport sets up OpenGL orthographic projection and provides methods for
navigating around a 3D scene. With the orthographic projection it rotates,
scales and translates
the scene in plane parallel to the screen.

A Viewport is initialized with the size of a GL widget it will be used with.
When the widget is
resized the viewport must also be resized by calling resize() method.

The dimensions of the scene by calling
setProjection(...) method. This method doesn't issue any GL commands only sets
the bounds for future calls.

Call applyProjection() to issue the GL projection command and applyRotation() to
apply the transformation to the model.
*/
class Viewport {
public:
  /// Constructor with size of the viewport
  explicit Viewport(QSize size);
  /// Called to resize the viewport
  void resize(QSize size);
  /// Get the viewport width and height.
  QSize dimensions() const;
  /// Set a projection by individual values
  void setProjection(double xmin, double xmax, double ymin, double ymax, double zmin, double zmax);
  /// Set a projection by vector of min/max bounds
  void setProjection(const Mantid::Kernel::V3D &minBounds, const Mantid::Kernel::V3D &maxBounds);
  void setProjectionZPlane(const Mantid::Kernel::V3D &minBounds, const Mantid::Kernel::V3D &maxBounds);
  /// Apply the projection to OpenGL engine
  void applyProjection() const;
  /// Rotate the model
  void applyRotation() const;
  /// Clear all transforamtions (rotation, translation. scaling)
  void reset();

  /* Rotation */

  /// Call to set the View to X+ direction
  void setViewToXPositive();
  /// Call to set the View to Y+ direction
  void setViewToYPositive();
  /// Call to set the View to Z+ direction
  void setViewToZPositive();
  /// Call to set the View to X- direction
  void setViewToXNegative();
  /// Call to set the View to Y- direction
  void setViewToYNegative();
  /// Call to set the View to Z- direction
  void setViewToZNegative();

  void adjustProjection();

  /// Init rotation at a point on the screen
  void initRotationFrom(int a, int b);
  /// Generate a new rotation matrix
  void generateRotationTo(int a, int b);
  /// Set rotation programmatically
  void setRotation(const Mantid::Kernel::Quat &rot);
  /// Get current rotation
  const Mantid::Kernel::Quat &getRotation() const { return m_quaternion; }

  /* Zooming */

  /// Init zooming with a point on the screen
  void initZoomFrom(int a, int b);
  /// Generate new zooming factor
  void generateZoomTo(int a, int b);
  /// Generate zooming factor using mouse wheel
  void wheelZoom(int a, int b, int d);
  /// Set zoom programmatically
  void setZoom(double zoom);

  /* Translation */

  /// Call when the mouse button is pressed to start translation
  void initTranslateFrom(int /*a*/, int /*b*/);
  /// Call when the mouse is moving during a translation
  void generateTranslationTo(int /*a*/, int /*b*/);
  /// Set translation programmatically
  void setTranslation(double /*xval*/, double /*yval*/);

  // void getProjection(double&,double&,double&,double&,double&,double&);
  void getInstantProjection(double & /*xmin*/, double & /*xmax*/, double & /*ymin*/, double & /*ymax*/,
                            double & /*zmin*/, double & /*zmax*/) const;

  /// Apply the transformation to a vector
  void transform(Mantid::Kernel::V3D &pos) const;
  /// Load settings for the widget tab from a project file
  virtual void loadFromProject(const std::string &lines);
  /// Save settings for the widget tab to a project file
  virtual std::string saveToProject() const;

private:
  /// Correct for aspect ratio
  void correctForAspectRatioAndZoom(double &xmin, double &xmax, double &ymin, double &ymax, double &zmin,
                                    double &zmax) const;
  /// Project a point onto a sphere centered at rotation point
  Mantid::Kernel::V3D projectOnSphere(int a, int b) const;
  /// Generate a 3D point coordinates from coordinates on the viewport.
  Mantid::Kernel::V3D generateTranslationPoint(int x, int y) const;

  /* Projection */
  QSize m_dimensions;
  /// Ortho projection xmin value (Left side of the x axis)
  double m_left;
  /// Ortho projection xmax value (Right side of the x axis)
  double m_right;
  /// Ortho projection ymin value (Bottom side of the y axis)
  double m_bottom;
  /// Ortho projection ymax value (Top side of the y axis)
  double m_top;
  /// Ortho zmin value (Near side of the z axis)
  double m_near;
  /// Ortho zmax value (Far side of the z axis)
  double m_far;
  /// Minimum Z value of the bounding box
  double m_zmin;
  /// Maximum Z value of the bounding box
  double m_zmax;

  // Cached copies of values when first set
  double m_leftOrig;
  double m_rightOrig;
  double m_bottomOrig;
  double m_topOrig;
  double m_zminOrig;
  double m_zmaxOrig;

  /* Trackball rotation */

  /// Previous point selected on sphere
  Mantid::Kernel::V3D m_lastpoint;
  /// Rotation matrix stored as a quaternion
  Mantid::Kernel::Quat m_quaternion;
  /// Rotation matrix (4x4 stored as linear array) used in OpenGL
  mutable double m_rotationmatrix[16];
  /// Rotation speed of the trackball
  double m_rotationspeed;

  /* Zooming */
  double m_zoomFactor;

  /* Translation */
  /// Translation in x direction
  double m_xTrans;
  /// Translation in y direction
  double m_yTrans;
  /// Translation in z direction
  mutable double m_zTrans;

  friend class InstrumentWidgetEncoder;
  friend class InstrumentWidgetDecoder;
};
} // namespace MantidWidgets
} // namespace MantidQt
