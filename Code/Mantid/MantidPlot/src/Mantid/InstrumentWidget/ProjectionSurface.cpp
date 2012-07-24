#include "ProjectionSurface.h"
#include "GLColor.h"
#include "MantidGLWidget.h"
#include "OpenGLError.h"

#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Objects/Object.h"

#include <QRgb>
#include <QSet>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QMessageBox>

#include <cfloat>
#include <limits>
#include <cmath>
#include "MantidKernel/V3D.h"

using Mantid::Kernel::V3D;

/**
  * The constructor.
  * @param rootActor :: The instrument actor containning all info about the instrument
  * @param origin :: Defines the origin of the projection reference system (if applicable)
  * @param axis :: 
  */
ProjectionSurface::ProjectionSurface(const InstrumentActor* rootActor,const Mantid::Kernel::V3D& origin,const Mantid::Kernel::V3D& axis):
    m_instrActor(rootActor),
    m_pos(origin),
    m_zaxis(axis),
    m_viewImage(NULL),
    m_pickImage(NULL),
    m_viewChanged(true),
    m_viewRect(),
    m_selectRect(),
    m_interactionMode(MoveMode),
    m_leftButtonDown(false),
    m_peakLabelPrecision(6)
{
  connect(rootActor,SIGNAL(colorMapChanged()),this,SLOT(colorMapChanged()));
  connect(&m_maskShapes,SIGNAL(shapeCreated()),this,SLOT(catchShapeCreated()));
  connect(&m_maskShapes,SIGNAL(shapeSelected()),this,SLOT(catchShapeSelected()));
  connect(&m_maskShapes,SIGNAL(shapesDeselected()),this,SLOT(catchShapesDeselected()));
  connect(&m_maskShapes,SIGNAL(shapeChanged()),this,SLOT(catchShapeChanged()));
}

ProjectionSurface::~ProjectionSurface()
{
  //std::cerr<<"ProjectionSurface deleted\n";
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
  m_viewRect = QRectF();
  m_selectRect = QRect();
  m_zoomStack.clear();
}

void ProjectionSurface::draw(MantidGLWidget *widget)const
{
  if (getInteractionMode() == MoveMode)
  {
    draw(widget,false);
    if (m_pickImage)
    {
      delete m_pickImage;
      m_pickImage = NULL;
    }
  }
  else
  {
    //std::cerr << "picking\n";
    bool changed = m_viewChanged;
    draw(widget,true);
    m_viewChanged = changed;
    draw(widget,false);
  }
}

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
      QRectF windowRect = getSurfaceBounds();
      m_maskShapes.setWindow(windowRect,painter.viewport());
      m_maskShapes.draw(painter);
      for(int i=0;i < m_peakShapes.size(); ++i)
      {
        m_peakShapes[i]->setWindow(windowRect,painter.viewport());
        m_peakShapes[i]->draw(painter);
      }
      painter.end();
    }
    m_viewChanged = false;

  }
  else if (!picking)
  {
    QPainter painter(widget);
    painter.drawImage(0,0,**image);

    QRectF windowRect = getSurfaceBounds();
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
    }

    if (getInteractionMode() == MoveMode)
    {
      drawSimpleToImage(m_viewImage,false);
      if (m_pickImage)
      {
        delete m_pickImage;
        m_pickImage = NULL;
      }
    }
    else
    {
      if (m_pickImage) delete m_pickImage;
      m_pickImage = new QImage(widget->width(), widget->height(),QImage::Format_RGB32);
      drawSimpleToImage(m_pickImage,true);
      drawSimpleToImage(m_viewImage,false);
    }
  }

  QPainter painter(widget);
  painter.drawImage(0,0,*m_viewImage);

  QRectF windowRect = getSurfaceBounds();
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
  painter.end();
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
  switch(m_interactionMode)
  {
  case MoveMode: this->mousePressEventMove(e); break;
  case PickMode: this->mousePressEventPick(e); break;
  case DrawMode: this->mousePressEventDraw(e); break;
  }
}

void ProjectionSurface::mouseMoveEvent(QMouseEvent* e)
{
  switch(m_interactionMode)
  {
  case MoveMode: this->mouseMoveEventMove(e); break;
  case PickMode: this->mouseMoveEventPick(e); break;
  case DrawMode: this->mouseMoveEventDraw(e); break;
  }
}

void ProjectionSurface::mouseReleaseEvent(QMouseEvent* e)
{
  switch(m_interactionMode)
  {
  case MoveMode: this->mouseReleaseEventMove(e); break;
  case PickMode: this->mouseReleaseEventPick(e); break;
  case DrawMode: this->mouseReleaseEventDraw(e); break;
  }
}

void ProjectionSurface::wheelEvent(QWheelEvent* e)
{
  switch(m_interactionMode)
  {
  case MoveMode: this->wheelEventMove(e); break;
  case PickMode: this->wheelEventPick(e); break;
  case DrawMode: this->wheelEventDraw(e); break;
  }
}

void ProjectionSurface::keyPressEvent(QKeyEvent* e)
{
  switch(m_interactionMode)
  {
  case MoveMode: break;
  case PickMode: break;
  case DrawMode: this->keyPressEventDraw(e); break;
  }
}

void ProjectionSurface::mousePressEventPick(QMouseEvent* e)
{
  if (e->button() == Qt::LeftButton)
  {
    m_leftButtonDown = true;
    startSelection(e->x(),e->y());
    int id = getDetectorID(e->x(),e->y());
    emit singleDetectorPicked(id);
  }
}

void ProjectionSurface::mouseMoveEventPick(QMouseEvent* e)
{
  if (m_leftButtonDown)
  {
    moveSelection(e->x(),e->y());
  }
  else
  {
    int id = getDetectorID(e->x(),e->y());
    emit singleDetectorTouched(id);
  }
}

void ProjectionSurface::mouseReleaseEventPick(QMouseEvent* e)
{
  if (m_leftButtonDown)
  {
    QList<int> detList;
    getSelectedDetectors(detList);
    emit multipleDetectorsSelected(detList);
    endSelection(e->x(),e->y());
  }
  m_leftButtonDown = false;
}

void ProjectionSurface::wheelEventPick(QWheelEvent*)
{
}

void ProjectionSurface::mousePressEventDraw(QMouseEvent* e)
{
  m_maskShapes.mousePressEvent(e);
}

void ProjectionSurface::mouseMoveEventDraw(QMouseEvent* e)
{
  m_maskShapes.mouseMoveEvent(e);
}

void ProjectionSurface::mouseReleaseEventDraw(QMouseEvent* e)
{
  m_maskShapes.mouseReleaseEvent(e);
}

void ProjectionSurface::wheelEventDraw(QWheelEvent* e)
{
  m_maskShapes.wheelEvent(e);
}

void ProjectionSurface::keyPressEventDraw(QKeyEvent* e)
{
  m_maskShapes.keyPressEvent(e);
}


void ProjectionSurface::startSelection(int x,int y)
{
  m_selectRect.setRect(x,y,1,1);
}

void ProjectionSurface::moveSelection(int x,int y)
{
  m_selectRect.setBottomRight(QPoint(x,y));
}

void ProjectionSurface::endSelection(int x,int y)
{
  (void) x; //avoid compiler warning
  (void) y; //avoid compiler warning
  m_selectRect = QRect();
}

void ProjectionSurface::zoom()
{
  if (!m_viewImage) return;
  QRectF newView = selectionRectUV();
  if (newView.isNull()) return;
  m_zoomStack.push(m_viewRect);
  m_viewRect = newView;
  m_viewChanged = true;
}

/**
  * Zooms to the specified area. The previous zoom stack is cleared.
  */
void ProjectionSurface::zoom(const QRectF& area)
{
  if (!m_zoomStack.isEmpty())
  {
    m_viewRect = m_zoomStack.first();
    m_zoomStack.clear();
  }
  m_zoomStack.push(m_viewRect);

  double left = area.left();
  double top  = area.top();
  double width = area.width();
  double height = area.height();

  if (width * m_viewRect.width() < 0)
  {
    left += width;
    width = -width;
  }
  if (height * m_viewRect.height() < 0)
  {
    top += height;
    height = -height;
  }
//  std::cerr<<"New area:\n";
//  std::cerr<<left<<','<<top<<' '<<width<<','<<height<<'\n'<<'\n';
  m_viewRect = QRectF(left,top,width,height);
  m_viewChanged = true;
}

void ProjectionSurface::unzoom()
{
  if (!m_zoomStack.isEmpty())
  {
    m_viewRect = m_zoomStack.pop();
    m_viewChanged = true;
  }
}

void ProjectionSurface::updateView()
{
  m_viewChanged = true;
}

void ProjectionSurface::updateDetectors()
{
  clear();
  this->init();
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

QRectF ProjectionSurface::selectionRectUV()const
{
  if (m_selectRect.width() <= 1 || m_selectRect.height() <= 1) return QRectF();

  double x_min  = double(m_selectRect.left())/m_viewImage->width();
  double x_size = double(m_selectRect.width())/m_viewImage->width();
  double y_min  = double(m_selectRect.top())/m_viewImage->height();
  double y_size = double(m_selectRect.height())/m_viewImage->height();

  if (x_size < 0)
  {
    x_min += x_size;
    x_size = fabs(x_size);
  }

  if (y_size < 0)
  {
    y_min += y_size;
    y_size = fabs(y_size);
  }

  x_min = m_viewRect.left() + x_min * m_viewRect.width();
  x_size = x_size * m_viewRect.width();
  y_min = m_viewRect.top() + y_min * m_viewRect.height();
  y_size = y_size * m_viewRect.height();

  return QRectF(x_min,y_min,x_size,y_size);
}

bool ProjectionSurface::hasSelection()const
{
  return ! m_selectRect.isNull() && m_selectRect.width() > 1;
}

void ProjectionSurface::colorMapChanged()
{
  this->changeColorMap();
  updateView();
}

void ProjectionSurface::setInteractionModePick()
{
  m_interactionMode = PickMode;
  m_maskShapes.deselectAll();
}

void ProjectionSurface::setInteractionModeMove()
{
  m_interactionMode = MoveMode;
  m_maskShapes.deselectAll();
}

void ProjectionSurface::setInteractionModeDraw()
{
  m_interactionMode = DrawMode;
}

/**
  * Return detector id at image coordinats x,y if in pick mode. -1 otherwise
  */
int ProjectionSurface::getDetectorID(int x, int y)const
{
  if (!m_pickImage) return -7;
  if (!m_pickImage->valid(x,y)) return -1;
  QRgb pixel = m_pickImage->pixel(x,y);
  return getDetectorID((unsigned char)qRed(pixel),(unsigned char)qGreen(pixel),(unsigned char)qBlue(pixel));
}

//------------------------------------------------------------------------------
boost::shared_ptr<const Mantid::Geometry::IDetector> ProjectionSurface::getDetector(int x, int y)const
{
  if (!m_pickImage || !m_pickImage->valid(x,y)) return boost::shared_ptr<const Mantid::Geometry::IDetector>();
  QRgb pixel = m_pickImage->pixel(x,y);
  int index = getDetectorIndex((unsigned char)qRed(pixel),(unsigned char)qGreen(pixel),(unsigned char)qBlue(pixel));
  if ( index >= 0 ) return m_instrActor->getDetector(index);
  return boost::shared_ptr<const Mantid::Geometry::IDetector>();
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
  if (!m_pickImage || !m_pickImage->valid(x,y)) return V3D();
  QRgb pixel = m_pickImage->pixel(x,y);
  int index = getDetectorIndex((unsigned char)qRed(pixel),(unsigned char)qGreen(pixel),(unsigned char)qBlue(pixel));
  if ( index >= 0 )
    return m_instrActor->getDetPos(index);
  else
    return V3D();
}

//------------------------------------------------------------------------------
int ProjectionSurface::getDetectorIndex(unsigned char r,unsigned char g,unsigned char b)const
{
  size_t index = GLActor::decodePickColor(r,g,b);
  if (index > m_instrActor->ndetectors())
  {
    return -1;
  }
  else
  {
    return int(index);
  }
}

int ProjectionSurface::getDetectorID(unsigned char r,unsigned char g,unsigned char b)const
{
  int index = getDetectorIndex(r,g,b);
  if (index < 0) return index;
  return m_instrActor->getDetID(index);
}

QString ProjectionSurface::getPickInfoText()const
{
  return "Move cursor over instrument to see detector information.\n"
    "Left click and drag to select multiple detectors.";
}

  // --- Shape2D manipulation --- //

void ProjectionSurface::startCreatingShape2D(const QString& type,const QColor& borderColor,const QColor& fillColor)
{
  m_maskShapes.startCreatingShape2D(type,borderColor,fillColor);
}

void ProjectionSurface::catchShapeCreated()
{
  emit shapeCreated();
}

void ProjectionSurface::catchShapeSelected()
{
  emit shapeSelected();
}

void ProjectionSurface::catchShapesDeselected()
{
  emit shapesDeselected();
}

void ProjectionSurface::catchShapeChanged()
{
  emit shapeChanged();
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
 * Remove an overlay if its peaks workspace is deleted.
 * @param ws :: Shared pointer to the deleted peaks workspace.
 */
void ProjectionSurface::peaksWorkspaceDeleted(boost::shared_ptr<Mantid::API::IPeaksWorkspace> ws)
{
  for(int i=0;i < m_peakShapes.size(); ++i)
  {
    if (m_peakShapes[i]->getPeaksWorkspace() == ws)
    {
      delete m_peakShapes[i];
      m_peakShapes.removeAt(i);
      break;
    }
  }
}

/**
 * Remove all peak overlays.
 */
void ProjectionSurface::clearPeakOverlays()
{
  for(int i=0;i < m_peakShapes.size(); ++i)
  {
      delete m_peakShapes[i];
  }
  m_peakShapes.clear();
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
void ProjectionSurface::setShowPeakRowFlag(bool on)
{
  m_showPeakRow = on;
  for(int i=0;i < m_peakShapes.size(); ++i)
  {
    m_peakShapes[i]->setShowRowsFlag(on);
  }
}
