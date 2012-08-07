#ifndef PROJECTIONSURFACE_H
#define PROJECTIONSURFACE_H

#include "MantidKernel/V3D.h"
#include "MantidKernel/Quat.h"
#include "MantidGeometry/IComponent.h"

#include "InstrumentActor.h"
#include "Shape2DCollection.h"
#include "PeakOverlay.h"

#include <QImage>
#include <QList>
#include <QStack>
#include <QColor>

namespace Mantid{
  namespace Geometry{
    class IDetector;
  }
  namespace API{
    class IPeaksWorkspace;
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
  enum InteractionMode {MoveMode = 0, PickMode = 1, DrawMode}; ///< Move around or select things
  /// Constructor
  ProjectionSurface(const InstrumentActor* rootActor,const Mantid::Kernel::V3D& origin,const Mantid::Kernel::V3D& axis);
  /// Destructor
  virtual ~ProjectionSurface();
  /// Resets the instrument actor.
  void resetInstrumentActor(const InstrumentActor* rootActor);

  //-----------------------------------
  //     Public virtual methods
  //-----------------------------------

  /// draw the surface onto a GL widget
  virtual void draw(MantidGLWidget* widget)const;
  /// draw the surface onto a normal widget
  virtual void drawSimple(QWidget* widget)const;
  /// called when the gl widget gets resized
  virtual void resize(int, int){}
  /// redraw surface without recalulationg of colours, etc
  virtual void updateView();
  /// full update and redraw of the surface
  virtual void updateDetectors();
  /// returns the bounding rectangle in the real coordinates
  virtual QRectF getSurfaceBounds()const{return m_viewRect;} 

  virtual void mousePressEvent(QMouseEvent*);
  virtual void mouseMoveEvent(QMouseEvent*);
  virtual void mouseReleaseEvent(QMouseEvent*);
  virtual void wheelEvent(QWheelEvent *);
  virtual void keyPressEvent(QKeyEvent*);

  /// start selection at a point on the screen
  virtual void startSelection(int x,int y);
  /// expand selection upto a point on the screen
  virtual void moveSelection(int x,int y);
  /// end selection at a point on the screen
  virtual void endSelection(int x,int y);
  /// return true if any of the detectors have been selected
  virtual bool hasSelection()const;

  virtual int getDetectorID(int x, int y)const;
  virtual boost::shared_ptr<const Mantid::Geometry::IDetector> getDetector(int x, int y)const;
  /// NULL deselects components and selects the whole instrument
  virtual void componentSelected(Mantid::Geometry::ComponentID = NULL) = 0;
  /// fill in a list of detector ids which were selected by the selction tool
  virtual void getSelectedDetectors(QList<int>& dets) = 0;
  /// fill in a list of detector ids which were masked by the mask shapes
  virtual void getMaskedDetectors(QList<int>& dets)const = 0;

  virtual QString getInfoText()const{return "";}

  /// Zoom into an area of the screen
  virtual void zoom(const QRectF& area);
  virtual void zoom();
  /// Unzoom view to the previous zoom area or to full view
  virtual void unzoom();
  //-----------------------------------

  Mantid::Kernel::V3D getDetectorPos(int x, int y) const;
  /// Change the interaction mode
  void setInteractionModeMove();
  void setInteractionModePick();
  void setInteractionModeDraw();
  InteractionMode getInteractionMode()const{return m_interactionMode;}

  /// Set background colour
  void setBackgroundColor(const QColor& color) {m_backgroundColor = color;}
  /// Get background colour
  QColor getBackgroundColor() const {return m_backgroundColor;}

  //-----------------------------------
  //    Shape2D manipulation
  //-----------------------------------

  QRectF getCurrentBoundingRect()const{return m_maskShapes.getCurrentBoundingRect();}
  void setCurrentBoundingRect(const QRectF& rect){m_maskShapes.setCurrentBoundingRect(rect);}

  void startCreatingShape2D(const QString& type,const QColor& borderColor,const QColor& fillColor = QColor());
  // double properties
  QStringList getCurrentDoubleNames()const{return m_maskShapes.getCurrentDoubleNames();}
  double getCurrentDouble(const QString& prop) const{return m_maskShapes.getCurrentDouble(prop);}
  void setCurrentDouble(const QString& prop, double value){m_maskShapes.setCurrentDouble(prop, value);}
  // QPointF properties
  QStringList getCurrentPointNames()const{return m_maskShapes.getCurrentPointNames();}
  QPointF getCurrentPoint(const QString& prop) const{return m_maskShapes.getCurrentPoint(prop);}
  void setCurrentPoint(const QString& prop, const QPointF& value){m_maskShapes.setCurrentPoint(prop,value);}

  bool isMasked(double x,double y)const{return m_maskShapes.isMasked(x,y);}
  void clearMask(){m_maskShapes.clear();}

  QList<PeakMarker2D*> getMarkersWithID(int detID)const;
  //PeakOverlay& getPeakOverlay(){return m_peakShapes;}
  void peaksWorkspaceDeleted(boost::shared_ptr<Mantid::API::IPeaksWorkspace> ws);
  void clearPeakOverlays();
  bool hasPeakOverlays() const {return !m_peakShapes.isEmpty();}
  void setPeakLabelPrecision(int n);
  int getPeakLabelPrecision() const {return m_peakLabelPrecision;}
  void setShowPeakRowFlag(bool on);
  bool getShowPeakRowFlag()const {return m_showPeakRow;}

signals:

  void singleDetectorTouched(int);
  void singleDetectorPicked(int);
  void multipleDetectorsSelected(QList<int>&);

  void shapeCreated();
  void shapeSelected();
  void shapesDeselected();
  void shapeChanged();

protected slots:

  void colorMapChanged();
  void catchShapeCreated();
  void catchShapeSelected();
  void catchShapesDeselected();
  void catchShapeChanged();

protected:
  
  //-----------------------------------
  //     Protected virtual methods
  //-----------------------------------

  virtual void init() = 0;
  /// Draw the surface onto an OpenGL widget
  virtual void drawSurface(MantidGLWidget* widget,bool picking = false)const = 0;
  /// Respond to a change of color map in m_instrActor
  virtual void changeColorMap() = 0;
  /// Draw the surface onto an image without OpenGL
  virtual void drawSimpleToImage(QImage* image,bool picking = false)const;

  virtual void mousePressEventMove(QMouseEvent*){}
  virtual void mouseMoveEventMove(QMouseEvent*){}
  virtual void mouseReleaseEventMove(QMouseEvent*){}
  virtual void wheelEventMove(QWheelEvent*){}

  virtual void mousePressEventPick(QMouseEvent*);
  virtual void mouseMoveEventPick(QMouseEvent*);
  virtual void mouseReleaseEventPick(QMouseEvent*);
  virtual void wheelEventPick(QWheelEvent*);

  virtual void mousePressEventDraw(QMouseEvent*);
  virtual void mouseMoveEventDraw(QMouseEvent*);
  virtual void mouseReleaseEventDraw(QMouseEvent*);
  virtual void wheelEventDraw(QWheelEvent*);
  virtual void keyPressEventDraw(QKeyEvent*);
  //-----------------------------------

  void draw(MantidGLWidget* widget,bool picking)const;
  void clear();
  QRect selectionRect()const;
  QRectF selectionRectUV()const;
  int getDetectorIndex(unsigned char r,unsigned char g,unsigned char b)const;
  int getDetectorID(unsigned char r,unsigned char g,unsigned char b)const;
  QString getPickInfoText()const;

  //-----------------------------------
  //     Protected data
  //-----------------------------------

  const InstrumentActor* m_instrActor;
  const Mantid::Kernel::V3D m_pos;   ///< Origin (sample position)
  const Mantid::Kernel::V3D m_zaxis; ///< The z axis of the surface specific coord system
  Mantid::Kernel::V3D m_xaxis;       ///< The x axis
  Mantid::Kernel::V3D m_yaxis;       ///< The y axis
  mutable QImage* m_viewImage;       ///< storage for view image
  mutable QImage* m_pickImage;       ///< storage for picking image
  mutable bool m_viewChanged;        ///< set when the image must be redrawn
  QColor m_backgroundColor;          ///< The background colour
  QRectF m_viewRect;                 ///< Keeps the physical dimensions of the surface
  QRect m_selectRect;
  QStack<QRectF> m_zoomStack;
  InteractionMode m_interactionMode;
  bool m_leftButtonDown;

  Shape2DCollection m_maskShapes;    ///< to draw mask shapes
  mutable QList<PeakOverlay*> m_peakShapes; ///< to draw peak labels
  mutable int m_peakLabelPrecision;
  mutable bool m_showPeakRow;        ///< flag to show peak row index

};

#endif // PROJECTIONSURFACE_H
