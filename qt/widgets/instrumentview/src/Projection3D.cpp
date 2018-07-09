#ifdef _WIN32
#include <windows.h>
#endif
#include "MantidQtWidgets/InstrumentView/GLColor.h"
#include "MantidQtWidgets/InstrumentView/OpenGLError.h"
#include "MantidQtWidgets/InstrumentView/Projection3D.h"
#include "MantidQtWidgets/InstrumentView/UnwrappedCylinder.h"
#include "MantidQtWidgets/InstrumentView/UnwrappedSphere.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidQtWidgets/Common/InputController.h"
#include "MantidQtWidgets/Common/TSVSerialiser.h"

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <QApplication>
#include <QSpinBox>
#include <QTime>
#include <QtOpenGL>

#include <algorithm>
#include <cfloat>
#include <map>
#include <string>

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

namespace MantidQt {
namespace MantidWidgets {
Projection3D::Projection3D(const InstrumentActor *rootActor, int winWidth,
                           int winHeight)
    : ProjectionSurface(rootActor), m_drawAxes(true), m_wireframe(false),
      m_viewport(0, 0) {
  m_viewport.resize(winWidth, winHeight);
  V3D minBounds, maxBounds;
  m_instrActor->getBoundingBox(minBounds, maxBounds);

  m_viewport.setProjection(minBounds, maxBounds);

  changeColorMap();

  // create and connect the move input controller
  InputController3DMove *moveController = new InputController3DMove(this);
  setInputController(MoveMode, moveController);
  connect(moveController, SIGNAL(initTranslation(int, int)), this,
          SLOT(initTranslation(int, int)));
  connect(moveController, SIGNAL(translate(int, int)), this,
          SLOT(translate(int, int)));
  connect(moveController, SIGNAL(initRotation(int, int)), this,
          SLOT(initRotation(int, int)));
  connect(moveController, SIGNAL(rotate(int, int)), this,
          SLOT(rotate(int, int)));
  connect(moveController, SIGNAL(initZoom(int, int)), this,
          SLOT(initZoom(int, int)));
  connect(moveController, SIGNAL(zoom(int, int)), this, SLOT(zoom(int, int)));
  connect(moveController, SIGNAL(wheelZoom(int, int, int)), this,
          SLOT(wheelZoom(int, int, int)));
  connect(moveController, SIGNAL(finish()), this, SLOT(finishMove()));
}

Projection3D::~Projection3D() {}

/**
 * Resize the surface on the screen.
 * @param w :: New width of the surface in pixels.
 * @param h :: New height of the surface in pixels.
 */
void Projection3D::resize(int w, int h) {
  m_viewport.resize(w, h);
  updateView();
}

/**
 * Draw the instrument on MantidGLWidget.
 */
void Projection3D::drawSurface(MantidGLWidget *, bool picking) const {
  OpenGLError::check("GL3DWidget::draw3D()[begin]");

  glEnable(GL_DEPTH_TEST);
  OpenGLError::check("GL3DWidget::draw3D()[depth] ");

  if (m_wireframe) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  } else {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }

  setLightingModel(picking);

  // fill the buffer with background colour
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  m_viewport.applyProjection();

  // Issue the rotation, translation and zooming of the trackball to the object
  m_viewport.applyRotation();

  // if actor is undefined leave it with clear screen
  if (m_instrActor) {
    QApplication::setOverrideCursor(Qt::WaitCursor);
    m_instrActor->draw(picking);
    OpenGLError::check("GL3DWidget::draw3D()[scene draw] ");
    QApplication::restoreOverrideCursor();
  }

  // Also some axes
  if (m_drawAxes && !picking) {
    // This draws a point at the origin, I guess
    glPointSize(3.0);
    glBegin(GL_POINTS);
    glVertex3d(0.0, 0.0, 0.0);
    glEnd();

    drawAxes();
  }

  OpenGLError::check("GL3DWidget::draw3D()");
}

/** Draw 3D axes centered at the origin (if the option is selected)
 *
 */
void Projection3D::drawAxes(double axis_length) const {
  glPointSize(3.0);
  glLineWidth(3.0);

  // To make sure the lines are colored
  glEnable(GL_COLOR_MATERIAL);
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_LIGHTING);
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

  glColor3f(1.0, 0., 0.);
  glBegin(GL_LINES);
  glVertex3d(0.0, 0.0, 0.0);
  glVertex3d(axis_length, 0.0, 0.0);
  glEnd();

  glColor3f(0., 1.0, 0.);
  glBegin(GL_LINES);
  glVertex3d(0.0, 0.0, 0.0);
  glVertex3d(0.0, axis_length, 0.0);
  glEnd();

  glColor3f(0., 0., 1.);
  glBegin(GL_LINES);
  glVertex3d(0.0, 0.0, 0.0);
  glVertex3d(0.0, 0.0, axis_length);
  glEnd();
}

void Projection3D::changeColorMap() {}

/**
 * Select 1 of the 6 axis-aligned orientations.
 */
void Projection3D::setViewDirection(const QString &input) {
  if (input.toUpper().compare("X+") == 0) {
    m_viewport.setViewToXPositive();
  } else if (input.toUpper().compare("X-") == 0) {
    m_viewport.setViewToXNegative();
  } else if (input.toUpper().compare("Y+") == 0) {
    m_viewport.setViewToYPositive();
  } else if (input.toUpper().compare("Y-") == 0) {
    m_viewport.setViewToYNegative();
  } else if (input.toUpper().compare("Z+") == 0) {
    m_viewport.setViewToZPositive();
  } else if (input.toUpper().compare("Z-") == 0) {
    m_viewport.setViewToZNegative();
  }
  updateView(false);
}

/**
 * Toggle the 3D axes.
 */
void Projection3D::set3DAxesState(bool on) { m_drawAxes = on; }

/**
 * Toggle wireframe view.
 */
void Projection3D::setWireframe(bool on) { m_wireframe = on; }

//-----------------------------------------------------------------------------
/** This seems to be called when the user has selected a rectangle
 * using the mouse.
 *
 * @param detIndices :: returns a list of detector Indices selected.
 */
void Projection3D::getSelectedDetectors(std::vector<size_t> &detIndices) {
  detIndices.clear();
  if (!hasSelection())
    return;
  double xmin, xmax, ymin, ymax, zmin, zmax;
  m_viewport.getInstantProjection(xmin, xmax, ymin, ymax, zmin, zmax);
  QRect rect = selectionRect();
  int w, h;
  m_viewport.getViewport(w, h);

  double xLeft = xmin + (xmax - xmin) * rect.left() / w;
  double xRight = xmin + (xmax - xmin) * rect.right() / w;
  double yBottom = ymin + (ymax - ymin) * (h - rect.bottom()) / h;
  double yTop = ymin + (ymax - ymin) * (h - rect.top()) / h;
  size_t ndet = m_instrActor->ndetectors();

  for (size_t i = 0; i < ndet; ++i) {
    V3D pos = m_instrActor->getDetPos(i);
    m_viewport.transform(pos);
    if (pos.X() >= xLeft && pos.X() <= xRight && pos.Y() >= yBottom &&
        pos.Y() <= yTop) {
      detIndices.push_back(i);
    }
  }
}

//-----------------------------------------------------------------------------
/** Select detectors to mask, using the mouse.
 * From the Instrument Window's mask tab.
 *
 * @param dets :: returns a list of detector Indices to mask.
 */
void Projection3D::getMaskedDetectors(std::vector<size_t> &detIndices) const {
  // find the layer of visible detectors
  QList<QPoint> pixels = m_maskShapes.getMaskedPixels();
  double zmin = 1.0;
  double zmax = 0.0;
  QSet<int> ids;
  foreach (const QPoint &p, pixels) {
    int id = getDetectorID(p.x(), p.y());
    if (ids.contains(id))
      continue;
    ids.insert(id);
    V3D pos = this->getDetectorPos(p.x(), p.y());
    m_viewport.transform(pos);
    double z = pos.Z();
    if (zmin > zmax) {
      zmin = zmax = z;
    } else {
      if (zmin > z)
        zmin = z;
      if (zmax < z)
        zmax = z;
    }
  }

  // find masked detector in that layer
  detIndices.clear();
  if (m_maskShapes.isEmpty())
    return;
  size_t ndet = m_instrActor->ndetectors();
  for (size_t i = 0; i < ndet; ++i) {
    // Find the cached ID and position. This is much faster than getting the
    // detector.
    V3D pos = m_instrActor->getDetPos(i);
    // project pos onto the screen plane
    m_viewport.transform(pos);
    if (pos.Z() < zmin || pos.Z() > zmax)
      continue;
    if (m_maskShapes.isMasked(pos.X(), pos.Y())) {
      detIndices.push_back(i);
    }
  }
}

/**
 * Orient the viewport to look at a selected component.
 * @param id :: The ID of a selected component.
 */
void Projection3D::componentSelected(size_t componentIndex) {

  const auto &componentInfo = m_instrActor->componentInfo();

  if (componentIndex == componentInfo.root()) {
    m_viewport.reset();
    return;
  }

  auto pos = componentInfo.position(componentIndex);

  auto compDir = pos - componentInfo.samplePosition();
  compDir.normalize();
  V3D up(0, 0, 1);
  V3D x = up.cross_prod(compDir);
  up = compDir.cross_prod(x);
  Quat rot;
  InstrumentActor::BasisRotation(x, up, compDir, V3D(-1, 0, 0), V3D(0, 1, 0),
                                 V3D(0, 0, -1), rot);

  rot.rotate(pos);
  m_viewport.setTranslation(-pos.X(), -pos.Y());
  m_viewport.setRotation(rot);
}

/**
 * Return information text to be displayed in the InstrumentWindow's info area.
 */
QString Projection3D::getInfoText() const {
  if (m_interactionMode == MoveMode) {
    QString text =
        "Mouse Buttons: Left -- Rotation, Middle -- Zoom, Right -- Translate.";
    if (m_drawAxes) {
      text += " Axes: X = Red; Y = Green; Z = Blue";
    }
    return text;
  }
  return ProjectionSurface::getInfoText();
}

/**
 * Initialize translation movement at point on the screen.
 * @param x :: The x screen coord clicked with the mouse to start translation.
 * @param y :: The y screen coord clicked with the mouse to start translation.
 */
void Projection3D::initTranslation(int x, int y) {
  m_viewport.initTranslateFrom(x, y);
}

/**
 * Translate the view in the surface.
 * @param x :: The x screen coord of the mouse pointer.
 * @param y :: The y screen coord of the mouse pointer.
 */
void Projection3D::translate(int x, int y) {
  m_viewport.generateTranslationTo(x, y);
  m_viewport.initTranslateFrom(x, y);
  updateView(false);
}

/**
 * Initialize zooming at point on the screen.
 * @param x :: The x screen coord of the mouse pointer.
 * @param y :: The y screen coord of the mouse pointer.
 */
void Projection3D::initZoom(int x, int y) { m_viewport.initZoomFrom(x, y); }

/**
 * Zoom the view in the surface.
 * @param x :: The x screen coord of the mouse pointer.
 * @param y :: The y screen coord of the mouse pointer.
 */
void Projection3D::zoom(int x, int y) {
  m_viewport.generateZoomTo(x, y);
  m_viewport.initZoomFrom(x, y);
  updateView(false);
  emit finishedMove();
}

/**
 * Zoom the view in the surface using the mouse wheel.
 * @param x :: The x screen coord of the mouse pointer.
 * @param y :: The y screen coord of the mouse pointer.
 * @param d :: zoom factor to shift y screen coord.
 */
void Projection3D::wheelZoom(int x, int y, int d) {
  m_viewport.wheelZoom(x, y, d);
  updateView(false);
  emit finishedMove();
}

/**
 * Initialize rotation movement at point on the screen.
 * @param x :: The x screen coord of the mouse pointer.
 * @param y :: The y screen coord of the mouse pointer.
 */
void Projection3D::initRotation(int x, int y) {
  m_viewport.initRotationFrom(x, y);
}

/**
 * Rotate the view in the surface.
 * @param x :: The x screen coord of the mouse pointer.
 * @param y :: The y screen coord of the mouse pointer.
 */
void Projection3D::rotate(int x, int y) {
  m_viewport.generateRotationTo(x, y);
  m_viewport.initRotationFrom(x, y);
  updateView(false);
}

/**
 * Call upon finishing all moves to update the pick image.
 */
void Projection3D::finishMove() {
  updateView(true);
  emit finishedMove();
}

/**
 * Get bounds of this projection surface. Used with 2D overlays.
 */
RectF Projection3D::getSurfaceBounds() const {
  double xmin, xmax, ymin, ymax, zmin, zmax;
  m_viewport.getInstantProjection(xmin, xmax, ymin, ymax, zmin, zmax);
  return RectF(QPointF(xmin, ymin), QPointF(xmax, ymax));
}

/**
 * Define lighting of the scene
 */
void Projection3D::setLightingModel(bool picking) const {
  // Basic lighting
  if (m_isLightingOn && !picking) {
    glShadeModel(
        GL_SMOOTH); // Shade model is smooth (expensive but looks pleasing)
    glEnable(GL_LINE_SMOOTH); // Set line should be drawn smoothly
    glEnable(GL_NORMALIZE);   // Slow normal normalization
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,
                  GL_TRUE); // This model lits both sides of the triangle

    const float lamp0_intensity = 0.3f;
    const float lamp1_intensity = 0.7f;

    // First light source - spot light at the origin
    glEnable(GL_LIGHT0); // Enable opengl first light
    float lamp0_diffuse[4] = {lamp0_intensity, lamp0_intensity, lamp0_intensity,
                              1.0f};
    float lamp0_ambient[4] = {0.1f, 0.1f, 0.1f, 1.0f};
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lamp0_diffuse);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lamp0_ambient);
    float lamp0_pos[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, lamp0_pos);

    // Second light source
    // Its a directional light which follows camera position
    glEnable(GL_LIGHT1); // Enable opengl second light
    float lamp1_diffuse[4] = {lamp1_intensity, lamp1_intensity, lamp1_intensity,
                              1.0f};
    glLightfv(GL_LIGHT1, GL_DIFFUSE, lamp1_diffuse);

    glEnable(GL_LIGHTING); // Enable overall lighting
  } else {
    glShadeModel(GL_FLAT);
    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    glDisable(GL_LIGHT1);
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_NORMALIZE);
  }
}

/** Load 3D projection state from a Mantid project file
 * @param lines :: lines from the project file to load state from
 */
void Projection3D::loadFromProject(const std::string &lines) {
  ProjectionSurface::loadFromProject(lines);
  API::TSVSerialiser tsv(lines);

  if (tsv.selectSection("Viewport")) {
    std::string viewportLines;
    tsv >> viewportLines;
    m_viewport.loadFromProject(viewportLines);
  }
}

/** Save the state of the 3D projection to a Mantid project file
 * @return a string representing the state of the 3D projection
 */
std::string Projection3D::saveToProject() const {
  API::TSVSerialiser tsv;
  tsv.writeRaw(ProjectionSurface::saveToProject());
  tsv.writeSection("Viewport", m_viewport.saveToProject());
  return tsv.outputLines();
}

} // namespace MantidWidgets
} // namespace MantidQt
