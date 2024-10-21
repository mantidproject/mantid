// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/IComponent.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"
#include "MantidQtWidgets/Common/MantidAlgorithmMetatype.h"

#include "InstrumentActor.h"
#include "PeakOverlay.h"
#include "RectF.h"
#include "Shape2DCollection.h"

#include <QColor>
#include <QImage>
#include <QList>
#include <QMap>
#include <QPoint>
#include <QSize>
#include <QStack>

#include <memory>

namespace Mantid {
namespace Geometry {
class IDetector;
}
namespace API {
class IPeaksWorkspace;
}
} // namespace Mantid
namespace MantidQt {
namespace MantidWidgets {
class InputController;
}
} // namespace MantidQt

class QMouseEvent;
class QWheelEvent;

namespace MantidQt {
namespace MantidWidgets {
class GLDisplay;

/**
* @class ProjectionSurface
* @brief Performs projection of an instrument onto a plane.
* @author Roman Tolchenov, Tessella plc
* @date 13 May 2011

* Performs projection of an instrument onto a plane. Draws the resulting image
on the screen.
* Supports selection and zooming.
*
* Iherited classes must implement the pure virtual methods and set m_viewRect -
the bounding
* rectangle in surface coordinates.
*/

class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW ProjectionSurface : public QObject {
  Q_OBJECT
public:
  enum InteractionMode {
    MoveMode = 0,
    PickSingleMode,
    PickTubeMode,
    AddPeakMode,
    ComparePeakMode,
    AlignPeakMode,
    EditShapeMode,
    DrawFreeMode,
    ErasePeakMode,
    InteractionModeSize
  };
  /// Constructor
  explicit ProjectionSurface(const IInstrumentActor *rootActor);
  ProjectionSurface()
      : m_viewImage(nullptr), m_pickImage(nullptr), m_isLightingOn(false), m_peakLabelPrecision(2),
        m_showPeakRows(false), m_showPeakLabels(false), m_showPeakRelativeIntensity(false), m_peakShapesStyle(0),
        m_viewChanged(true), m_redrawPicking(true) {};
  /// Destructor
  ~ProjectionSurface() override;
  /// Resets the instrument actor.
  virtual void resetInstrumentActor(const IInstrumentActor *rootActor);

  //-----------------------------------
  //     Public virtual methods
  //-----------------------------------

  /// draw the surface onto a GL widget
  virtual void draw(GLDisplay *widget) const;
  /// draw the surface onto a normal widget
  virtual void drawSimple(QWidget *widget) const;
  /// called when the gl widget gets resized
  virtual void resize(int /*unused*/, int /*unused*/);
  /// redraw surface without recalulationg of colours, etc
  virtual void updateView(bool picking = true);
  /// full update and redraw of the surface
  virtual void updateDetectors();
  /// returns the bounding rectangle in the real coordinates
  virtual RectF getSurfaceBounds() const { return m_viewRect; }

  virtual void mousePressEvent(QMouseEvent * /*e*/);
  virtual void mouseMoveEvent(QMouseEvent * /*e*/);
  virtual void mouseReleaseEvent(QMouseEvent * /*e*/);
  virtual void wheelEvent(QWheelEvent * /*e*/);
  virtual void keyPressEvent(QKeyEvent * /*e*/);
  virtual void enterEvent(QEvent * /*e*/);
  virtual void leaveEvent(QEvent * /*e*/);

  /// return true if any of the detectors have been selected
  virtual bool hasSelection() const;

  virtual int getDetectorID(int x, int y) const;
  virtual size_t getDetector(int x, int y) const;
  /// NULL deselects components and selects the whole instrument
  virtual void componentSelected(size_t componentIndex) = 0;
  /// fill in a list of detector indices which were selected by the selction
  /// tool
  virtual void getSelectedDetectors(std::vector<size_t> &detIndices) = 0;
  /// fill in a list of detector indices which were masked by the mask shapes
  virtual void getMaskedDetectors(std::vector<size_t> &detIndices) const = 0;

  virtual QString getInfoText() const;
  /// Change the interaction mode
  virtual void setInteractionMode(int mode);

  /// Save masks to a table workspace
  virtual void saveShapesToTableWorkspace();

  //-----------------------------------

  Mantid::Kernel::V3D getDetectorPos(int x, int y) const;
  /// Return the current interaction mode
  int getInteractionMode() const { return m_interactionMode; }
  /// Ask current input controller if a context menu is allowed
  bool canShowContextMenu() const;

  /// Set background colour
  void setBackgroundColor(const QColor &color) { m_backgroundColor = color; }
  /// Get background colour
  QColor getBackgroundColor() const { return m_backgroundColor; }
  /// Send a redraw request to the surface owner
  void requestRedraw(bool resetPeakVisibility = false);
  /// Enable lighting if the implementation allows it
  void enableLighting(bool on);
  /// Load settings for the projection surface from a project file
  virtual void loadFromProject(const std::string &lines);
  /// Save settings for the projection surface to a project file
  virtual std::string saveToProject() const;

  void setCurrentTab(QString currentTab) { m_currentTab = currentTab; }
  void freezeRotation(bool freeze);
  void toggleToolTip(bool);

  //-----------------------------------
  //    Mask methods
  //-----------------------------------

  /// Return bounding rect of the currently selected shape in the "original"
  /// coord system.
  /// It doesn't depend on the zooming of the surface
  RectF getCurrentBoundingRect() const { return m_maskShapes.getCurrentBoundingRect(); }

  /// Set new bounding rect of the currently selected shape in the "original"
  /// coord system.
  /// This method resizes the shape to fit into the new rectangle.
  void setCurrentBoundingRect(const RectF &rect) { m_maskShapes.setCurrentBoundingRect(rect); }

  /// Return bounding rotation of the currently selected shape in the "original"
  /// coord system.
  /// It doesn't depend on the zooming of the surface
  double getCurrentBoundingRotation() const { return m_maskShapes.getCurrentBoundingRotation(); }

  /// Set new bounding rect of the currently selected shape in the "original"
  /// coord system.
  /// This method resizes the shape to fit into the new rectangle.
  void setCurrentBoundingRotation(const double rotation) { m_maskShapes.setCurrentBoundingRotation(rotation); }

  std::string getCurrentShapeType() const { return m_maskShapes.getCurrentShapeType(); }

  /// Initialize interactive shape creation.
  /// @param type :: Type of the shape. For available types see code of
  /// Shape2DCollection::createShape(const QString& type,int x,int y) const
  /// @param borderColor :: The color of the shape outline.
  /// @param fillColor :: The fill color.
  void startCreatingShape2D(const QString &type, const QColor &borderColor, const QColor &fillColor = QColor());

  /// Initialize interactive creation of a free draw shape.
  /// @param borderColor :: The color of the shape outline.
  /// @param fillColor :: The fill color.
  void startCreatingFreeShape(const QColor &borderColor, const QColor &fillColor = QColor());

  // Properties methods which allow the mask shapes to be modified with a
  // property browser.

  /// Return a list of all properties of type double of the currently selected
  /// shape.
  QStringList getCurrentDoubleNames() const { return m_maskShapes.getCurrentDoubleNames(); }

  /// Get value of a "double" property of the currently selected shape.
  /// @param prop :: Name of the property
  double getCurrentDouble(const QString &prop) const { return m_maskShapes.getCurrentDouble(prop); }

  /// Set value of a "double" property of the currently selected shape.
  /// @param prop :: Name of the property
  /// @param value :: New value
  void setCurrentDouble(const QString &prop, double value) { m_maskShapes.setCurrentDouble(prop, value); }

  /// Return a list of all properties of type QPointF of the currently selected
  /// shape.
  QStringList getCurrentPointNames() const { return m_maskShapes.getCurrentPointNames(); }

  /// Get value of a "QPointF" property of the currently selected shape.
  /// @param prop :: Name of the property
  QPointF getCurrentPoint(const QString &prop) const { return m_maskShapes.getCurrentPoint(prop); }

  /// Set value of a "QPointF" property of the currently selected shape.
  /// @param prop :: Name of the property
  /// @param value :: New value
  void setCurrentPoint(const QString &prop, const QPointF &value) { m_maskShapes.setCurrentPoint(prop, value); }

  /// Check if a point on the scren is under any of the mask shapes
  bool isMasked(double x, double y) const { return m_maskShapes.isMasked(x, y); }
  /// Check if there are any masks defined
  bool hasMasks() const { return m_maskShapes.size() > 0; }
  /// Remove all mask shapes.
  void clearMask() { m_maskShapes.clear(); }
  /// Change all border colors.
  void changeBorderColor(const QColor &color) { m_maskShapes.changeBorderColor(color); }

  /// Load masks from a table workspace
  void loadShapesFromTableWorkspace(const Mantid::API::ITableWorkspace_const_sptr &ws);

  /// Draw a 2D shape onto the surface with the given coordinates
  void drawShape2D(const QString &type, const QColor &borderColor, const QColor &fillColor, const QPoint &topLeftPos,
                   const QPoint &bottomRightPos, const bool select);
  void clearMaskedShapes();

  //-----------------------------------
  //    Peaks overlay methods
  //-----------------------------------

  QList<PeakMarker2D *> getMarkersWithID(int detID) const;
  std::shared_ptr<Mantid::API::IPeaksWorkspace> getEditPeaksWorkspace() const;
  QStringList getPeaksWorkspaceNames() const;
  void deletePeaksWorkspace(const std::shared_ptr<Mantid::API::IPeaksWorkspace> &ws);
  void clearPeakOverlays();
  void clearAlignmentPlane();
  void clearComparisonPeaks();
  bool hasPeakOverlays() const { return !m_peakShapes.isEmpty(); }
  int getPeakOverlayCount() const { return m_peakShapes.count(); }
  void setPeakLabelPrecision(int n);
  int getPeakLabelPrecision() const { return m_peakLabelPrecision; }
  void setShowPeakRowsFlag(bool on);
  bool getShowPeakRowsFlag() const { return m_showPeakRows; }
  void setShowPeakLabelsFlag(bool on);
  bool getShowPeakLabelsFlag() const { return m_showPeakLabels; }
  void setShowPeakRelativeIntensityFlag(bool on);
  bool getShowPeakRelativeIntensityFlag() const { return m_showPeakRelativeIntensity; }

signals:

  // detector selection
  void singleComponentTouched(size_t /*_t1*/);
  void singleComponentPicked(size_t /*_t1*/);
  void singleComponentPickedForMasking(size_t /*_t1*/);

  // shape manipulation
  void signalToStartCreatingShape2D(const QString &type, const QColor &borderColor, const QColor &fillColor);
  void signalToStartCreatingFreeShape(const QColor &borderColor, const QColor &fillColor);
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
  void alignPeaks(const std::vector<Mantid::Kernel::V3D> & /*_t1*/, const Mantid::Geometry::IPeak * /*_t2*/);
  void comparePeaks(
      const std::pair<std::vector<Mantid::Geometry::IPeak *>, std::vector<Mantid::Geometry::IPeak *>> & /*_t1*/);

  // other
  void redrawRequired(); ///< request redrawing of self
  void updateInfoText(); ///< request update of the info string at bottom of InstrumentWindow
  void executeAlgorithm(Mantid::API::IAlgorithm_sptr /*_t1*/);

protected slots:

  void setSelectionRect(const QRect &rect);
  void emptySelectionRect();
  void selectMultipleMasks(const QRect &rect);
  void pickComponentAt(int x, int y);
  void touchComponentAt(int x, int y);
  void erasePeaks(const QRect &rect);
  void comparePeaks(const QRect &rect);
  void alignPeaks(const QRect &rect);
  void showToolTip(int x, int y);

  void colorMapChanged();
  void refreshView();

protected:
  //-----------------------------------
  //     Protected virtual methods
  //-----------------------------------

  virtual void init() = 0;
  /// Draw the surface onto an OpenGL widget
  virtual void drawSurface(GLDisplay *widget, bool picking = false) const = 0;
  /// Respond to a change of color map in m_instrActor
  virtual void changeColorMap() = 0;
  /// Draw the surface onto an image without OpenGL
  virtual void drawSimpleToImage(QImage *image, bool picking = false) const;

  //-----------------------------------

  void draw(GLDisplay *widget, bool picking) const;
  void clear();
  QRect selectionRect() const;
  size_t getPickID(int x, int y) const;
  void setInputController(int mode, MantidQt::MantidWidgets::InputController *controller);
  void setPeakVisibility() const;

  //-----------------------------------
  //     Protected data
  //-----------------------------------

  const IInstrumentActor *m_instrActor;
  mutable QImage *m_viewImage; ///< storage for view image
  mutable QImage *m_pickImage; ///< storage for picking image
  QColor m_backgroundColor;    ///< The background colour
  RectF m_viewRect;            ///< Keeps the physical dimensions of the surface
  QRect m_selectRect;
  int m_interactionMode; ///< mode of interaction - index in m_inputControllers
  bool m_isLightingOn;   ///< Lighting on/off flag

  Shape2DCollection m_maskShapes;            ///< to draw mask shapes
  mutable QList<PeakOverlay *> m_peakShapes; ///< to draw peak labels
  mutable int m_peakLabelPrecision;
  mutable bool m_showPeakRows;      ///< flag to show peak row index
  mutable bool m_showPeakLabels;    ///< flag to show peak hkl labels
  bool m_showPeakRelativeIntensity; ///< flag to show peak hkl labels
  mutable int m_peakShapesStyle;    ///< index of a default PeakMarker2D style to

  std::vector<std::pair<Mantid::Kernel::V3D, QPointF>> m_selectedAlignmentPlane;
  std::pair<Mantid::Geometry::IPeak *, QPointF> m_selectedAlignmentPeak;

  std::pair<std::vector<Mantid::Geometry::IPeak *>, std::vector<Mantid::Geometry::IPeak *>> m_selectedPeaks;
  std::pair<QPointF, QPointF> m_selectedMarkers;

private:
  /// Draw a line between two peak markers
  void drawPeakComparisonLine(QPainter &painter) const;
  /// Draw the peak markers on the surface
  void drawPeakMarkers(QPainter &painter) const;
  /// Draw the mask shapes on the surface
  void drawMaskShapes(QPainter &painter) const;
  /// Draw the selection rectangle to the surface
  void drawSelectionRect(QPainter &painter) const;
  /// Draw the alignment markers on the surface
  void drawPeakAlignmentMarkers(QPainter &painter) const;
  /// Check if a peak is visible at a given point
  bool peakVisibleAtPoint(const QPointF &point) const;
  /// Get the current input controller
  MantidQt::MantidWidgets::InputController *getController() const;

  QMap<int, MantidQt::MantidWidgets::InputController *>
      m_inputControllers; ///< controllers for mouse and keyboard input
                          /// Set when the image must be redrawn
  mutable bool m_viewChanged;
  /// Set when the picking image must be redrawn regardless of the interaction
  /// mode
  mutable bool m_redrawPicking;
  QString m_currentTab;

  friend class InstrumentWidgetEncoder;
  friend class InstrumentWidgetDecoder;
};

using ProjectionSurface_sptr = std::shared_ptr<ProjectionSurface>;

} // namespace MantidWidgets
} // namespace MantidQt
