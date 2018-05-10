#ifndef MANTIDPLOT_SHAPE2DCOLLECTION_H_
#define MANTIDPLOT_SHAPE2DCOLLECTION_H_

#include "MantidAPI/ITableWorkspace.h"

#include "RectF.h"
#include "Shape2D.h"

#include <QList>
#include <QTransform>

class QPainter;
class QMouseEvent;
class QWheelEvent;
class QKeyEvent;

namespace MantidQt {
namespace MantidWidgets {

/**
 * Class Shape2DCollection is a collection of 2D shapes.
 * It supports operations on teh shapes such as adding, removing, and aditting
 *either
 * with the mouse via control points (CPs) or via properties.
 *
 * The shapes operate in two coordinate systems:
 * 1. 'Real' or logical coordinates
 * 2. Transformed screen coordinates
 *
 * Shape2DCollection must know the boundaries of the drawing area in logical and
 *transformed screen coords.
 * They are set by calling setWindow(...) method. The first argument is the
 *logical drawing rectangle and
 * the second one is the corresponding screen viewport in pixels. The individual
 *shapes draw themselves in the
 * logical coords and unaware of the screen ones at all. If the size of the
 *screen/widget changes setWindow
 * must be called again. Changing the logical drawing bounds translates and
 *zooms the picture. The transformation is done by Qt's QTransform object.
 */
class Shape2DCollection : public QObject, public Shape2D {
  Q_OBJECT
public:
  Shape2DCollection();
  ~Shape2DCollection() override;
  Shape2D *clone() const override { return nullptr; }
  void setWindow(const RectF &surface, const QRect &viewport) const;
  void draw(QPainter &painter) const override;
  virtual void addShape(Shape2D *, bool slct = false);
  virtual void removeShape(Shape2D *, bool sendSignal = true);
  virtual void removeShapes(const QList<Shape2D *> &);
  virtual void clear();

  void keyPressEvent(QKeyEvent *);

  bool selectAtXY(int x, int y, bool edit = true);
  bool selectAtXY(const QPointF &point, bool edit = true);
  void deselectAtXY(int x, int y);
  void deselectAtXY(const QPointF &point);
  bool selectIn(const QRect &rect);
  void removeCurrentShape();
  bool isEmpty() const { return m_shapes.isEmpty(); }
  size_t size() const { return static_cast<size_t>(m_shapes.size()); }
  void addToSelection(int i);
  bool hasSelection() const;

  RectF getCurrentBoundingRect() const;
  void setCurrentBoundingRect(const RectF &rect);
  // double properties
  QStringList getCurrentDoubleNames() const;
  double getCurrentDouble(const QString &prop) const;
  void setCurrentDouble(const QString &prop, double value);
  // QPointF properties
  QStringList getCurrentPointNames() const;
  QPointF getCurrentPoint(const QString &prop) const;
  void setCurrentPoint(const QString &prop, const QPointF &value);

  using Shape2D::isMasked; // Unhide base class method (avoids Intel compiler
                           // warning)
  // is a point in real space masked by any of the shapes
  bool isMasked(double x, double y) const;
  // collect all screen pixels that are masked by the shapes
  QList<QPoint> getMaskedPixels() const;

  // set the bounding rect of the current shape such that its real rect is given
  // by the argument
  void setCurrentBoundingRectReal(const QRectF &rect);

  /// Change border color of all shapes.
  void changeBorderColor(const QColor &color);
  /// Save shape collection to a Table workspace
  void saveToTableWorkspace();
  /// Load shape collectio from a Table workspace
  void loadFromTableWorkspace(Mantid::API::ITableWorkspace_const_sptr ws);
  /// Load settings for the shape 2D collection from a project file
  virtual void loadFromProject(const std::string &lines);
  /// Save settings for the shape 2D collection to a project file
  virtual std::string saveToProject() const override;

signals:

  void shapeCreated();
  void shapeSelected();
  void shapesDeselected();
  void shapesRemoved();
  void shapeChanged();
  void shapeChangeFinished();
  void cleared();

public slots:
  void addShape(const QString &type, int x, int y, const QColor &borderColor,
                const QColor &fillColor);
  void addFreeShape(const QPolygonF &, const QColor &borderColor,
                    const QColor &fillColor);
  void deselectAll();
  void moveRightBottomTo(int, int);
  void selectShapeOrControlPointAt(int x, int y);
  void addToSelectionShapeAt(int x, int y);
  void moveShapeOrControlPointBy(int dx, int dy);
  void touchShapeOrControlPointAt(int x, int y);
  void removeSelectedShapes();
  void restoreOverrideCursor();
  void drawFree(const QPolygonF &polygon);
  void eraseFree(const QPolygonF &polygon);

protected:
  void drawShape(QPainter &) const override {} // never called
  void addToPath(QPainterPath &) const override {}
  void refit() override;
  void resetBoundingRect() override;

  Shape2D *createShape(const QString &type, int x, int y) const;
  bool selectControlPointAt(int x, int y);
  void deselectControlPoint();
  bool isOverCurrentAt(int x, int y);
  bool isOverSelectionAt(int x, int y);
  void addToSelection(Shape2D *shape);
  void removeFromSelection(Shape2D *shape);
  void edit(Shape2D *shape);
  void finishEdit();
  QList<Shape2D *> getSelectedShapes() const { return m_selectedShapes; }

  QList<Shape2D *> m_shapes;
  mutable RectF
      m_surfaceRect; ///< original surface window in "real" coordinates
  mutable double m_wx, m_wy;
  mutable int m_h;                ///< original screen viewport height
  mutable QRect m_viewport;       ///< current screen viewport
  mutable QTransform m_transform; ///< current transform

  Shape2D *m_currentShape; ///< shape selected to edit (change size/shape)
  size_t m_currentCP;      ///< control point of m_currentShape selected to edit
  QList<Shape2D *>
      m_selectedShapes; ///< A list of selected shapes (can be moved or deleted)
  bool m_overridingCursor;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif /*MANTIDPLOT_SHAPE2DCOLLECTION_H_*/
