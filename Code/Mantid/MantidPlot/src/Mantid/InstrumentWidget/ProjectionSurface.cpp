#include "ProjectionSurface.h"
#include "GLColor.h"
#include "MantidGLWidget.h"
#include "OpenGLError.h"

#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/IInstrument.h"

#include <QRgb>
#include <QSet>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>

#include <cfloat>
#include <limits>
#include <cmath>

/**
  * The constructor.
  * @param rootActor :: The instrument actor containning all info about the instrument
  * @param origin :: Defines the origin of the projection reference system (if applicable)
  * @param axis :: 
  */
ProjectionSurface::ProjectionSurface(const InstrumentActor* rootActor,const Mantid::Geometry::V3D& origin,const Mantid::Geometry::V3D& axis):
    m_instrActor(rootActor),
    m_pos(origin),
    m_zaxis(axis),
    m_viewImage(NULL),
    m_pickImage(NULL),
    m_viewChanged(true),
    m_viewRect(),
    m_selectRect(),
    m_interactionMode(MoveMode),
    m_leftButtonDown(false)
{
  connect(rootActor,SIGNAL(colorMapChanged()),this,SLOT(colorMapChanged()));
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
      widget->swapBuffers();
    }
    m_viewChanged = false;
  }
  else if (!picking)
  {
    QPainter painter(widget);
    painter.drawImage(0,0,**image);
    // draw the selection rectangle
    if (!m_selectRect.isNull())
    {
      painter.setPen(Qt::blue);
      //painter.setCompositionMode(QPainter::CompositionMode_Xor);
      painter.drawRect(m_selectRect);
    }
    painter.end();
  }
}

void ProjectionSurface::mousePressEvent(QMouseEvent* e)
{
  if (m_interactionMode == MoveMode)
  {
    this->mousePressEventMove(e);
  }
  else
  {
    this->mousePressEventPick(e);
  }
}

void ProjectionSurface::mouseMoveEvent(QMouseEvent* e)
{
  if (m_interactionMode == MoveMode)
  {
    this->mouseMoveEventMove(e);
  }
  else
  {
    this->mouseMoveEventPick(e);
  }
}

void ProjectionSurface::mouseReleaseEvent(QMouseEvent* e)
{
  if (m_interactionMode == MoveMode)
  {
    this->mouseReleaseEventMove(e);
  }
  else
  {
    this->mouseReleaseEventPick(e);
  }
}

void ProjectionSurface::wheelEvent(QWheelEvent* e)
{
  if (m_interactionMode == MoveMode)
  {
    this->wheelEventMove(e);
  }
  else
  {
    this->wheelEventPick(e);
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
}

void ProjectionSurface::setInteractionModeMove()
{
  m_interactionMode = MoveMode;
}

/**
  * Return detector id at image coordinats x,y if in pick mode. -1 otherwise
  */
int ProjectionSurface::getDetectorID(int x, int y)
{
  if (!m_pickImage) return -7;
  if (!m_pickImage->valid(x,y)) return -1;
  QRgb pixel = m_pickImage->pixel(x,y);
  return getDetectorID((unsigned char)qRed(pixel),(unsigned char)qGreen(pixel),(unsigned char)qBlue(pixel));
}

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
