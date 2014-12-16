#ifndef PROJECTIONSURFACE_H
#define PROJECTIONSURFACE_H

#include "MantidKernel/V3D.h"
#include "MantidKernel/Quat.h"
#include "MantidGeometry/IComponent.h"

#include "InstrumentActor.h"
#include "Shape2DCollection.h"
#include "PeakOverlay.h"
#include "RectF.h"
#include "../MantidAlgorithmMetatype.h"

#include <QImage>
#include <QList>
#include <QMap>
#include <QStack>
#include <QColor>

#include <boost/shared_ptr.hpp>

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
class InputController;

class QMouseEvent;
class QWheelEvent;

/**
  * @class ProjectionSurface
  * @brief Performs projection of an instrument onto a plane.
  * @author Roman Tolchenov, Tessella plc
  * @date 13 May 2011

  * Performs projection of an instrument onto a plane. Draws the resulting image on the screen.
  * Supports selection and zooming.
  *
  * Iherited classes must implement the pure virtual methods and set m_viewRect - the bounding
  * rectangle in surface coordinates.
  */

class ProjectionSurface: public QObject
{
  Q_OBJECT
public:
  enum InteractionMode {MoveMode = 0, PickSingleMode, PickTubeMode, AddPeakMode, DrawMode, EraseMode, InteractionModeSize };
  /// Constructor
  ProjectionSurface(const InstrumentActor* rootActor);
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
  virtual void resize(int, int);
  /// redraw surface without recalulationg of colours, etc
  virtual void updateView(bool picking = true);
  /// full update and redraw of the surface
  virtual void updateDetectors();
  /// returns the bounding rectangle in the real coordinates
  virtual RectF getSurfaceBounds()const{return m_viewRect;}

  virtual void mousePressEvent(QMouseEvent*);
  virtual void mouseMoveEvent(QMouseEvent*);
  virtual void mouseReleaseEvent(QMouseEvent*);
  virtual void wheelEvent(QWheelEvent *);
  virtual void keyPressEvent(QKeyEvent*);
  virtual void enterEvent(QEvent*);
  virtual void leaveEvent(QEvent*);

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

  virtual QString getInfoText()const;
  /// Change the interaction mode
  virtual void setInteractionMode(int mode);

  //-----------------------------------

  Mantid::Kernel::V3D getDetectorPos(int x, int y) const;
  /// Return the current interaction mode
  int getInteractionMode()const{return m_interactionMode;}
  /// Ask current input controller if a context menu is allowed
  bool canShowContextMenu() const;

  /// Set background colour
  void setBackgroundColor(const QColor& color) {m_backgroundColor = color;}
  /// Get background colour
  QColor getBackgroundColor() const {return m_backgroundColor;}
  /// Send a redraw request to the surface owner
  void requestRedraw(bool resetPeakVisibility = false);
  /// Enable lighting if the implementation allows it
  void enableLighting(bool on);

  //-----------------------------------
  //    Mask methods
  //-----------------------------------

  /// Return bounding rect of the currently selected shape in the "original" coord system.
  /// It doesn't depend on the zooming of the surface
  RectF getCurrentBoundingRect()const{return m_maskShapes.getCurrentBoundingRect();}

  /// Set new bounding rect of the currently selected shape in the "original" coord system.
  /// This method resizes the shape to fit into the new rectangle.
  void setCurrentBoundingRect(const RectF& rect){m_maskShapes.setCurrentBoundingRect(rect);}

  /// Initialize interactive shape creation.
  /// @param type :: Type of the shape. For available types see code of Shape2DCollection::createShape(const QString& type,int x,int y) const
  /// @param borderColor :: The color of the shape outline.
  /// @param fillColor :: The fill color.
  void startCreatingShape2D(const QString& type,const QColor& borderColor,const QColor& fillColor = QColor());

  // Properties methods which allow the mask shapes to be modified with a property browser.

  /// Return a list of all properties of type double of the currently selected shape.
  QStringList getCurrentDoubleNames()const{return m_maskShapes.getCurrentDoubleNames();}

  /// Get value of a "double" property of the currently selected shape.
  /// @param prop :: Name of the property
  double getCurrentDouble(const QString& prop) const{return m_maskShapes.getCurrentDouble(prop);}

  /// Set value of a "double" property of the currently selected shape.
  /// @param prop :: Name of the property
  /// @param value :: New value
  void setCurrentDouble(const QString& prop, double value){m_maskShapes.setCurrentDouble(prop, value);}

  /// Return a list of all properties of type QPointF of the currently selected shape.
  QStringList getCurrentPointNames()const{return m_maskShapes.getCurrentPointNames();}

  /// Get value of a "QPointF" property of the currently selected shape.
  /// @param prop :: Name of the property
  QPointF getCurrentPoint(const QString& prop) const{return m_maskShapes.getCurrentPoint(prop);}

  /// Set value of a "QPointF" property of the currently selected shape.
  /// @param prop :: Name of the property
  /// @param value :: New value
  void setCurrentPoint(const QString& prop, const QPointF& value){m_maskShapes.setCurrentPoint(prop,value);}

  /// Check if a point on the scren is under any of the mask shapes
  bool isMasked(double x,double y)const{return m_maskShapes.isMasked(x,y);}
  /// Check if there are any masks defined
  bool hasMasks() const {return m_maskShapes.size() > 0;}
  /// Remove all mask shapes.
  void clearMask(){m_maskShapes.clear();}
  /// Change all border colors.
  void changeBorderColor(const QColor& color) {m_maskShapes.changeBorderColor(color);}

  //-----------------------------------
  //    Peaks overlay methods
  //-----------------------------------

  QList<PeakMarker2D*> getMarkersWithID(int detID)const;
  boost::shared_ptr<Mantid::API::IPeaksWorkspace> getEditPeaksWorkspace() const;
  QStringList getPeaksWorkspaceNames() const;
  void deletePeaksWorkspace(boost::shared_ptr<Mantid::API::IPeaksWorkspace> ws);
  void clearPeakOverlays();
  bool hasPeakOverlays() const {return !m_peakShapes.isEmpty();}
  void setPeakLabelPrecision(int n);
  int getPeakLabelPrecision() const {return m_peakLabelPrecision;}
  void setShowPeakRowsFlag(bool on);
  bool getShowPeakRowsFlag()const {return m_showPeakRows;}
  void setShowPeakLabelsFlag(bool on);
  bool getShowPeakLabelsFlag()const {return m_showPeakLabels;}

signals:

  // detector selection
  void singleComponentTouched(size_t);
  void singleComponentPicked(size_t);

  // shape manipulation
  void signalToStartCreatingShape2D(const QString& type,const QColor& borderColor,const QColor& fillColor);
  void shapeCreated();
  void shapeSelected();
  void shapesDeselected();
  void shapeChanged();
  void shapesCleared();
  void shapesRemoved();
  void shapeChangeFinished();

  // peaks
  void peaksWorkspaceAdded();
  void peaksWorkspaceDeleted();

  // other
  void redrawRequired();   ///< request redrawing of self
  void updateInfoText();   ///< request update of the info string at bottom of InstrumentWindow
  void executeAlgorithm(Mantid::API::IAlgorithm_sptr);

protected slots:

  void setSelectionRect(const QRect& rect);
  void emptySelectionRect();
  void selectMultipleMasks(const QRect& rect);
  void pickComponentAt(int x,int y);
  void touchComponentAt(int x,int y);
  void erasePeaks(const QRect& rect);

  void colorMapChanged();

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

  //-----------------------------------

  void draw(MantidGLWidget* widget,bool picking)const;
  void clear();
  QRect selectionRect()const;
  RectF selectionRectUV()const;
  size_t getPickID(int x, int y)const;
  void setInputController(int mode, InputController* controller);
  void setPeakVisibility() const;

  //-----------------------------------
  //     Protected data
  //-----------------------------------

  const InstrumentActor* m_instrActor;
  mutable QImage* m_viewImage;       ///< storage for view image
  mutable QImage* m_pickImage;       ///< storage for picking image
  QColor m_backgroundColor;          ///< The background colour
  RectF m_viewRect;                  ///< Keeps the physical dimensions of the surface
  QRect m_selectRect;
  int m_interactionMode;             ///< mode of interaction - index in m_inputControllers
  bool m_isLightingOn;               ///< Lighting on/off flag

  Shape2DCollection m_maskShapes;    ///< to draw mask shapes
  mutable QList<PeakOverlay*> m_peakShapes; ///< to draw peak labels
  mutable int m_peakLabelPrecision;
  mutable bool m_showPeakRows;        ///< flag to show peak row index
  mutable bool m_showPeakLabels;     ///< flag to show peak hkl labels
  mutable int m_peakShapesStyle;     ///< index of a default PeakMarker2D style to use with a new PeakOverlay.

private:
  /// Get the current input controller
  InputController* getController() const;

  QMap<int,InputController*> m_inputControllers; ///< controllers for mouse and keyboard input
  /// Set when the image must be redrawn
  mutable bool m_viewChanged;
  /// Set when the picking image must be redrawn regardless of the interaction mode
  mutable bool m_redrawPicking;
};

typedef boost::shared_ptr<ProjectionSurface> ProjectionSurface_sptr;

#endif // PROJECTIONSURFACE_H
