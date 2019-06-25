// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDPLOT_SHAPE2D_H_
#define MANTIDPLOT_SHAPE2D_H_

#include "RectF.h"

#include <QColor>
#include <QPointF>

class QPainter;
class QPainterPath;
class QMouseEvent;
class QWheelEvent;

namespace MantidQt {
namespace MantidWidgets {

/**
 * Base class for an editable 2D shape, which can be drawn on ProjectionSurface.
 *
 * Any shape must implement these pure virtual methods:
 *    clone()     -- to copy itself
 *    drawShape() -- to draw itself
 *    refit()     -- to make sure the shape is entirely within its bounding rect
 *                   ( returned by getBoundingRect() )
 *    addToPath() -- to add itself to a QPainterPath, to allow this shape to be
 *used to
 *                   construct more complex shapes
 *
 * A shape has a border and the area inside this border. A point on the screen
 * which is inside the border is considered to be masked by this shape.
 * To be able to mask anything a shape must implement selectAt(...) and
 *contains(...)
 * methods.
 *
 * A shape can be a part of a Shape2DCollection. Collections allow to apply
 *transformations to
 * their items. A scale transformation is used to implement zooming. By default
 *a shape resizes with the transformation. To override this call setScalable(
 *false ). In this case zooming only changes the visible position but not the
 *size of the shape.
 *
 * Shapes can be edited (ie change their position and sizes) in a generic way
 *either by using coltrol
 * points or setting properties.
 *
 */
class Shape2D {
public:
  /// Constructor.
  Shape2D();
  /// Virtual destructor.
  virtual ~Shape2D() {}

  // --- Public pure virtual methods --- //

  /// Virtual "costructor".
  virtual Shape2D *clone() const = 0;
  /// modify path so painter.drawPath(path) could be used to draw the shape.
  /// needed for filling in complex shapes
  virtual void addToPath(QPainterPath &path) const = 0;
  /// make sure the shape is within the bounding box
  virtual void refit() = 0;

  // --- Public virtual methods --- //

  /// Draw this shape.
  virtual void draw(QPainter &painter) const;
  /// Get the origin - the centre of the bounding rect.
  virtual QPointF origin() const { return m_boundingRect.center(); }
  /// Move the shape by a vector.
  virtual void moveBy(const QPointF &pos);
  /// Get total number of control points.
  virtual size_t getNControlPoints() const;
  /// Get a control point.
  virtual QPointF getControlPoint(size_t i) const;
  /// Set a control point.
  virtual void setControlPoint(size_t i, const QPointF &pos);
  /// Return the bounding rect of the shape.
  virtual RectF getBoundingRect() const { return m_boundingRect; }
  /// move the left, top, right and bottom sides of the bounding rect
  /// by dx1, dy1, dx2, and dy2 correspondingly
  virtual void adjustBoundingRect(double dx1, double dy1, double dx2,
                                  double dy2);
  /// Set new bounding rect.
  virtual void setBoundingRect(const RectF &rect);
  /// will the shape be selected if clicked at a point? By default return false.
  virtual bool selectAt(const QPointF & /*unused*/) const { return false; }
  /// is a point inside the shape (closed line)? By default return false.
  virtual bool contains(const QPointF & /*unused*/) const { return false; }
  /// is a point "masked" by the shape. Only filled regions of a shape mask a
  /// point
  virtual bool isMasked(const QPointF & /*p*/) const;
  /// Set border color.
  virtual void setColor(const QColor &color) { m_color = color; }
  /// Get border color.
  virtual QColor getColor() const { return m_color; }
  /// Set fill color.
  virtual void setFillColor(const QColor &color) { m_fill_color = color; }
  /// Get fill color.
  virtual QColor getFillColor() const { return m_fill_color; }

  // --- Public methods --- //

  /// Set the shape scalable (default).
  void setScalable(bool on) { m_scalable = on; }
  /// Can the shape be scaled by Shape2DCollection's transformation?
  bool isScalable() const { return m_scalable; }
  /// Set the shape editable. Makes visible the bounding rect and teh control
  /// points.
  void edit(bool on) { m_editing = on; }
  /// Check if the shape is being edited.
  bool isEditing() const { return m_editing; }
  /// Show or hide the shape
  void setVisible(bool on) { m_visible = on; }
  /// Is shape visible?
  bool isVisible() const { return m_visible; }
  /// Select/deselect the shape
  void setSelected(bool on) { m_selected = on; }
  /// Is shape selected?
  bool isSelected() const { return m_selected; }
  /// Load settings for the widget tab from a project file
  static Shape2D *loadFromProject(const std::string &lines);
  /// Save settings for the widget tab to a project file
  virtual std::string saveToProject() const;
  virtual std::string type() const { return "base"; }

  // --- Properties. for gui interaction --- //

  // double properties
  virtual QStringList getDoubleNames() const { return QStringList(); }
  virtual double getDouble(const QString &prop) const {
    (void)prop;
    return 0.0;
  }
  virtual void setDouble(const QString &prop, double value) {
    (void)prop;
    (void)value;
  }

  // QPointF properties
  virtual QStringList getPointNames() const { return QStringList(); }
  virtual QPointF getPoint(const QString &prop) const {
    (void)prop;
    return QPointF();
  }
  virtual void setPoint(const QString &prop, const QPointF &value) {
    (void)prop;
    (void)value;
  }

protected:
  // --- Protected pure virtual methods --- //

  virtual void drawShape(QPainter &painter) const = 0;

  // --- Protected virtual methods --- //

  // return number of control points specific to this shape
  virtual size_t getShapeNControlPoints() const { return 0; }
  // returns position of a shape specific control point, 0 < i <
  // getShapeNControlPoints()
  virtual QPointF getShapeControlPoint(size_t /*unused*/) const {
    return QPointF();
  }
  // sets position of a shape specific control point, 0 < i <
  // getShapeNControlPoints()
  virtual void setShapeControlPoint(size_t /*unused*/,
                                    const QPointF & /*unused*/) {}
  // make sure the bounding box is correct
  virtual void resetBoundingRect() {}

  // --- Protected data --- //

  static const size_t NCommonCP;
  static const qreal sizeCP;
  RectF m_boundingRect;
  QColor m_color;
  QColor m_fill_color;
  bool m_scalable; ///< shape can be scaled when zoomed
  bool m_editing;  ///< shape is being edited
  bool m_selected; ///< shape is selected
  bool m_visible;  ///< flag to show or hide the shape

private:
  /// Instantiate specifc shapes from a type string
  static Shape2D *loadShape2DFromType(const std::string &type,
                                      const std::string &lines);

  friend class InstrumentWidgetEncoder;
  friend class InstrumentWidgetDecoder;
};

/**
 * An ellipse with the axes parallel to the x and y axes on the screen.
 *
 * It has a QPointF property "center" defining the centre of the ellipse
 * and double properties "radius1" and "radius2" equal to distances from
 * the centre to the curve along the x and y axes.
 */
class Shape2DEllipse : public Shape2D {
public:
  Shape2DEllipse(const QPointF &center, double radius1, double radius2 = 0);
  Shape2D *clone() const override { return new Shape2DEllipse(*this); }
  bool selectAt(const QPointF &p) const override;
  bool contains(const QPointF &p) const override;
  void addToPath(QPainterPath &path) const override;
  // double properties
  QStringList getDoubleNames() const override;
  double getDouble(const QString &prop) const override;
  void setDouble(const QString &prop, double value) override;
  // QPointF properties
  QStringList getPointNames() const override { return QStringList("center"); }
  QPointF getPoint(const QString &prop) const override;
  void setPoint(const QString &prop, const QPointF &value) override;
  /// Load state for the shape from a project file
  static Shape2D *loadFromProject(const std::string &lines);
  /// Save state for the shape to a project file
  virtual std::string saveToProject() const override;
  std::string type() const override { return "ellipse"; }

protected:
  void drawShape(QPainter &painter) const override;
  void refit() override {}
};

/**
 * A axis aligned rectangle.
 *
 * No specific properties.
 */
class Shape2DRectangle : public Shape2D {
public:
  Shape2DRectangle();
  Shape2DRectangle(const QPointF &p0, const QPointF &p1);
  Shape2DRectangle(const QPointF &p0, const QSizeF &size);
  Shape2D *clone() const override { return new Shape2DRectangle(*this); }
  bool selectAt(const QPointF &p) const override;
  bool contains(const QPointF &p) const override {
    return m_boundingRect.contains(p);
  }
  void addToPath(QPainterPath &path) const override;
  /// Load state for the shape from a project file
  static Shape2D *loadFromProject(const std::string &lines);
  /// Save state for the shape to a project file
  virtual std::string saveToProject() const override;
  std::string type() const override { return "rectangle"; }

protected:
  void drawShape(QPainter &painter) const override;
  void refit() override {}
};

/**
 * A ring: area bounded by two curves of the same shape but different size.
 *
 * The constructor takes a curve shape and the ring widths in the x and y
 * directions.
 * It has QPointF "centre" property and "xwidth" and "ywidth" double properties.
 */
class Shape2DRing : public Shape2D {
public:
  Shape2DRing(Shape2D *shape, double xWidth = 0.000001,
              double yWidth = 0.000001);
  Shape2DRing(const Shape2DRing &ring);
  Shape2D *clone() const override { return new Shape2DRing(*this); }
  bool selectAt(const QPointF &p) const override;
  bool contains(const QPointF &p) const override;
  // double properties
  QStringList getDoubleNames() const override;
  double getDouble(const QString &prop) const override;
  void setDouble(const QString &prop, double value) override;
  // QPointF properties
  QStringList getPointNames() const override { return QStringList("center"); }
  QPointF getPoint(const QString &prop) const override;
  void setPoint(const QString &prop, const QPointF &value) override;
  void setColor(const QColor &color) override;
  QColor getColor() const override { return m_outer_shape->getColor(); }
  const Shape2D *getOuterShape() const { return m_outer_shape; }
  /// Load state for the shape from a project file
  static Shape2D *loadFromProject(const std::string &lines);
  /// Save state for the shape to a project file
  virtual std::string saveToProject() const override;
  std::string type() const override { return "ring"; }

protected:
  void drawShape(QPainter &painter) const override;
  void addToPath(QPainterPath & /*path*/) const override {}
  void refit() override;
  void resetBoundingRect() override;
  size_t getShapeNControlPoints() const override { return 4; }
  QPointF getShapeControlPoint(size_t i) const override;
  void setShapeControlPoint(size_t i, const QPointF &pos) override;
  Shape2D *m_outer_shape;
  Shape2D *m_inner_shape;
  double m_xWidth;
  double m_yWidth;
};

/**
 * An arbitrary shape. Implemented as a polygon.
 * It can have disjointed parts and holes.
 *
 * No shape specific properties.
 */
class Shape2DFree : public Shape2D {
public:
  explicit Shape2DFree(const QPointF &p);
  explicit Shape2DFree(const QPolygonF &polygon);
  Shape2D *clone() const override { return new Shape2DFree(*this); }
  bool selectAt(const QPointF &p) const override;
  bool contains(const QPointF &p) const override;
  void addToPath(QPainterPath &path) const override;
  void addPolygon(const QPolygonF &polygon);
  void subtractPolygon(const QPolygonF &polygon);
  /// Load state for the shape from a project file
  static Shape2D *loadFromProject(const std::string &lines);
  /// Save state for the shape to a project file
  virtual std::string saveToProject() const override;
  std::string type() const override { return "free"; }

protected:
  void drawShape(QPainter &painter) const override;
  void refit() override;
  void resetBoundingRect() override;

private:
  RectF getPolygonBoundingRect() const;
  QPolygonF m_polygon;    ///< Implements the shape.
  QPainterPath m_outline; ///< Object to draw the shape's border.
  friend class InstrumentWidgetEncoder;
  friend class InstrumentWidgetDecoder;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif /*MANTIDPLOT_SHAPE2D_H_*/
