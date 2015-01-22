#include "ProjectionSurface.h"
#include "GLColor.h"
#include "MantidGLWidget.h"
#include "OpenGLError.h"
#include "InputController.h"

#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidAPI/IPeaksWorkspace.h"

#include <QRgb>
#include <QSet>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QMessageBox>

#include <cfloat>
#include <limits>
#include <cmath>
#include <algorithm>
#include "MantidKernel/V3D.h"

using Mantid::Kernel::V3D;

/**
  * The constructor.
  * @param rootActor :: The instrument actor containing all info about the instrument
  */
ProjectionSurface::ProjectionSurface(const InstrumentActor* rootActor):
    m_instrActor(rootActor),
    m_viewImage(NULL),
    m_pickImage(NULL),
    m_viewRect(),
    m_selectRect(),
    m_interactionMode(MoveMode),
    m_isLightingOn(false),
    m_peakLabelPrecision(2),
    m_peakShapesStyle(0),
    m_viewChanged(true),
    m_redrawPicking(true)
{
  connect(rootActor,SIGNAL(colorMapChanged()),this,SLOT(colorMapChanged()));
  connect(&m_maskShapes,SIGNAL(shapeCreated()),this,SIGNAL(shapeCreated()));
  connect(&m_maskShapes,SIGNAL(shapeSelected()),this,SIGNAL(shapeSelected()));
  connect(&m_maskShapes,SIGNAL(shapesDeselected()),this,SIGNAL(shapesDeselected()));
  connect(&m_maskShapes,SIGNAL(shapesRemoved()),this,SIGNAL(shapesRemoved()));
  connect(&m_maskShapes,SIGNAL(shapeChanged()),this,SIGNAL(shapeChanged()));
  connect(&m_maskShapes,SIGNAL(cleared()),this,SIGNAL(shapesCleared()));

  // create and connect the pick input controller
  InputControllerPick* pickController = new InputControllerPick(this);
  setInputController(PickSingleMode, pickController);
  setInputController(PickTubeMode, pickController);
  setInputController(AddPeakMode, pickController);
  connect(pickController,SIGNAL(pickPointAt(int,int)),this,SLOT(pickComponentAt(int,int)));
  connect(pickController,SIGNAL(touchPointAt(int,int)),this,SLOT(touchComponentAt(int,int)));

  // create and connect the mask drawing input controller
  InputControllerDrawShape* drawController = new InputControllerDrawShape(this);
  setInputController(DrawMode, drawController);
  connect(drawController,SIGNAL(addShape(QString,int,int,QColor,QColor)),&m_maskShapes,SLOT(addShape(QString,int,int,QColor,QColor)));
  connect(this,SIGNAL(signalToStartCreatingShape2D(QString,QColor,QColor)),drawController,SLOT(startCreatingShape2D(QString,QColor,QColor)));
  connect(drawController,SIGNAL(moveRightBottomTo(int,int)),&m_maskShapes,SLOT(moveRightBottomTo(int,int)));
  connect(drawController,SIGNAL(selectAt(int,int)),&m_maskShapes,SLOT(selectShapeOrControlPointAt(int,int)));
  connect(drawController,SIGNAL(selectCtrlAt(int,int)),&m_maskShapes,SLOT(addToSelectionShapeAt(int,int)));
  connect(drawController,SIGNAL(moveBy(int,int)),&m_maskShapes,SLOT(moveShapeOrControlPointBy(int,int)));
  connect(drawController,SIGNAL(touchPointAt(int,int)),&m_maskShapes,SLOT(touchShapeOrControlPointAt(int,int)));
  connect(drawController,SIGNAL(disabled()),&m_maskShapes,SLOT(deselectAll()));
  connect(drawController,SIGNAL(removeSelectedShapes()),&m_maskShapes,SLOT(removeSelectedShapes()));
  connect(drawController,SIGNAL(deselectAll()),&m_maskShapes,SLOT(deselectAll()));
  connect(drawController,SIGNAL(restoreOverrideCursor()),&m_maskShapes,SLOT(restoreOverrideCursor()));
  connect(drawController,SIGNAL(setSelection(QRect)),this,SLOT(setSelectionRect(QRect)));
  connect(drawController,SIGNAL(finishSelection(QRect)),this,SLOT(selectMultipleMasks(QRect)));
  connect(drawController,SIGNAL(finishSelection(QRect)),this,SIGNAL(shapeChangeFinished()));

  // create and connect the peak eraser controller
  InputControllerErase* eraseController = new InputControllerErase(this);
  setInputController(EraseMode, eraseController);
  connect(eraseController,SIGNAL(erase(QRect)),this,SLOT(erasePeaks(QRect)));
}

ProjectionSurface::~ProjectionSurface()
{
  if (m_viewImage)
  {
    delete m_viewImage;
  }
  if (m_pickImage)
  {
    delete m_pickImage;
  }
  for(int i=0;i < m_peakShapes.size(); ++i)
  {
    if (m_peakShapes[i]) delete m_peakShapes[i];
  }
  m_peakShapes.clear();
}

/**
 * Resets the instrument actor. The caller must ensure that the instrument
 * stays the same and workspace dimensions also don't change.
 */
void ProjectionSurface::resetInstrumentActor(const InstrumentActor* rootActor)
{
  m_instrActor = rootActor;
  connect(rootActor,SIGNAL(colorMapChanged()),this,SLOT(colorMapChanged()));
}

void ProjectionSurface::clear()
{
  if (m_viewImage)
  {
    delete m_viewImage;
    m_viewImage = NULL;
  }
  if (m_pickImage)
  {
    delete m_pickImage;
    m_pickImage = NULL;
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
void ProjectionSurface::draw(MantidGLWidget *widget)const
{
  if ( m_viewChanged && ( m_redrawPicking
                          || m_interactionMode == PickSingleMode
                          || m_interactionMode == PickTubeMode
                          || m_interactionMode == DrawMode ) )
  {
    draw(widget,true);
    m_redrawPicking = false;
  }
  draw(widget,false);
  if ( m_viewChanged )
  {
    m_viewChanged = false;
  }
}

/**
 * Draw the surface on an OpenGL widget.
 * @param widget :: A widget to draw on.
 * @param picking :: Picking / normal drawing switch.
 */
void ProjectionSurface::draw(MantidGLWidget *widget,bool picking)const
{
  QImage **image = picking ? &m_pickImage : &m_viewImage;

  if (!*image || (*image)->width() != widget->width() || (*image)->height() != widget->height())
  {
    m_viewChanged = true;
  }

  if (m_viewChanged)
  {

    this->drawSurface(widget,picking);

    if (*image)
    {
      delete (*image);
    }
    (*image) = new QImage(widget->grabFrameBuffer());

    if (!picking)
    {
      QPainter painter(widget);
      RectF windowRect = getSurfaceBounds();
      m_maskShapes.setWindow(windowRect,painter.viewport());
      m_maskShapes.draw(painter);
      for(int i=0;i < m_peakShapes.size(); ++i)
      {
        m_peakShapes[i]->setWindow(windowRect,painter.viewport());
        m_peakShapes[i]->draw(painter);
      }
      painter.end();
    }
  }
  else if (!picking)
  {
    QPainter painter(widget);
    painter.drawImage(0,0,**image);

    RectF windowRect = getSurfaceBounds();
    m_maskShapes.setWindow(windowRect,painter.viewport());
    m_maskShapes.draw(painter);

    for(int i=0;i < m_peakShapes.size(); ++i)
    {
      m_peakShapes[i]->setWindow(windowRect,painter.viewport());
      m_peakShapes[i]->draw(painter);
    }

    // draw the selection rectangle
    if (!m_selectRect.isNull())
    {
      painter.setPen(Qt::blue);
      //painter.setCompositionMode(QPainter::CompositionMode_Xor);
      painter.drawRect(m_selectRect);
    }
    getController()->onPaint( painter );
    painter.end();
    // Discard any error generated here
    glGetError();
  }

}

/**
 * Draw the surface onto a normal widget without OpenGL
 * @param widget :: A widget to draw on.
 */
void ProjectionSurface::drawSimple(QWidget* widget)const
{
  if (m_viewChanged)
  {
    if ( !m_viewImage || m_viewImage->width() != widget->width() || m_viewImage->height() != widget->height())
    {
      if ( m_viewImage ) delete m_viewImage;
      m_viewImage = new QImage(widget->width(), widget->height(),QImage::Format_RGB32);
      if (m_pickImage) delete m_pickImage;
      m_pickImage = new QImage(widget->width(), widget->height(),QImage::Format_RGB32);
    }

    if ( m_redrawPicking || m_interactionMode == PickSingleMode || m_interactionMode == PickTubeMode )
    {
      drawSimpleToImage(m_pickImage,true);
      m_redrawPicking = false;
    }
    drawSimpleToImage(m_viewImage,false);
    m_viewChanged = false;
  }

  QPainter painter(widget);
  painter.drawImage(0,0,*m_viewImage);

  RectF windowRect = getSurfaceBounds();
  m_maskShapes.setWindow(windowRect,painter.viewport());
  m_maskShapes.draw(painter);

  for(int i=0;i < m_peakShapes.size(); ++i)
  {
    m_peakShapes[i]->setWindow(windowRect,painter.viewport());
    m_peakShapes[i]->draw(painter);
  }

  // draw the selection rectangle
  if (!m_selectRect.isNull())
  {
    painter.setPen(Qt::blue);
    //painter.setCompositionMode(QPainter::CompositionMode_Xor);
    painter.drawRect(m_selectRect);
  }
  getController()->onPaint( painter );
  painter.end();
}

void ProjectionSurface::resize(int, int)
{
    updateView();
}

/**
 * Draw the surface onto an image without OpenGL
 * @param image :: Image to draw on.
 * @param picking :: If true draw a picking image.
 */
void ProjectionSurface::drawSimpleToImage(QImage*,bool)const
{
}

void ProjectionSurface::mousePressEvent(QMouseEvent* e)
{
    getController()->mousePressEvent( e );
}

void ProjectionSurface::mouseMoveEvent(QMouseEvent* e)
{
    getController()->mouseMoveEvent( e );
}

void ProjectionSurface::mouseReleaseEvent(QMouseEvent* e)
{
    getController()->mouseReleaseEvent( e );
}

void ProjectionSurface::wheelEvent(QWheelEvent* e)
{
    getController()->wheelEvent( e );
}

void ProjectionSurface::keyPressEvent(QKeyEvent* e)
{
    getController()->keyPressEvent( e );
}

void ProjectionSurface::enterEvent(QEvent *e)
{
    getController()->enterEvent( e );
}

void ProjectionSurface::leaveEvent(QEvent *e)
{
    getController()->leaveEvent( e );
}

/**
  * Update the view of the surface at the next redraw.
  * @param picking :: Set to true to update the picking image regardless the interaction
  *   mode of the surface.
  */
void ProjectionSurface::updateView(bool picking)
{
  m_viewChanged = true;
  if (picking)
  {
    // don't change to false if it's already true
    m_redrawPicking = true;
  }
}

void ProjectionSurface::updateDetectors()
{
  clear();
  this->init();
  // if integration range in the instrument actor has changed
  // update visiblity of peak markers
  setPeakVisibility();
}

/// Send a redraw request to the surface owner
void ProjectionSurface::requestRedraw(bool resetPeakVisibility)
{
  if ( resetPeakVisibility )
  {
    setPeakVisibility();
  }
  emit redrawRequired();
}

QRect ProjectionSurface::selectionRect()const
{
  if (m_selectRect.width() <= 1 || m_selectRect.height() <= 1) return QRect();

  int x_min  = m_selectRect.left();
  int x_size = m_selectRect.width();
  int y_min  = m_selectRect.top();
  int y_size = m_selectRect.height();

  if (x_size < 0)
  {
    x_min += x_size;
    x_size = abs(x_size);
  }

  if (y_size < 0)
  {
    y_min += y_size;
    y_size = abs(y_size);
  }

  return QRect(x_min,y_min,x_size,y_size);
}

RectF ProjectionSurface::selectionRectUV()const
{
  double left = static_cast<double>(m_selectRect.left());
  double right = static_cast<double>(m_selectRect.right());
  double top = static_cast<double>(m_selectRect.top());
  double bottom = static_cast<double>(m_selectRect.bottom());

  if ( left > right )
  {
      std::swap( left, right );
  }

  if ( top > bottom )
  {
      std::swap( top, bottom );
  }

  if ( abs(m_selectRect.width()) <= 1 || abs(m_selectRect.height()) <= 1) return RectF();

  double sx = m_viewRect.xSpan() / m_viewImage->width();
  double sy = m_viewRect.ySpan() / m_viewImage->height();

  double x_min = left * sx + m_viewRect.x0();
  double x_max = right * sx + m_viewRect.x0();
  double y_min = (m_viewImage->height() - bottom) * sy + m_viewRect.y0();
  double y_max = (m_viewImage->height() - top) * sy + m_viewRect.y0();

  return RectF( QPointF(x_min, y_min), QPointF(x_max, y_max) );
}

bool ProjectionSurface::hasSelection()const
{
  return ! m_selectRect.isNull() && m_selectRect.width() > 0;
}

void ProjectionSurface::colorMapChanged()
{
  this->changeColorMap();
  updateView(false);
  requestRedraw();
}

/**
  * Set an interaction mode for the surface.
  * @param mode :: A new mode.
  */
void ProjectionSurface::setInteractionMode(int mode)
{
    if ( mode < 0 || mode >= m_inputControllers.size() )
    {
        throw std::logic_error("Invalid interaction mode requested.");
    }
    if ( mode == m_interactionMode ) return;
    InputController *controller = m_inputControllers[m_interactionMode];
    if ( !controller ) throw std::logic_error("Input controller doesn't exist.");
    controller->onDisabled();
    m_interactionMode = mode;
    controller = m_inputControllers[m_interactionMode];
    if ( !controller ) throw std::logic_error("Input controller doesn't exist.");
    controller->onEnabled();
    if ( mode != DrawMode )
    {
        m_maskShapes.deselectAll();
        foreach(PeakOverlay* po, m_peakShapes)
        {
            po->deselectAll();
        }
    }
}

/**
  * Return detector id at image coordinats x,y if in pick mode. -1 otherwise
  */
int ProjectionSurface::getDetectorID(int x, int y)const
{
  size_t pickID = getPickID( x, y );
  return m_instrActor->getDetID( pickID );
}

//------------------------------------------------------------------------------
boost::shared_ptr<const Mantid::Geometry::IDetector> ProjectionSurface::getDetector(int x, int y)const
{
  size_t pickID = getPickID( x, y );
  return m_instrActor->getDetector( pickID );
}

/**
  * Return info text for interactions common to all surfaces.
  */
QString ProjectionSurface::getInfoText() const
{
    switch ( m_interactionMode )
    {
    case PickSingleMode:
    case PickTubeMode:
        return "Move cursor over instrument to see detector information. ";
    case AddPeakMode:
        return "Click on a detector then click on the mini-plot to add a peak.";
    case DrawMode:
        return "Select a tool button to draw a new shape. "
                "Click on shapes to select. Click and move to edit.";
    case EraseMode:
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
Mantid::Kernel::V3D ProjectionSurface::getDetectorPos(int x, int y) const
{
  size_t pickID = getPickID( x, y );
  return m_instrActor->getDetPos( pickID );
}

/**
  * Is context menu allowed?
  */
bool ProjectionSurface::canShowContextMenu() const
{
    InputController *controller = m_inputControllers[m_interactionMode];
    if ( controller )
    {
        return controller->canShowContextMenu();
    }
    return false;
}

//------------------------------------------------------------------------------
size_t ProjectionSurface::getPickID(int x, int y)const
{
    if (!m_pickImage || !m_pickImage->valid(x,y)) return -1;
    QRgb pixel = m_pickImage->pixel(x,y);
    return GLActor::decodePickColor(pixel);
}

/**
  * Adds an input controller to the controller list.
  * @param mode :: The interaction mode which is the index of the controller in the list.
  * @param controller :: A pointer to the controller to be set.
  */
void ProjectionSurface::setInputController(int mode, InputController *controller)
{
    m_inputControllers[mode] = controller;
}

/**
  * Set visibility of the peak markers according to the integration range
  * in the instrument actor.
  */
void ProjectionSurface::setPeakVisibility() const
{
    if ( hasPeakOverlays() )
    {
        Mantid::Kernel::Unit_sptr unit = m_instrActor->getWorkspace()->getAxis(0)->unit();
        QString unitID = QString::fromStdString(unit->unitID());
        double xmin = m_instrActor->minBinValue();
        double xmax = m_instrActor->maxBinValue();
        foreach(PeakOverlay* po, m_peakShapes)
        {
            po->setPeakVisibility(xmin,xmax,unitID);
        }
    }
}

/**
  * Returns the current controller. If the controller doesn't exist throws a logic_error exceotion.
  */
InputController *ProjectionSurface::getController() const
{
    InputController* controller = m_inputControllers[m_interactionMode];
    if ( !controller )
    {
        throw std::logic_error("Input controller doesn't exist for current interaction mode.");
    }
    return controller;
}

  // --- Shape2D manipulation --- //

void ProjectionSurface::startCreatingShape2D(const QString& type,const QColor& borderColor,const QColor& fillColor)
{
  emit signalToStartCreatingShape2D(type,borderColor,fillColor);
}

/**
 * Return a combined list of peak parkers from all overlays
 * @param detID :: The detector ID of interest
 */
QList<PeakMarker2D*> ProjectionSurface::getMarkersWithID(int detID)const
{
  QList<PeakMarker2D*> out;
  for(int i=0;i < m_peakShapes.size(); ++i)
  {
    out += m_peakShapes[i]->getMarkersWithID(detID);
  }
  return out;
}

/**
  * Get peaks workspace for manually editing.
  */
boost::shared_ptr<Mantid::API::IPeaksWorkspace> ProjectionSurface::getEditPeaksWorkspace() const
{
    if ( !m_peakShapes.isEmpty() )
    {
        return m_peakShapes.last()->getPeaksWorkspace();
    }
    return boost::shared_ptr<Mantid::API::IPeaksWorkspace>();
}

/**
 * Remove an overlay if its peaks workspace is deleted.
 * @param ws :: Shared pointer to the deleted peaks workspace.
 */
void ProjectionSurface::deletePeaksWorkspace(boost::shared_ptr<Mantid::API::IPeaksWorkspace> ws)
{
  const int npeaks = m_peakShapes.size();
  for(int i=0;i < npeaks; ++i)
  {
    if (m_peakShapes[i]->getPeaksWorkspace() == ws)
    {
      delete m_peakShapes[i];
      m_peakShapes.removeAt(i);
      break;
    }
  }
  if ( m_peakShapes.size() < npeaks )
  {
      emit peaksWorkspaceDeleted();
  }
}

/**
 * Remove all peak overlays.
 */
void ProjectionSurface::clearPeakOverlays()
{
    if ( !m_peakShapes.isEmpty() )
    {
        for(int i=0;i < m_peakShapes.size(); ++i)
        {
          delete m_peakShapes[i];
        }
        m_peakShapes.clear();
        m_peakShapesStyle = 0;
        emit peaksWorkspaceDeleted();
    }
}

/**
 * Set the precision (significant digits) with which the HKL peak labels are displayed.
 * @param n :: Precision, > 0
 */
void ProjectionSurface::setPeakLabelPrecision(int n)
{
  if (n < 1)
  {
    QMessageBox::critical(NULL,"MantidPlot - Error","Precision must be a positive number");
    return;
  }
  m_peakLabelPrecision = n;
  for(int i=0;i < m_peakShapes.size(); ++i)
  {
    m_peakShapes[i]->setPrecision(n);
  }
}

/**
 * Enable or disable the show peak row flag
 */
void ProjectionSurface::setShowPeakRowsFlag(bool on)
{
  m_showPeakRows = on;
  for(int i=0;i < m_peakShapes.size(); ++i)
  {
    m_peakShapes[i]->setShowRowsFlag(on);
  }
}

/**
 * Enable or disable the show peak label flag
 */
void ProjectionSurface::setShowPeakLabelsFlag(bool on)
{
    m_showPeakLabels = on;
    for(int i=0;i < m_peakShapes.size(); ++i)
    {
      m_peakShapes[i]->setShowLabelsFlag(on);
    }
}

/**
  * Set the selection rect in screen corrds.
  * @param rect :: New selection rectangle.
  */
void ProjectionSurface::setSelectionRect(const QRect &rect)
{
    if ( m_interactionMode != DrawMode || !m_maskShapes.hasSelection() )
    {
        m_selectRect = rect;
    }
}

/**
  * Delete selection rectanle.
  */
void ProjectionSurface::emptySelectionRect()
{
    m_selectRect = QRect();
}

/**
 * Select multiple mask shapes as a result of a rubber-band selection
 * @param rect :: The rubber band rect.
 */
void ProjectionSurface::selectMultipleMasks(const QRect &rect)
{
    if ( !m_maskShapes.hasSelection() )
    {
        m_maskShapes.selectIn( rect );
    }
    emptySelectionRect();
}

/**
  * Pick a detector at a pointe on the screen.
  */
void ProjectionSurface::pickComponentAt(int x, int y)
{
    size_t pickID = getPickID( x, y );
    emit singleComponentPicked(pickID);
}

void ProjectionSurface::touchComponentAt(int x, int y)
{
    size_t pickID = getPickID( x, y );
    emit singleComponentTouched(pickID);
}

void ProjectionSurface::erasePeaks(const QRect &rect)
{
    foreach(PeakOverlay* po, m_peakShapes)
    {
        po->selectIn( rect );
        po->removeSelectedShapes();
    }

}

/**
 * Enable or disable lighting in non-picking mode
 * @param on :: True for enabling, false for disabling.
 */
void ProjectionSurface::enableLighting(bool on)
{
    m_isLightingOn = on;
}

/**
  * Return names of attached peaks workspaces.
  */
QStringList ProjectionSurface::getPeaksWorkspaceNames() const
{
    QStringList names;
    foreach(PeakOverlay* po, m_peakShapes)
    {
        names << QString::fromStdString(po->getPeaksWorkspace()->name());
    }
    return names;
}

