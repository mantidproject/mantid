// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/InstrumentView/UnwrappedSurface.h"
#include "MantidQtWidgets/InstrumentView/GLColor.h"
#include "MantidQtWidgets/InstrumentView/GLDisplay.h"
#include "MantidQtWidgets/InstrumentView/InstrumentRenderer.h"
#include "MantidQtWidgets/InstrumentView/OpenGLError.h"
#include "MantidQtWidgets/InstrumentView/PeakMarker2D.h"

#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidQtWidgets/Common/InputController.h"

#include <QApplication>
#include <QDebug>
#include <QMenu>
#include <QMouseEvent>
#include <QPoint>
#include <QSize>
#include <QTransform>

#include <cfloat>
#include <cmath>
#include <limits>

using namespace Mantid::Geometry;

namespace MantidQt::MantidWidgets {
namespace {

QRectF getArea(const UnwrappedDetector &udet, double maxWidth, double maxHeight) {
  auto w = udet.width;
  if (w > maxWidth)
    w = maxWidth;
  auto h = udet.height;
  if (h > maxHeight)
    h = maxHeight;
  return QRectF(udet.u - w, udet.v - h, w * 2, h * 2);
}
} // namespace
/**
 * Constructor.
 * @param rootActor :: The instrument actor.
 */
UnwrappedSurface::UnwrappedSurface(const IInstrumentActor *rootActor, const QSize &widgetSize,
                                   const bool maintainAspectRatio)
    : ProjectionSurface(rootActor), m_u_min(DBL_MAX), m_u_max(-DBL_MAX), m_v_min(DBL_MAX), m_v_max(-DBL_MAX),
      m_height_max(0), m_width_max(0), m_flippedView(false), m_startPeakShapes(false), m_widgetSize(widgetSize),
      m_maintainAspectRatio(maintainAspectRatio) {
  // create and set the move input controller
  InputControllerMoveUnwrapped *moveController = new InputControllerMoveUnwrapped(this);
  setInputController(MoveMode, moveController);
  connect(moveController, SIGNAL(setSelectionRect(QRect)), this, SLOT(setSelectionRect(QRect)));
  connect(moveController, SIGNAL(zoom()), this, SLOT(zoom()));
  connect(moveController, SIGNAL(resetZoom()), this, SLOT(resetZoom()));
  connect(moveController, SIGNAL(unzoom()), this, SLOT(unzoom()));
}

/**
 * Get information about the dimensions of the surface.
 */
QString UnwrappedSurface::getDimInfo() const {
  return QString("U: [%1, %2] V: [%3, %4]")
      .arg(m_viewRect.x0())
      .arg(m_viewRect.x1())
      .arg(m_viewRect.y0())
      .arg(m_viewRect.y1());
}

RectF UnwrappedSurface::selectionRectUV() const {
  auto left = static_cast<double>(m_selectRect.left());
  auto right = static_cast<double>(m_selectRect.right());
  auto top = static_cast<double>(m_selectRect.top());
  auto bottom = static_cast<double>(m_selectRect.bottom());

  if (left > right) {
    std::swap(left, right);
  }

  if (top > bottom) {
    std::swap(top, bottom);
  }

  if (abs(m_selectRect.width()) <= 1 || abs(m_selectRect.height()) <= 1)
    return RectF();

  auto expandedViewRect = correctForAspectRatioAndZoom(m_viewImage->width(), m_viewImage->height());

  const QSizeF viewSizeLogical(m_viewImage->width() / m_viewImage->devicePixelRatio(),
                               m_viewImage->height() / m_viewImage->devicePixelRatio());

  double sx = expandedViewRect.xSpan() / viewSizeLogical.width();
  double sy = expandedViewRect.ySpan() / viewSizeLogical.height();

  double x_min = left * sx + expandedViewRect.x0();
  double x_max = right * sx + expandedViewRect.x0();
  // Qt has y increasing in downward direction so map onto m_viewRect which has y increasing in upward direction
  double y_min = (viewSizeLogical.height() - bottom) * sy + expandedViewRect.y0();
  double y_max = (viewSizeLogical.height() - top) * sy + expandedViewRect.y0();

  return RectF(QPointF(x_min, y_min), QPointF(x_max, y_max));
}

RectF UnwrappedSurface::correctForAspectRatioAndZoom(const int widget_width, const int widget_height) const {
  // check if the scene is going to be stretched along x or y axes
  // and correct the extent to make it look normal (from Viewport::correctForAspectRatioAndZoom)
  double view_left = m_viewRect.x0();
  double view_top = m_viewRect.y1();
  double view_right = m_viewRect.x1();
  double view_bottom = m_viewRect.y0();

  // make sure the view rectangle has a finite area
  if (view_left == view_right) {
    view_left -= m_width_max / 2;
    view_right += m_width_max / 2;
  }
  if (view_top == view_bottom) {
    view_top += m_height_max / 2;
    view_bottom -= m_height_max / 2;
  }

  if (m_maintainAspectRatio) {
    double xSize = std::abs(view_right - view_left);
    double ySize = std::abs(view_top - view_bottom);
    double r = ySize * widget_width / (xSize * widget_height);
    if (r < 1.0) {
      // ySize is too small
      ySize /= r;
      view_bottom = (view_bottom + view_top - ySize) / 2;
      view_top = view_bottom + ySize;
    } else {
      // xSize is too small
      xSize *= r;
      view_left = (view_left + view_right - xSize) / 2;
      view_right = view_left + xSize;
    }
  }
  return RectF(QPointF(view_left, view_bottom), QPointF(view_right, view_top));
}

//------------------------------------------------------------------------------
/**
 * Draw the unwrapped instrument onto the screen
 * @param widget :: The widget to draw it on.
 * @param picking :: True if detector is being drawn in the picking mode.
 */
void UnwrappedSurface::drawSurface(GLDisplay *widget, bool picking) const {
  // dimensions of the screen to draw on
  int widget_width = widget->width();
  int widget_height = widget->height();

  // view rectangle
  double view_left = m_viewRect.x0();
  double view_top = m_viewRect.y1();
  double view_right = m_viewRect.x1();
  double view_bottom = m_viewRect.y0();

  // make sure the view rectangle has a finite area
  if (view_left == view_right) {
    view_left -= m_width_max / 2;
    view_right += m_width_max / 2;
  }
  if (view_top == view_bottom) {
    view_top += m_height_max / 2;
    view_bottom -= m_height_max / 2;
  }

  auto expandedViewRect = correctForAspectRatioAndZoom(widget_width, widget_height);

  const double dw = fabs((view_right - view_left) / widget_width);
  const double dh = fabs((view_top - view_bottom) / widget_height);

  if (m_startPeakShapes) {
    createPeakShapes();
  }

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(expandedViewRect.x0(), expandedViewRect.x1(), expandedViewRect.y0(), expandedViewRect.y1(), -10, 10);

  if (OpenGLError::hasError("UnwrappedSurface::drawSurface")) {
    OpenGLError::log() << "glOrtho arguments:\n";
    OpenGLError::log() << view_left << ',' << view_right << ',' << view_bottom << ',' << view_top << ',' << -10 << ','
                       << 10 << '\n';
  }
  glMatrixMode(GL_MODELVIEW);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  GLfloat oldLineWidth;
  glGetFloatv(GL_LINE_WIDTH, &oldLineWidth);
  glLineWidth(1.0f);

  glLoadIdentity();

  if (m_isLightingOn && !picking) {
    glShadeModel(GL_SMOOTH);  // Shade model is smooth
    glEnable(GL_LINE_SMOOTH); // Set line should be drawn smoothly
    glEnable(GL_LIGHT0);      // Enable opengl second light
    float diffuse[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    float direction[3] = {0.0f, 0.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, direction);

    glEnable(GL_LIGHTING); // Enable overall lighting
  } else {
    glDisable(GL_LIGHT0);
    glDisable(GL_LIGHTING);
    glDisable(GL_LINE_SMOOTH);
    glShadeModel(GL_FLAT);
  }

  const auto &componentInfo = m_instrActor->componentInfo();
  for (const auto &udet : m_unwrappedDetectors) {
    if (udet.empty() || !componentInfo.hasValidShape(udet.detIndex))
      continue;

    int iw = int(udet.width / dw);
    int ih = int(udet.height / dh);
    double w = (iw == 0) ? dw : udet.width / 2;
    double h = (ih == 0) ? dh : udet.height / 2;

    // check that the detector is visible in the current view
    if (!(m_viewRect.contains(udet.u - w, udet.v - h) || m_viewRect.contains(udet.u + w, udet.v + h)))
      continue;

    // apply the detector's colour
    setColor(udet.detIndex, picking);

    // if the detector is too small to see its shape draw a rectangle
    if (iw < 6 || ih < 6) {
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      glRectd(udet.u - w, udet.v - h, udet.u + w, udet.v + h);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      if (iw > 2 || ih > 2) {
        glRectd(udet.u - w, udet.v - h, udet.u + w, udet.v + h);
      }
    }
    // else draw the correct shape
    else {
      glPushMatrix();

      glTranslated(udet.u, udet.v, 0.);
      glScaled(udet.uscale, udet.vscale, 1);

      Mantid::Kernel::Quat rot;
      this->rotate(udet, rot);
      double deg, ax0, ax1, ax2;
      rot.getAngleAxis(deg, ax0, ax1, ax2);
      glRotated(deg, ax0, ax1, ax2);

      Mantid::Kernel::V3D scaleFactor = componentInfo.scaleFactor(udet.detIndex);
      glScaled(scaleFactor[0], scaleFactor[1], scaleFactor[2]);

      m_instrActor->componentInfo().shape(udet.detIndex).draw();

      glPopMatrix();
    }
  }

  OpenGLError::check("UnwrappedSurface::drawSurface");

  glLineWidth(oldLineWidth);

  if (OpenGLError::check("UnwrappedSurface::drawSurface")) {
    OpenGLError::log() << "oldLineWidth=" << oldLineWidth << '\n';
  }
}

/**
 * Set detector color in OpenGL context.
 * @param index :: Detector's index in m_unwrappedDetectors
 * @param picking :: True if detector is being drawn in the picking mode.
 *   In this case index is transformed into color
 */
void UnwrappedSurface::setColor(size_t index, bool picking) const {
  if (picking) {
    auto c = InstrumentRenderer::makePickColor(index);
    unsigned char r, g, b;
    c.get(r, g, b);
    glColor3ub(r, g, b);
  } else {
    unsigned char col[3];
    m_unwrappedDetectors[index].color.getUB3(&col[0]);
    glColor3ub(col[0], col[1], col[2]);
  }
}

bool hasParent(const std::shared_ptr<const Mantid::Geometry::IComponent> &comp, Mantid::Geometry::ComponentID id) {
  std::shared_ptr<const Mantid::Geometry::IComponent> parent = comp->getParent();
  if (!parent)
    return false;
  if (parent->getComponentID() == id)
    return true;
  return hasParent(parent, id);
}

//------------------------------------------------------------------------------
/** This method is called when a component is selected in the
 *InstrumentTreeWidget
 * and zooms into that spot on the view.
 *
 * @param id :: ComponentID to zoom to.
 */
void UnwrappedSurface::componentSelected(size_t componentIndex) {
  const auto &componentInfo = m_instrActor->componentInfo();
  if (componentInfo.isDetector(componentIndex)) {
    const auto &udet = m_unwrappedDetectors[componentIndex];
    zoom(getArea(udet, m_width_max, m_height_max));
  } else {
    auto detectors = componentInfo.detectorsInSubtree(componentIndex);
    QRectF area;
    for (auto det : detectors) {
      QRectF detRect;
      const auto &udet = m_unwrappedDetectors[det];
      detRect.setLeft(udet.u - udet.width);
      detRect.setRight(udet.u + udet.width);
      detRect.setBottom(udet.v - udet.height);
      detRect.setTop(udet.v + udet.height);
      area |= detRect;
    }
    zoom(area);
  }
}

void UnwrappedSurface::getSelectedDetectors(std::vector<size_t> &detIndices) {
  if (m_selectRect.isNull()) {
    return;
  }
  QRect rect = selectionRect();

  double vtop = m_v_min;
  double vbottom = m_v_min;
  double uleft = m_u_min;
  double uright = m_u_min;

  // find the first picking colours different from black (0,0,0) to get the
  // top-left
  // and bottom-right detectors
  int rwidth = rect.width();
  int rheight = rect.height();
  for (int i = 0; i < rwidth; ++i) {
    bool stop = false;
    for (int j = 0; j < rheight; ++j) {
      int x = rect.x() + i;
      int y = rect.y() + j;
      size_t ind = getPickID(x, y);
      if (ind < m_unwrappedDetectors.size()) {
        uleft = m_unwrappedDetectors[ind].u - m_unwrappedDetectors[ind].width / 2;
        vtop = m_unwrappedDetectors[ind].v + m_unwrappedDetectors[ind].height / 2;
        stop = true;
        break;
      }
    }
    if (stop)
      break;
  }

  for (int i = rwidth - 1; i >= 0; --i) {
    bool stop = false;
    for (int j = rheight - 1; j >= 0; --j) {
      int x = rect.x() + i;
      int y = rect.y() + j;
      size_t ind = getPickID(x, y);
      if (ind < m_unwrappedDetectors.size()) {
        uright = m_unwrappedDetectors[ind].u + m_unwrappedDetectors[ind].width / 2;
        vbottom = m_unwrappedDetectors[ind].v - m_unwrappedDetectors[ind].height / 2;
        stop = true;
        break;
      }
    }
    if (stop)
      break;
  }

  // select detectors with u,v within the allowed boundaries
  for (auto &udet : m_unwrappedDetectors) {
    if (udet.u >= uleft && udet.u <= uright && udet.v >= vbottom && udet.v <= vtop) {
      detIndices.emplace_back(udet.detIndex);
    }
  }
}

void UnwrappedSurface::getMaskedDetectors(std::vector<size_t> &detIndices) const {
  detIndices.clear();
  if (m_maskShapes.isEmpty())
    return;
  for (const auto &udet : m_unwrappedDetectors) {
    if (!udet.empty() && m_maskShapes.isMasked(udet.u, udet.v)) {
      detIndices.emplace_back(udet.detIndex);
    }
  }
}

void UnwrappedSurface::getIntersectingDetectors(std::vector<size_t> &detIndices) const {
  detIndices.clear();
  if (m_maskShapes.isEmpty())
    return;
  for (const auto &udet : m_unwrappedDetectors) {
    if (!udet.empty() && m_maskShapes.isIntersecting(udet.toQRectF())) {
      detIndices.emplace_back(udet.detIndex);
    }
  }
}

void UnwrappedSurface::changeColorMap() {
  for (auto &udet : m_unwrappedDetectors) {
    udet.color = m_instrActor->getColor(udet.detIndex);
  }
}

QString UnwrappedSurface::getInfoText() const {
  if (m_interactionMode == MoveMode) {
    // return getDimInfo() +
    return "Left mouse click and drag to zoom in. Right mouse click to zoom "
           "out.";
  }
  return ProjectionSurface::getInfoText();
}

RectF UnwrappedSurface::getSurfaceBounds() const {
  auto expandedViewRect = correctForAspectRatioAndZoom(m_widgetSize.width(), m_widgetSize.height());
  return expandedViewRect;
}

/**
 * Set a peaks workspace to be drawn ontop of the workspace.
 * @param pws :: A shared pointer to the workspace.
 */
void UnwrappedSurface::setPeaksWorkspace(const std::shared_ptr<Mantid::API::IPeaksWorkspace> &pws) {
  if (!pws) {
    return;
  }
  PeakOverlay *po = new PeakOverlay(this, pws);
  po->setPrecision(m_peakLabelPrecision);
  po->setShowRowsFlag(m_showPeakRows);
  po->setShowLabelsFlag(m_showPeakLabels);
  po->setShowRelativeIntensityFlag(m_showPeakRelativeIntensity);

  if (!std::any_of(m_peakShapes.cbegin(), m_peakShapes.cend(),
                   [&pws](auto const &pkShape) { return pkShape->getPeaksWorkspace() == pws; }))
    m_peakShapes.append(po);

  m_startPeakShapes = true;
  connect(po, SIGNAL(executeAlgorithm(Mantid::API::IAlgorithm_sptr)), this,
          SIGNAL(executeAlgorithm(Mantid::API::IAlgorithm_sptr)));
  emit peaksWorkspaceAdded();
}

//-----------------------------------------------------------------------------
/** Create the peak labels from the peaks set by setPeaksWorkspace.
 * The method is called from the draw(...) method
 *
 */
void UnwrappedSurface::createPeakShapes() const {
  if (!m_peakShapes.isEmpty()) {
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    PeakOverlay &peakShapes = *m_peakShapes.last();
    PeakMarker2D::Style style = peakShapes.getDefaultStyle(m_peakShapesStyle);
    m_peakShapesStyle++;
    peakShapes.createMarkers(style);
    QApplication::restoreOverrideCursor();
  }
  m_startPeakShapes = false;
  setPeakVisibility();
}

/**
 * Toggle between flipped and straight view.
 */
void UnwrappedSurface::setFlippedView(bool on) {
  if (m_flippedView != on) {
    m_flippedView = on;
    m_viewRect.xFlip();
    for (int i = 0; i < m_zoomStack.size(); ++i) {
      m_zoomStack[i].xFlip();
    }
  }
}

/**
 * Calculate the QRect of a particular detector in units of pixels.
 * @param detectorIndex :: The index of the detector to calculate the QRect for.
 * @returns The bounding QRect of a particular detector in units of pixels.
 */
QRect UnwrappedSurface::detectorQRectInPixels(const std::size_t detectorIndex) const {
  const auto detIter =
      std::find_if(m_unwrappedDetectors.cbegin(), m_unwrappedDetectors.cend(),
                   [&detectorIndex](const auto &detector) { return detector.detIndex == detectorIndex; });

  if (detIter == m_unwrappedDetectors.cend()) {
    return QRect();
  }

  const auto expandedViewRect = correctForAspectRatioAndZoom(m_viewImage->width(), m_viewImage->height());

  const QSizeF viewSizeLogical(m_viewImage->width() / m_viewImage->devicePixelRatio(),
                               m_viewImage->height() / m_viewImage->devicePixelRatio());
  const double vwidth = viewSizeLogical.width();
  const double vheight = viewSizeLogical.height();
  const double dw = fabs(expandedViewRect.width() / vwidth);
  const double dh = fabs(expandedViewRect.height() / vheight);

  const auto detRect = detIter->toQRectF();

  // Calculate the centre position of the QRect. The x position will be different depending on if the view is flipped
  const auto xCentre = m_flippedView ? (expandedViewRect.x0() - detRect.center().x()) / dw
                                     : (detRect.center().x() - expandedViewRect.x0()) / dw;
  const auto yCentre = (detRect.center().y() - expandedViewRect.y0()) / dh;

  // Calculate the width and height of the QRect
  const auto size = QSize(static_cast<int>(detRect.width() / dw), static_cast<int>(detRect.height() / dh));
  // Calculate the position of the top left corner of the QRect
  const auto position =
      QPoint(static_cast<int>(xCentre) - static_cast<int>(size.width() / 2.0),
             static_cast<int>(vheight) - static_cast<int>(yCentre) - static_cast<int>(size.height() / 2.0));
  return QRect(position, size);
}

/**
 * Draw the surface onto an image without OpenGL
 * @param image :: Image to draw on.
 * @param picking :: If true draw a picking image.
 */
void UnwrappedSurface::drawSimpleToImage(QImage *image, bool picking) const {
  if (!image)
    return;

  QPainter paint(image);

  int vwidth = image->width();
  int vheight = image->height();

  paint.fillRect(0, 0, vwidth, vheight, m_backgroundColor);

  // expand the view area to maintain aspect ratio (could have used QPainter viewport feature but do it this way to
  // align with approach in drawSurface with OpenGL)
  auto expandedViewRect = correctForAspectRatioAndZoom(vwidth, vheight);

  QTransform transform;
  expandedViewRect.findTransform(transform, QRectF(0, 0, vwidth, vheight));
  paint.setTransform(transform);

  if (m_startPeakShapes) {
    createPeakShapes();
  }

  for (size_t i = 0; i < m_unwrappedDetectors.size(); ++i) {
    const UnwrappedDetector &udet = m_unwrappedDetectors[i];

    if (!(m_viewRect.contains(udet.u - udet.width / 2, udet.v - udet.height / 2) ||
          m_viewRect.contains(udet.u + udet.width / 2, udet.v + udet.height / 2)))
      continue;

    QColor color;
    int index = int(i);
    if (picking) {
      GLColor c = InstrumentRenderer::makePickColor(index);
      unsigned char r, g, b;
      c.get(r, g, b);
      color = QColor(r, g, b);
    } else {
      auto c = m_unwrappedDetectors[index].color;
      color = QColor(c.red(), c.green(), c.blue());
    }

    paint.fillRect(QRectF(udet.u - udet.width / 2, udet.v - udet.height / 2, udet.width, udet.height), color);
  }

  // draw custom stuff
  if (!picking) {
    drawCustom(&paint);
  }
}

/**
 * Zooms to the specified area. The previous zoom stack is cleared.
 */
void UnwrappedSurface::zoom(const QRectF &area) {
  if (!m_zoomStack.isEmpty()) {
    m_viewRect = m_zoomStack.first();
    m_zoomStack.clear();
  }
  m_zoomStack.push(m_viewRect);

  double left = area.left();
  double top = area.top();
  double width = area.width();
  double height = area.height();

  if (width * m_viewRect.width() < 0) {
    left += width;
    width = -width;
  }
  if (height * m_viewRect.height() < 0) {
    top += height;
    height = -height;
  }
  m_viewRect = RectF(QPointF(left, top), QPointF(left + width, top + height));
  updateView();
}

void UnwrappedSurface::unzoom() {
  if (!m_viewImage)
    return;

  RectF newView = selectionRectUV();
  if (newView.isEmpty())
    return;

  m_zoomStack.push(m_viewRect);

  auto area = newView.toQRectF();
  double left = area.left();
  double top = area.top();
  double width = area.width();
  double height = area.height();

  auto old = m_viewRect.toQRectF();
  double owidth = old.width();
  double oheight = old.height();

  auto newWidth = owidth / width * owidth;
  auto newHeight = oheight / height * oheight;
  auto newLeft = left + width / 2 - newWidth / 2;
  auto newTop = top + height / 2 - newHeight / 2;
  m_viewRect = RectF(QPointF(newLeft, newTop), QPointF(newLeft + newWidth, newTop + newHeight));

  updateView();
  emptySelectionRect();
  emit updateInfoText();
}

void UnwrappedSurface::resetZoom() {
  if (m_zoomStack.empty())
    return;

  m_viewRect = m_zoomStack.first();
  m_zoomStack.clear();
  updateView();
  emptySelectionRect();
  emit updateInfoText();
}

void UnwrappedSurface::zoom() {
  if (!m_viewImage)
    return;
  RectF newView = selectionRectUV();
  if (newView.isEmpty())
    return;
  m_zoomStack.push(m_viewRect);
  m_viewRect = newView;
  updateView();
  emptySelectionRect();
  emit updateInfoText();
}

//------------------------------------------------------------------------------
/** Calculate the UV and size of the given detector
 * Calls the pure virtual project() and calcSize() methods that
 * depend on the type of projection
 *
 * @param udet :: detector to unwrap.
 * @param pos :: detector position relative to the sample origin
 */
void UnwrappedSurface::calcUV(UnwrappedDetector &udet) {
  this->project(udet.detIndex, udet.u, udet.v, udet.uscale, udet.vscale);
  calcSize(udet);
}

//------------------------------------------------------------------------------
/** Calculate the size of the detector in U/V
 *
 * @param udet :: UwrappedDetector struct to calculate the size for. udet's size
 *fields
 * are updated by this method.
 */
void UnwrappedSurface::calcSize(UnwrappedDetector &udet) {
  // U is the horizontal axis on the screen
  constexpr Mantid::Kernel::V3D U(-1, 0, 0);
  // V is the vertical axis on the screen
  constexpr Mantid::Kernel::V3D V(0, 1, 0);

  // find the detector's rotation
  Mantid::Kernel::Quat R;
  this->rotate(udet, R);

  const auto &componentInfo = m_instrActor->componentInfo();
  const auto &bbox = componentInfo.shape(udet.detIndex).getBoundingBox();
  auto scale = componentInfo.scaleFactor(udet.detIndex);

  // sizes of the detector along each 3D axis
  Mantid::Kernel::V3D size = bbox.maxPoint() - bbox.minPoint();
  size *= scale;

  Mantid::Kernel::V3D s1(size);
  Mantid::Kernel::V3D s2 = size + Mantid::Kernel::V3D(-size.X(), 0, 0) - Mantid::Kernel::V3D(size.X(), 0, 0);
  Mantid::Kernel::V3D s3 = size + Mantid::Kernel::V3D(0, -size.Y(), 0) - Mantid::Kernel::V3D(0, size.Y(), 0);
  // rotate the size vectors to get the dimensions along axes U and V
  R.rotate(s1);
  R.rotate(s2);
  R.rotate(s3);

  // get the larges projection to the U axis which is the visible width
  double d = fabs(s1.scalar_prod(U));
  udet.width = d;
  d = fabs(s2.scalar_prod(U));
  if (d > udet.width)
    udet.width = d;
  d = fabs(s3.scalar_prod(U));
  if (d > udet.width)
    udet.width = d;

  // get the larges projection to the V axis which is the visible height
  d = fabs(s1.scalar_prod(V));
  udet.height = d;
  d = fabs(s2.scalar_prod(V));
  if (d > udet.height)
    udet.height = d;
  d = fabs(s3.scalar_prod(V));
  if (d > udet.height)
    udet.height = d;

  // apply the scale factors
  udet.width *= udet.uscale;
  udet.height *= udet.vscale;

  // don't let them be too large
  if (udet.width > m_width_max)
    m_width_max = udet.width;
  if (udet.height > m_height_max)
    m_height_max = udet.height;
}

/** Load unwrapped surface state from a Mantid project file
 * @param lines :: lines from the project file to load state from
 */
void UnwrappedSurface::loadFromProject(const std::string &lines) {
  Q_UNUSED(lines);
  throw std::runtime_error("UnwrappedSurface::loadFromProject() not implemented for Qt >= 5");
}

/**
 * Get a peaks workspace from the ADS
 * @param name :: name of the workspace to retrieve
 * @return a shared pointer to the fond peaks workspace
 */
Mantid::API::IPeaksWorkspace_sptr UnwrappedSurface::retrievePeaksWorkspace(const std::string &name) const {
  using namespace Mantid::API;
  Workspace_sptr ws = nullptr;

  try {
    ws = AnalysisDataService::Instance().retrieve(name);
  } catch (const std::runtime_error &) {
    // couldn't find the workspace in the ADS for some reason
    // just fail silently. There's nothing more we can do.
    return nullptr;
  }

  return std::dynamic_pointer_cast<IPeaksWorkspace>(ws);
}

/** Save the state of the unwrapped surface to a Mantid project file
 * @return a string representing the state of the surface
 */
std::string UnwrappedSurface::saveToProject() const {
  throw std::runtime_error("UnwrappedSurface::saveToProject() not implemented for Qt >= 5");
}

/**
 * Resize the surface on the screen.
 * @param w :: New width of the surface in pixels.
 * @param h :: New height of the surface in pixels.
 */
void UnwrappedSurface::resize(int w, int h) {
  m_widgetSize.setHeight(h);
  m_widgetSize.setWidth(w);
  updateView();
}

} // namespace MantidQt::MantidWidgets
