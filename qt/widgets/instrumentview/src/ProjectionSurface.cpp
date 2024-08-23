// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/InstrumentView/ProjectionSurface.h"
#include "MantidQtWidgets/Common/InputController.h"
#include "MantidQtWidgets/InstrumentView/GLColor.h"
#include "MantidQtWidgets/InstrumentView/GLDisplay.h"
#include "MantidQtWidgets/InstrumentView/InstrumentRenderer.h"
#include "MantidQtWidgets/InstrumentView/OpenGLError.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidKernel/Unit.h"

#include <QCursor>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QRgb>
#include <QSet>

#include "MantidKernel/V3D.h"
#include <QToolTip>
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <limits>
#include <utility>

using Mantid::Kernel::V3D;

namespace MantidQt::MantidWidgets {

/**
 * The constructor.
 * @param rootActor :: The instrument actor containing all info about the
 * instrument
 */
ProjectionSurface::ProjectionSurface(const IInstrumentActor *rootActor)
    : m_instrActor(rootActor), m_viewImage(nullptr), m_pickImage(nullptr), m_backgroundColor(Qt::black), m_viewRect(),
      m_selectRect(), m_interactionMode(MoveMode), m_isLightingOn(false), m_peakLabelPrecision(2),
      m_showPeakRows(false), m_showPeakLabels(false), m_showPeakRelativeIntensity(false), m_peakShapesStyle(0),
      m_viewChanged(true), m_redrawPicking(true) {
  connect(rootActor, SIGNAL(colorMapChanged()), this, SLOT(colorMapChanged()));
  connect(rootActor, SIGNAL(refreshView()), this, SLOT(refreshView()));
  connect(&m_maskShapes, SIGNAL(shapeCreated()), this, SIGNAL(shapeCreated()));
  connect(&m_maskShapes, SIGNAL(shapeSelected()), this, SIGNAL(shapeSelected()));
  connect(&m_maskShapes, SIGNAL(shapesDeselected()), this, SIGNAL(shapesDeselected()));
  connect(&m_maskShapes, SIGNAL(shapesRemoved()), this, SIGNAL(shapesRemoved()));
  connect(&m_maskShapes, SIGNAL(shapeChanged()), this, SIGNAL(shapeChanged()));
  connect(&m_maskShapes, SIGNAL(cleared()), this, SIGNAL(shapesCleared()));

  // create and connect the pick input controller
  InputControllerPick *pickController = new InputControllerPick(this);
  setInputController(PickSingleMode, pickController);
  setInputController(PickTubeMode, pickController);
  setInputController(AddPeakMode, pickController);
  connect(pickController, SIGNAL(pickPointAt(int, int)), this, SLOT(pickComponentAt(int, int)));
  connect(pickController, SIGNAL(touchPointAt(int, int)), this, SLOT(touchComponentAt(int, int)));

  // create and connect the mask drawing input controller
  InputControllerDrawShape *drawController = new InputControllerDrawShape(this);
  setInputController(EditShapeMode, drawController);
  connect(drawController, SIGNAL(addShape(QString, int, int, QColor, QColor)), &m_maskShapes,
          SLOT(addShape(QString, int, int, QColor, QColor)));
  connect(this, SIGNAL(signalToStartCreatingShape2D(QString, QColor, QColor)), drawController,
          SLOT(startCreatingShape2D(QString, QColor, QColor)));
  connect(drawController, SIGNAL(moveRightBottomTo(int, int)), &m_maskShapes, SLOT(moveRightBottomTo(int, int)));
  connect(drawController, SIGNAL(selectAt(int, int)), &m_maskShapes, SLOT(selectShapeOrControlPointAt(int, int)));
  connect(drawController, SIGNAL(selectCtrlAt(int, int)), &m_maskShapes, SLOT(addToSelectionShapeAt(int, int)));
  connect(drawController, SIGNAL(moveBy(int, int)), &m_maskShapes, SLOT(moveShapeOrControlPointBy(int, int)));
  connect(drawController, SIGNAL(touchPointAt(int, int)), &m_maskShapes, SLOT(touchShapeOrControlPointAt(int, int)));
  connect(drawController, SIGNAL(removeSelectedShapes()), &m_maskShapes, SLOT(removeSelectedShapes()));
  connect(drawController, SIGNAL(deselectAll()), &m_maskShapes, SLOT(deselectAll()));
  connect(drawController, SIGNAL(restoreOverrideCursor()), &m_maskShapes, SLOT(restoreOverrideCursor()));
  connect(drawController, SIGNAL(setSelection(QRect)), this, SLOT(setSelectionRect(QRect)));
  connect(drawController, SIGNAL(finishSelection(QRect)), this, SLOT(selectMultipleMasks(QRect)));
  connect(drawController, SIGNAL(finishSelection(QRect)), this, SIGNAL(shapeChangeFinished()));
  connect(drawController, SIGNAL(copySelectedShapes()), &m_maskShapes, SLOT(copySelectedShapes()));
  connect(drawController, SIGNAL(pasteCopiedShapes()), &m_maskShapes, SLOT(pasteCopiedShapes()));

  InputControllerDrawAndErase *freeDrawController = new InputControllerDrawAndErase(this);
  setInputController(DrawFreeMode, freeDrawController);
  connect(this, SIGNAL(signalToStartCreatingFreeShape(QColor, QColor)), freeDrawController,
          SLOT(startCreatingShape2D(QColor, QColor)));
  connect(freeDrawController, SIGNAL(addShape(const QPolygonF &, QColor, QColor)), &m_maskShapes,
          SLOT(addFreeShape(const QPolygonF &, QColor, QColor)));
  connect(freeDrawController, SIGNAL(draw(const QPolygonF &)), &m_maskShapes, SLOT(drawFree(const QPolygonF &)));
  connect(freeDrawController, SIGNAL(erase(const QPolygonF &)), &m_maskShapes, SLOT(eraseFree(const QPolygonF &)));

  // create and connect the peak eraser controller
  auto eraseIcon = new QPixmap(":/PickTools/eraser.png");
  InputControllerSelection *eraseController = new InputControllerSelection(this, eraseIcon);
  setInputController(ErasePeakMode, eraseController);
  connect(eraseController, SIGNAL(selection(QRect)), this, SLOT(erasePeaks(QRect)));

  // create and connect the peak compare controller
  auto selectIcon = new QPixmap(":/PickTools/selection-pointer.png");
  InputControllerSelection *compareController = new InputControllerSelection(this, selectIcon);
  setInputController(ComparePeakMode, compareController);
  connect(compareController, SIGNAL(selection(QRect)), this, SLOT(comparePeaks(QRect)));

  // create and connect the peak alignment controller
  auto alignIcon = new QPixmap(":/PickTools/selection-pointer.png");
  InputControllerSelection *alignController = new InputControllerSelection(this, alignIcon);
  setInputController(AlignPeakMode, alignController);
  connect(alignController, SIGNAL(selection(QRect)), this, SLOT(alignPeaks(QRect)));
}

ProjectionSurface::~ProjectionSurface() {
  if (m_viewImage) {
    delete m_viewImage;
  }
  if (m_pickImage) {
    delete m_pickImage;
  }
  for (auto &peakShape : m_peakShapes) {
    if (peakShape)
      delete peakShape;
  }
  m_peakShapes.clear();
}

/**
 * @brief ProjectionSurface::toggleToolTip
 * Connect or disconnect all controllers reporting for tooltip when a component is touched
 * @param activateToolTip the new status
 */
void ProjectionSurface::toggleToolTip(bool activateToolTip) {
  for (auto controller : m_inputControllers) {
    if (activateToolTip) {
      connect(controller, SIGNAL(touchPointAt(int, int)), this, SLOT(showToolTip(int, int)));
    } else
      disconnect(controller, SIGNAL(touchPointAt(int, int)), this, SLOT(showToolTip(int, int)));
  }
}
/**
 * Resets the instrument actor. The caller must ensure that the instrument
 * stays the same and workspace dimensions also don't change.
 */
void ProjectionSurface::resetInstrumentActor(const IInstrumentActor *rootActor) {
  m_instrActor = rootActor;
  connect(rootActor, SIGNAL(colorMapChanged()), this, SLOT(colorMapChanged()));
}

void ProjectionSurface::clear() {
  if (m_viewImage) {
    delete m_viewImage;
    m_viewImage = nullptr;
  }
  if (m_pickImage) {
    delete m_pickImage;
    m_pickImage = nullptr;
  }
  m_viewChanged = true;
  m_redrawPicking = true;
  m_viewRect = RectF();
  m_selectRect = QRect();
}

/**
 * Draw the surface on an OpenGL widget
 * @param widget :: A widget to draw on.
 */
void ProjectionSurface::draw(GLDisplay *widget) const {
  if (m_viewChanged && (m_redrawPicking || m_interactionMode == PickSingleMode || m_interactionMode == PickTubeMode ||
                        m_interactionMode == EditShapeMode)) {
    draw(widget, true);
    m_redrawPicking = false;
  }
  draw(widget, false);
  if (m_viewChanged) {
    m_viewChanged = false;
  }
}

/**
 * Draw the surface on an OpenGL widget.
 * @param widget :: A widget to draw on.
 * @param picking :: Picking / normal drawing switch.
 */
void ProjectionSurface::draw(GLDisplay *widget, bool picking) const {
  // By default the viewport and window on the QPainter is set to
  // the dimensions of the number of physical pixels on the device
  // when used with an OpenGLWidget whereas methods like widget->width()/height()
  // return logical pixels. All subsequent commands for 2D
  // overlays assume logical pixels so we configure the painter
  // such that the internal transformations take care o the mapping of logical
  // to physical appropriately.
  auto configurePainter = [widget](QPainter &painter) {
    painter.setWindow(0, 0, widget->width(), widget->height());
    painter.setViewport(0, 0, widget->width(), widget->height());
  };

  QImage **image = picking ? &m_pickImage : &m_viewImage;
  if (m_viewChanged) {
    this->drawSurface(widget, picking);
    if (*image) {
      delete (*image);
    }
    (*image) = new QImage(widget->grabFramebuffer());
    // The QImage dimensions will match the number of physical pixels
    // but the view image is used in determining transformations for zooming
    // where logical pixels are used. Storing the devicePixelRatio allows the
    // information to be retrieved when necessary.
    (*image)->setDevicePixelRatio(widget->devicePixelRatio());

    if (!picking) {
      QPainter painter(widget);
      configurePainter(painter);
      drawMaskShapes(painter);
      drawPeakMarkers(painter);
      drawPeakComparisonLine(painter);
      drawPeakAlignmentMarkers(painter);

      painter.end();
    }
  } else if (!picking) {
    QPainter painter(widget);
    configurePainter(painter);
    painter.drawImage(0, 0, **image);
    drawMaskShapes(painter);
    drawPeakMarkers(painter);
    drawPeakComparisonLine(painter);
    drawPeakAlignmentMarkers(painter);
    drawSelectionRect(painter);

    getController()->onPaint(painter);
    painter.end();
    // Discard any error generated here
    GLuint ecode = glGetError();
    OpenGLError::logDebug() << "Discarding OpenGL error: " << gluErrorString(ecode);
  }
}

/**
 * Draw the surface onto a normal widget without OpenGL
 * @param widget :: A widget to draw on.
 */
void ProjectionSurface::drawSimple(QWidget *widget) const {
  if (m_viewChanged) {
    if (!m_viewImage || m_viewImage->width() != widget->width() || m_viewImage->height() != widget->height()) {
      if (m_viewImage)
        delete m_viewImage;
      m_viewImage = new QImage(widget->width(), widget->height(), QImage::Format_RGB32);
      if (m_pickImage)
        delete m_pickImage;
      m_pickImage = new QImage(widget->width(), widget->height(), QImage::Format_RGB32);
    }

    if (m_redrawPicking || m_interactionMode == PickSingleMode || m_interactionMode == PickTubeMode) {
      drawSimpleToImage(m_pickImage, true);
      m_redrawPicking = false;
    }
    drawSimpleToImage(m_viewImage, false);
    m_viewChanged = false;
  }

  QPainter painter(widget);
  painter.drawImage(0, 0, *m_viewImage);

  drawMaskShapes(painter);
  drawPeakMarkers(painter);
  drawPeakComparisonLine(painter);
  drawPeakAlignmentMarkers(painter);
  drawSelectionRect(painter);

  getController()->onPaint(painter);
  painter.end();
}

void ProjectionSurface::resize(int /*unused*/, int /*unused*/) { updateView(); }

/**
 * Draw the surface onto an image without OpenGL
 * @param image :: Image to draw on.
 * @param picking :: If true draw a picking image.
 */
void ProjectionSurface::drawSimpleToImage(QImage * /*unused*/, bool /*unused*/) const {}

void ProjectionSurface::mousePressEvent(QMouseEvent *e) { getController()->mousePressEvent(e); }

void ProjectionSurface::mouseMoveEvent(QMouseEvent *e) { getController()->mouseMoveEvent(e); }

void ProjectionSurface::mouseReleaseEvent(QMouseEvent *e) { getController()->mouseReleaseEvent(e); }

void ProjectionSurface::wheelEvent(QWheelEvent *e) { getController()->wheelEvent(e); }

void ProjectionSurface::keyPressEvent(QKeyEvent *e) { getController()->keyPressEvent(e); }

void ProjectionSurface::enterEvent(QEvent *e) { getController()->enterEvent(e); }

void ProjectionSurface::leaveEvent(QEvent *e) { getController()->leaveEvent(e); }

/**
 * Update the view of the surface at the next redraw.
 * @param picking :: Set to true to update the picking image regardless the
 * interaction
 *   mode of the surface.
 */
void ProjectionSurface::updateView(bool picking) {
  m_viewChanged = true;
  if (picking) {
    // don't change to false if it's already true
    m_redrawPicking = true;
  }
}

void ProjectionSurface::updateDetectors() {
  // updating detectors should not reset the view rect
  // cache the value here are reapply after updating the detectors
  auto viewRectCache = m_viewRect;
  clear();
  this->init();
  // if integration range in the instrument actor has changed
  // update visiblity of peak markers
  setPeakVisibility();
  m_viewRect = viewRectCache;
}

/// Send a redraw request to the surface owner
void ProjectionSurface::requestRedraw(bool resetPeakVisibility) {
  if (resetPeakVisibility) {
    setPeakVisibility();
  }
  emit redrawRequired();
}

QRect ProjectionSurface::selectionRect() const {
  if (m_selectRect.width() <= 1 || m_selectRect.height() <= 1)
    return QRect();

  int x_min = m_selectRect.left();
  int x_size = m_selectRect.width();
  int y_min = m_selectRect.top();
  int y_size = m_selectRect.height();

  if (x_size < 0) {
    x_min += x_size;
    x_size = abs(x_size);
  }

  if (y_size < 0) {
    y_min += y_size;
    y_size = abs(y_size);
  }

  return QRect(x_min, y_min, x_size, y_size);
}

bool ProjectionSurface::hasSelection() const { return !m_selectRect.isNull() && m_selectRect.width() > 0; }

void ProjectionSurface::colorMapChanged() {
  this->changeColorMap();
  updateView(false);
  requestRedraw();
}

void ProjectionSurface::refreshView() {
  updateView(false);
  requestRedraw();
}

/**
 * Set an interaction mode for the surface.
 * @param mode :: A new mode.
 */
void ProjectionSurface::setInteractionMode(int mode) {
  if (mode < 0 || mode >= m_inputControllers.size()) {
    throw std::logic_error("Invalid interaction mode requested.");
  }
  if (mode == m_interactionMode)
    return;
  InputController *controller = m_inputControllers[m_interactionMode];
  if (!controller)
    throw std::logic_error("Input controller doesn't exist.");
  controller->onDisabled();
  m_interactionMode = mode;
  controller = m_inputControllers[m_interactionMode];
  if (!controller)
    throw std::logic_error("Input controller doesn't exist.");
  controller->onEnabled();
  if (mode != EditShapeMode && mode != DrawFreeMode) {
    m_maskShapes.deselectAll();
    for (auto peakOverlay : std::as_const(m_peakShapes)) {
      peakOverlay->deselectAll();
    }
  }
}

/**
 * Return detector id at image coordinats x,y if in pick mode. -1 otherwise
 */
int ProjectionSurface::getDetectorID(int x, int y) const {
  size_t pickID = getPickID(x, y);
  return m_instrActor->getDetID(pickID);
}

//------------------------------------------------------------------------------
size_t ProjectionSurface::getDetector(int x, int y) const { return getPickID(x, y); }

/**
 * Return info text for interactions common to all surfaces.
 */
QString ProjectionSurface::getInfoText() const {
  switch (m_interactionMode) {
  case PickSingleMode:
  case PickTubeMode:
    return "Move cursor over instrument to see detector information. ";
  case AddPeakMode:
    return "Click on a detector then click on the mini-plot to add a peak.";
  case EditShapeMode:
    return "Select a tool button to draw a new shape. "
           "Click on shapes to select. Click and move to edit. Press Ctrl+C "
           "/ Ctrl+V to copy/paste. Press Delete key to remove a shape.";
  case DrawFreeMode:
    return "Draw by holding the left button down. "
           "Erase with the right button.";
  case ComparePeakMode:
    return "Click on one peak, then click on another to compare peaks.";
  case ErasePeakMode:
    return "Click and move the mouse to erase peaks. "
           "Rotate the wheel to resize the cursor.";
  }
  return "";
}

//------------------------------------------------------------------------------
/** Return the detector position (in real-space) at the pixel coordinates.
 *
 * @param x :: x pixel coordinate
 * @param y :: y pixel coordinate
 * @return V3D of the detector position
 */
Mantid::Kernel::V3D ProjectionSurface::getDetectorPos(int x, int y) const {
  size_t pickID = getPickID(x, y);
  return m_instrActor->getDetPos(pickID);
}

/**
 * Is context menu allowed?
 */
bool ProjectionSurface::canShowContextMenu() const {
  InputController *controller = m_inputControllers[m_interactionMode];
  if (controller) {
    return controller->canShowContextMenu();
  }
  return false;
}

//------------------------------------------------------------------------------
/**
 * Return the unique "pick ID" that identifies a detector index from
 * the given 2D position in the image.
 * @param x The X coordinate in logical pixels
 * @param y The Y coordinate in logical pixels
 */
size_t ProjectionSurface::getPickID(int x, int y) const {
  if (!m_pickImage)
    return -1;

  // OpenGL canvases on high-pixel density monitors have a higher number of physical
  // pixels in the QImage. The pick coordinates are in logical coordinates so we need to scale them
  auto toImageCoord = [this](int logical) {
    return static_cast<int>(std::lround(logical * m_pickImage->devicePixelRatio()));
  };

  const int imageX(toImageCoord(x)), imageY(toImageCoord(y));
  if (!m_pickImage->valid(imageX, imageY))
    return -1;

  QRgb pixel = m_pickImage->pixel(imageX, imageY);
  return InstrumentRenderer::decodePickColor(pixel);
}

/**
 * Adds an input controller to the controller list.
 * @param mode :: The interaction mode which is the index of the controller in
 * the list.
 * @param controller :: A pointer to the controller to be set.
 */
void ProjectionSurface::setInputController(int mode, InputController *controller) {
  m_inputControllers[mode] = controller;
}

/**
 * Set visibility of the peak markers according to the integration range
 * in the instrument actor.
 */
void ProjectionSurface::setPeakVisibility() const {
  if (hasPeakOverlays()) {
    Mantid::Kernel::Unit_sptr unit = m_instrActor->getWorkspace()->getAxis(0)->unit();
    QString unitID = QString::fromStdString(unit->unitID());
    double xmin = m_instrActor->minBinValue();
    double xmax = m_instrActor->maxBinValue();
    for (auto peakOverlay : std::as_const(m_peakShapes)) {
      peakOverlay->setPeakVisibility(xmin, xmax, unitID);
    }
  }
}

/** Check a peak is visible at the given point
 *
 * Will return true if any peak in any overlay was found to be positioned at
 * the given point.
 *
 * @param point :: the point to check for peaks
 * @return true if any peaks was found at the given point
 */
bool ProjectionSurface::peakVisibleAtPoint(const QPointF &point) const {
  for (const auto po : m_peakShapes) {
    po->selectAtXY(point);
    auto markers = po->getSelectedPeakMarkers();
    bool visible =
        std::any_of(markers.begin(), markers.end(), [](PeakMarker2D *marker) { return marker->isVisible(); });
    if (visible) {
      return true;
    }
  }
  return false;
}

/**
 * Draw a line between peak markers being compared
 * @param painter :: The QPainter object to draw the line with
 */
void ProjectionSurface::drawPeakComparisonLine(QPainter &painter) const {
  const auto &firstOrigin = m_selectedMarkers.first;
  const auto &secondOrigin = m_selectedMarkers.second;

  // Check is user has selected enough peaks
  if (firstOrigin.isNull() || secondOrigin.isNull())
    return;

  // Check if the integration range is such that some peaks are visible
  if (!peakVisibleAtPoint(firstOrigin) || !peakVisibleAtPoint(secondOrigin))
    return;

  // Draw line between peaks
  QTransform transform;
  auto windowRect = getSurfaceBounds();
  windowRect.findTransform(transform, painter.viewport());
  auto p1 = transform.map(firstOrigin);
  auto p2 = transform.map(secondOrigin);
  painter.setPen(Qt::red);
  painter.drawLine(p1, p2);
}

/**
 * Draw the peak marker objects on the surface
 * @param painter :: The QPainter object to draw the markers with
 */
void ProjectionSurface::drawPeakMarkers(QPainter &painter) const {
  auto windowRect = getSurfaceBounds();
  for (auto &peakShape : m_peakShapes) {
    peakShape->setWindow(windowRect, painter.viewport());
    peakShape->draw(painter);
  }
}

/**
 * Draw the mask shapes on the surface
 * @param painter :: The QPainter object to draw the masks with
 */
void ProjectionSurface::drawMaskShapes(QPainter &painter) const {
  RectF windowRect = getSurfaceBounds();
  m_maskShapes.setWindow(windowRect, painter.viewport());
  m_maskShapes.draw(painter);
}

/**
 * Draw the selection rectangle on the surface
 * @param painter :: The QPainter object to draw the rectangle with
 */
void ProjectionSurface::drawSelectionRect(QPainter &painter) const {
  // draw the selection rectangle
  if (!m_selectRect.isNull()) {
    painter.setPen(Qt::blue);
    // painter.setCompositionMode(QPainter::CompositionMode_Xor);
    painter.drawRect(m_selectRect);
  }
}

/**
 * Draw the peak alignment marker objects on the surface
 * @param painter :: The QPainter object to draw the markers with
 */
void ProjectionSurface::drawPeakAlignmentMarkers(QPainter &painter) const {
  QTransform transform;
  auto windowRect = getSurfaceBounds();
  windowRect.findTransform(transform, painter.viewport());

  auto outOfPlanePoint = m_selectedAlignmentPeak.second;
  // draw the 4th peak in a different colour
  if (!outOfPlanePoint.isNull()) {
    painter.setPen(Qt::green);
    auto point = transform.map(outOfPlanePoint);
    painter.drawEllipse(point, 8, 8);
  }

  // draw highlight around the first three peaks
  QPolygonF poly;
  painter.setPen(Qt::blue);
  for (auto &item : m_selectedAlignmentPlane) {
    auto origin = item.second;
    if (origin != outOfPlanePoint) {
      auto point = transform.map(origin);
      painter.drawEllipse(point, 8, 8);
    }
  }
}

/**
 * Returns the current controller. If the controller doesn't exist throws a
 * logic_error exceotion.
 */
InputController *ProjectionSurface::getController() const {
  InputController *controller = m_inputControllers[m_interactionMode];
  if (!controller) {
    throw std::logic_error("Input controller doesn't exist for current interaction mode.");
  }
  return controller;
}

void ProjectionSurface::freezeRotation(bool freeze) { getController()->freezeRotation(freeze); }

// --- Shape2D manipulation --- //

void ProjectionSurface::startCreatingShape2D(const QString &type, const QColor &borderColor, const QColor &fillColor) {
  emit signalToStartCreatingShape2D(type, borderColor, fillColor);
}

void ProjectionSurface::startCreatingFreeShape(const QColor &borderColor, const QColor &fillColor) {
  emit signalToStartCreatingFreeShape(borderColor, fillColor);
}

/**
 * Save shapes drawn on the view to a table workspace
 */
void ProjectionSurface::saveShapesToTableWorkspace() { m_maskShapes.saveToTableWorkspace(); }

/**
 * Load shapes from a table workspace on to the view.
 * @param ws :: table workspace to load shapes from
 */
void ProjectionSurface::loadShapesFromTableWorkspace(const Mantid::API::ITableWorkspace_const_sptr &ws) {
  m_maskShapes.loadFromTableWorkspace(ws);
}

/**
 * Draw a 2D shape onto the surface with the given coordinates
 * @param type :: The shape type such as 'rectangle'
 * @param borderColor :: The color of the shapes border
 * @param fillColor :: The color of the shapes fill
 * @param topLeftPos :: The position of the top left corner in pixels
 * @param bottomRightPos :: The position of the bottom right corner in pixels
 * @param select :: True if you want the shape to be selected immediately after drawing
 */
void ProjectionSurface::drawShape2D(const QString &type, const QColor &borderColor, const QColor &fillColor,
                                    const QPoint &topLeftPos, const QPoint &bottomRightPos, const bool select) {
  m_maskShapes.addShape(type, topLeftPos.x(), topLeftPos.y(), borderColor, fillColor);
  m_maskShapes.moveRightBottomTo(bottomRightPos.x(), bottomRightPos.y());
  if (!select) {
    m_maskShapes.deselectAll();
  }
}

void ProjectionSurface::clearMaskedShapes() { m_maskShapes.clear(); }

/**
 * Return a combined list of peak parkers from all overlays
 * @param detID :: The detector ID of interest
 */
QList<PeakMarker2D *> ProjectionSurface::getMarkersWithID(int detID) const {
  QList<PeakMarker2D *> out;
  for (auto &peakShape : m_peakShapes) {
    out += peakShape->getMarkersWithID(detID);
  }
  return out;
}

/**
 * Get peaks workspace for manually editing.
 */
std::shared_ptr<Mantid::API::IPeaksWorkspace> ProjectionSurface::getEditPeaksWorkspace() const {
  if (!m_peakShapes.isEmpty()) {
    return m_peakShapes.last()->getPeaksWorkspace();
  }
  return std::shared_ptr<Mantid::API::IPeaksWorkspace>();
}

/**
 * Remove an overlay if its peaks workspace is deleted.
 * @param ws :: Shared pointer to the deleted peaks workspace.
 */
void ProjectionSurface::deletePeaksWorkspace(const std::shared_ptr<Mantid::API::IPeaksWorkspace> &ws) {
  const int npeaks = m_peakShapes.size();
  for (int i = 0; i < npeaks; ++i) {
    if (m_peakShapes[i]->getPeaksWorkspace() == ws) {
      delete m_peakShapes[i];
      m_peakShapes.removeAt(i);
      break;
    }
  }
  if (m_peakShapes.size() < npeaks) {
    emit peaksWorkspaceDeleted();
  }
}

/**
 * Remove all peak overlays.
 */
void ProjectionSurface::clearPeakOverlays() {
  if (!m_peakShapes.isEmpty()) {
    for (auto &peakShape : m_peakShapes) {
      delete peakShape;
    }
    m_peakShapes.clear();
    m_peakShapesStyle = 0;
    emit peaksWorkspaceDeleted();
  }

  clearAlignmentPlane();
  clearComparisonPeaks();
}

/**
 * Remove all peaks used to define alignment plane
 */
void ProjectionSurface::clearAlignmentPlane() {
  m_selectedAlignmentPlane.clear();
  m_selectedAlignmentPeak = std::make_pair(nullptr, QPointF());
}

/**
 * Remove all peaks used to define comparison peaks
 */
void ProjectionSurface::clearComparisonPeaks() {
  m_selectedPeaks.first.clear();
  m_selectedPeaks.second.clear();
  m_selectedMarkers.first = QPointF();
  m_selectedMarkers.second = QPointF();
}

/**
 * Set the precision (significant digits) with which the HKL peak labels are
 * displayed.
 * @param n :: Precision, > 0
 */
void ProjectionSurface::setPeakLabelPrecision(int n) {
  if (n < 1) {
    QMessageBox::critical(nullptr, "Mantid - Error", "Precision must be a positive number");
    return;
  }
  m_peakLabelPrecision = n;
  for (auto &peakShape : m_peakShapes) {
    peakShape->setPrecision(n);
  }
}

/**
 * Enable or disable the show peak row flag
 */
void ProjectionSurface::setShowPeakRowsFlag(bool on) {
  m_showPeakRows = on;
  for (auto &peakShape : m_peakShapes) {
    peakShape->setShowRowsFlag(on);
  }
}

/**
 * Enable or disable the show peak label flag
 */
void ProjectionSurface::setShowPeakLabelsFlag(bool on) {
  m_showPeakLabels = on;
  for (auto &peakShape : m_peakShapes) {
    peakShape->setShowLabelsFlag(on);
  }
}

/**
 * Enable or disable the show peak label flag
 */
void ProjectionSurface::setShowPeakRelativeIntensityFlag(bool on) {
  m_showPeakRelativeIntensity = on;
  for (auto &peakShape : m_peakShapes) {
    peakShape->setShowRelativeIntensityFlag(on);
  }
}

/**
 * Set the selection rect in screen corrds.
 * @param rect :: New selection rectangle.
 */
void ProjectionSurface::setSelectionRect(const QRect &rect) {
  if (m_interactionMode != EditShapeMode || !m_maskShapes.hasSelection()) {
    m_selectRect = rect;
  }
}

/**
 * Delete selection rectanle.
 */
void ProjectionSurface::emptySelectionRect() { m_selectRect = QRect(); }

/**
 * Select multiple mask shapes as a result of a rubber-band selection
 * @param rect :: The rubber band rect.
 */
void ProjectionSurface::selectMultipleMasks(const QRect &rect) {
  if (!m_maskShapes.hasSelection()) {
    m_maskShapes.selectIn(rect);
  }
  emptySelectionRect();
}

/**
 * Pick a detector at a point on the screen.
 */
void ProjectionSurface::pickComponentAt(int x, int y) {
  size_t pickID = getPickID(x, y);
  if (m_currentTab == "Draw") {
    emit singleComponentPickedForMasking(pickID);
  } else {
    emit singleComponentPicked(pickID);
  }
}

void ProjectionSurface::touchComponentAt(int x, int y) {
  size_t pickID = getPickID(x, y);
  emit singleComponentTouched(pickID);
}

/**
 * @brief Show a tooltip with basic info on the surface
 * @param x the x coordinate of the picked point on the screen
 * @param y the y coordinate of the picked point on the screen
 */
void ProjectionSurface::showToolTip(int x, int y) {
  size_t pickID = getPickID(x, y);

  const auto &componentInfo = m_instrActor->componentInfo();

  if (componentInfo.isDetector(pickID)) {
    QString text;

    text += "Detector: " + QString::fromStdString(componentInfo.name(pickID)) + '\n';

    const double integrated = m_instrActor->getIntegratedCounts(pickID);
    const QString counts = integrated == InstrumentActor::INVALID_VALUE ? "N/A" : QString::number(integrated);
    text += "Counts: " + counts;
    QToolTip::showText(QCursor::pos(), text);
  }
}

void ProjectionSurface::erasePeaks(const QRect &rect) {
  for (auto po : m_peakShapes) {
    po->selectIn(rect);
    auto peakMarkers = po->getSelectedPeakMarkers();

    // clear selected peak markers
    for (const auto &marker : peakMarkers) {
      auto peak = po->getPeaksWorkspace()->getPeakPtr(marker->getRow());
      if (!peak)
        continue;

      if ((!m_selectedPeaks.first.empty() && m_selectedPeaks.first.front() == peak) ||
          (!m_selectedPeaks.second.empty() && m_selectedPeaks.second.front() == peak)) {
        clearComparisonPeaks();
      }

      // check if erased peak matches one of our alignment peaks
      auto result =
          std::find_if(m_selectedAlignmentPlane.cbegin(), m_selectedAlignmentPlane.cend(),
                       [peak](const std::pair<V3D, QPointF> &item) { return item.first == peak->getQSampleFrame(); });

      if (result != m_selectedAlignmentPlane.cend()) {
        clearAlignmentPlane();
      }
    }

    po->removeSelectedShapes();
  }
}

void ProjectionSurface::comparePeaks(const QRect &rect) {
  // Find the selected peak across all of the peak overlays.
  QPointF origin;
  std::vector<Mantid::Geometry::IPeak *> peaks;
  for (auto *po : m_peakShapes) {
    po->selectIn(rect);
    const auto markers = po->getSelectedPeakMarkers();

    // make the assumption that the first peak found in the recticle is the one
    // we wanted.
    if (markers.length() > 0 && origin.isNull()) {
      origin = markers.first()->origin();
    }

    for (const auto &marker : markers) {
      // only collect peaks in the same detector & with the same origin
      if (marker->origin() == origin) {
        auto peak = po->getPeaksWorkspace()->getPeakPtr(marker->getRow());
        peaks.emplace_back(peak);
      }
    }
  }

  if (m_selectedPeaks.first.empty()) {
    // No peaks have been selected yet
    m_selectedPeaks.first = peaks;
    m_selectedMarkers.first = origin;
  } else if (m_selectedPeaks.second.empty()) {
    // Two peaks have now been selected
    m_selectedPeaks.second = peaks;
    m_selectedMarkers.second = origin;
  } else if (!m_selectedPeaks.first.empty() && !m_selectedPeaks.second.empty()) {
    // Two peaks have already been selected. Clear the pair and store
    // the new peak as the first entry
    m_selectedPeaks.first = peaks;
    m_selectedMarkers.first = origin;
    m_selectedPeaks.second.clear();
    m_selectedMarkers.second = QPointF();
  }

  // Only emit the signal to update when we have two peaks
  if (!m_selectedPeaks.first.empty() && !m_selectedPeaks.second.empty()) {
    emit comparePeaks(m_selectedPeaks);
  }
}

void ProjectionSurface::alignPeaks(const QRect &rect) {
  using Mantid::Geometry::IPeak;
  PeakMarker2D *marker = nullptr;
  IPeak *peak = nullptr;
  QPointF origin;

  for (auto po : m_peakShapes) {
    po->selectIn(rect);
    const auto markers = po->getSelectedPeakMarkers();
    if (markers.length() > 0) {
      marker = markers.first();
      origin = marker->origin();
      peak = po->getPeaksWorkspace()->getPeakPtr(marker->getRow());
      break;
    }
  }

  // check we found a peak
  if (!marker || !peak)
    return;

  if (m_selectedAlignmentPlane.size() < 2) {
    // check Q value is not already in the plane list
    // We only want unique vectors to define the plane
    const auto result =
        std::find_if(m_selectedAlignmentPlane.cbegin(), m_selectedAlignmentPlane.cend(),
                     [peak](const std::pair<V3D, QPointF> &item) { return item.first == peak->getQSampleFrame(); });

    if (result == m_selectedAlignmentPlane.cend()) {
      m_selectedAlignmentPlane.emplace_back(peak->getQSampleFrame(), origin);
    }
  } else {
    m_selectedAlignmentPeak = std::make_pair(peak, origin);
  }

  if (m_selectedAlignmentPlane.size() >= 2 && m_selectedAlignmentPeak.first) {
    // create vector V3Ds for the plane
    std::vector<Mantid::Kernel::V3D> qValues;
    std::transform(m_selectedAlignmentPlane.begin(), m_selectedAlignmentPlane.end(), std::back_inserter(qValues),
                   [](const std::pair<V3D, QPointF> &item) { return item.first; });
    emit alignPeaks(qValues, m_selectedAlignmentPeak.first);
  }
}

/**
 * Enable or disable lighting in non-picking mode
 * @param on :: True for enabling, false for disabling.
 */
void ProjectionSurface::enableLighting(bool on) { m_isLightingOn = on; }

/**
 * Return names of attached peaks workspaces.
 */
QStringList ProjectionSurface::getPeaksWorkspaceNames() const {
  QStringList names;
  for (auto peakOverlay : std::as_const(m_peakShapes)) {
    names << QString::fromStdString(peakOverlay->getPeaksWorkspace()->getName());
  }
  return names;
}

/** Load projection surface state from a Mantid project file
 * @param lines :: lines from the project file to load state from
 */
void ProjectionSurface::loadFromProject(const std::string &lines) {
  Q_UNUSED(lines);
  throw std::runtime_error("ProjectionSurface::loadFromProject() not implemented for Qt >= 5");
}

/** Save the state of the projection surface to a Mantid project file
 * @return a string representing the state of the projection surface
 */
std::string ProjectionSurface::saveToProject() const {
  throw std::runtime_error("ProjectionSurface::loadsaveToProject() not implemented for Qt >= 5");
}

} // namespace MantidQt::MantidWidgets
