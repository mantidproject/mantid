#ifndef PROJECTIONSURFACE_H
#define PROJECTIONSURFACE_H

#include "MantidKernel/V3D.h"
#include "MantidKernel/Quat.h"
#include "MantidGeometry/IComponent.h"
#include "InstrumentActor.h"
#include <boost/shared_ptr.hpp>

#include <QImage>
#include <QList>
#include <QStack>
#include <QSet>
#include <QMap>

namespace Mantid{
  namespace Geometry{
    class IDetector;
  }
}

class GLColor;
class MantidGLWidget;

class QMouseEvent;
class QWheelEvent;

/**
  * @class ProjectionSurface
  * @brief Performs projection of an instrument onto a plane.
  * @author Roman Tolchenov, Tessella plc
  * @date 13 May 2011

  * Performs projection of an instrument onto a plane. Draws the resulting image on the screen.
  * Supports selection and zooming.
  */

class ProjectionSurface: public QObject
{
  Q_OBJECT
public:
  enum InteractionMode {MoveMode = 0, PickMode = 1}; ///< Move around or select things

  ProjectionSurface(const InstrumentActor* rootActor,const Mantid::Kernel::V3D& origin,const Mantid::Kernel::V3D& axis);
  virtual ~ProjectionSurface();
  /// draw the surface onto a GL widget
  virtual void draw(MantidGLWidget* widget)const;
  /// called when the gl widget gets resized
  virtual void resize(int, int){}
  /// redraw surface without recalulationg of colours, etc
  virtual void updateView();
  /// full update and redraw of the surface
  virtual void updateDetectors();

  virtual void mousePressEvent(QMouseEvent* event);
  virtual void mouseMoveEvent(QMouseEvent*);
  virtual void mouseReleaseEvent(QMouseEvent*);
  virtual void wheelEvent(QWheelEvent *);

  void setInteractionModeMove();
  void setInteractionModePick();
  InteractionMode getInteractionMode()const{return m_interactionMode;}

  /// start selection at a point on the screen
  virtual void startSelection(int x,int y);
  /// expand selection upto a point on the screen
  virtual void moveSelection(int x,int y);
  /// end selection at a point on the screen
  virtual void endSelection(int x,int y);
  /// return true if any of the detectors have been selected
  virtual bool hasSelection()const;

  virtual int getDetectorID(int x, int y);
  /// NULL deselects components and selects the whole instrument
  virtual void componentSelected(Mantid::Geometry::ComponentID = NULL) = 0;
  virtual void getSelectedDetectors(QList<int>& dets) = 0;

  virtual QString getInfoText()const{return "";}

  /// Zoom into an area of the screen
  virtual void zoom(const QRectF& area);
  virtual void zoom();
  /// Unzoom view to the previous zoom area or to full view
  virtual void unzoom();

signals:

  void singleDetectorTouched(int);
  void singleDetectorPicked(int);
  void multipleDetectorsSelected(QList<int>&);

protected slots:

  void colorMapChanged();

protected:
  virtual void init() = 0;
  virtual void drawSurface(MantidGLWidget* widget,bool picking = false)const = 0;
  /// Respond to a change of color map in m_instrActor
  virtual void changeColorMap() = 0;
  virtual void mousePressEventMove(QMouseEvent*){}
  virtual void mouseMoveEventMove(QMouseEvent*){}
  virtual void mouseReleaseEventMove(QMouseEvent*){}
  virtual void wheelEventMove(QWheelEvent*){}

  virtual void mousePressEventPick(QMouseEvent*);
  virtual void mouseMoveEventPick(QMouseEvent*);
  virtual void mouseReleaseEventPick(QMouseEvent*);
  virtual void wheelEventPick(QWheelEvent*);

  void draw(MantidGLWidget* widget,bool picking)const;
  void clear();
  QRect selectionRect()const;
  QRectF selectionRectUV()const;
  int getDetectorIndex(unsigned char r,unsigned char g,unsigned char b)const;
  int getDetectorID(unsigned char r,unsigned char g,unsigned char b)const;
  QString getPickInfoText()const;

  const InstrumentActor* m_instrActor;
  const Mantid::Kernel::V3D m_pos;   ///< Origin (sample position)
  const Mantid::Kernel::V3D m_zaxis; ///< The z axis of the surface specific coord system
  Mantid::Kernel::V3D m_xaxis;       ///< The x axis
  Mantid::Kernel::V3D m_yaxis;       ///< The y axis
  mutable QImage* m_viewImage;      ///< storage for view image
  mutable QImage* m_pickImage;      ///< storage for picking image
  mutable bool m_viewChanged;       ///< set when the image must be redrawn
  QRectF m_viewRect;
  QRect m_selectRect;
  QStack<QRectF> m_zoomStack;
  InteractionMode m_interactionMode;
  bool m_leftButtonDown;
};

#endif // PROJECTIONSURFACE_H
